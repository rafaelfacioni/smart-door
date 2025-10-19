#include "HT_MQTT_Api.h"

// GPIO03 - LED VERDE - STATUS BROKER
#define LED_VERDE_INSTANCE 0               /**</ LED VERDE pin instance. */
#define LED_VERDE_PIN 3                    /**</ LED VERDE pin number. */
#define LED_VERDE_PAD_ID 14                /**</ LED VERDE Pad ID. */
#define LED_VERDE_PAD_ALT_FUNC PAD_MuxAlt0 /**</ Button pin alternate function. */

// GPIO05 - LED AZUL - PUBLISH
#define LED_AZUL_INSTANCE 0               /**</ LED AZUL pin instance. */
#define LED_AZUL_PIN 5                    /**</ LED AZUL pin number. */
#define LED_AZUL_PAD_ID 16                /**</ LED AZUL Pad ID. */
#define LED_AZUL_PAD_ALT_FUNC PAD_MuxAlt0 /**</ Button pin alternate function. */


void LedVerdeInit(void)
{
    GPIO_InitType GPIO_InitStruct = {0};
    GPIO_InitStruct.af = LED_VERDE_PAD_ALT_FUNC;
    GPIO_InitStruct.pad_id = LED_VERDE_PAD_ID;
    GPIO_InitStruct.gpio_pin = LED_VERDE_PIN;
    GPIO_InitStruct.pin_direction = GPIO_DirectionOutput;
    GPIO_InitStruct.instance = LED_VERDE_INSTANCE;
    GPIO_InitStruct.init_output = 1;
    HT_GPIO_Init(&GPIO_InitStruct);
}

void LedAzulInit(void)
{
    GPIO_InitType GPIO_InitStruct = {0};
    GPIO_InitStruct.af = LED_AZUL_PAD_ALT_FUNC;
    GPIO_InitStruct.pad_id = LED_AZUL_PAD_ID;
    GPIO_InitStruct.gpio_pin = LED_AZUL_PIN;
    GPIO_InitStruct.pin_direction = GPIO_DirectionOutput;
    GPIO_InitStruct.instance = LED_AZUL_INSTANCE;
    GPIO_InitStruct.init_output = 1;
    HT_GPIO_Init(&GPIO_InitStruct);
}


/*===========================================================
Inicio - Configuração de rede 
=============================================================*/
static QueueHandle_t psEventQueueHandle;

static uint8_t gImsi[16] = {0}; // código do sim
static uint32_t gCellID = 0;    // Identificador da célula NB-IoT
static NmAtiSyncRet gNetworkInfo; // Informações de IP, rede, et.

//Subcribe callback flag
volatile uint8_t subscribe_callback = 0;

static volatile uint8_t simReady = 0; // Flag para verificar SIM card

trace_add_module(APP, P_INFO);
extern void mqtt_demo_onenet(void);

static void HT_SetConnectioParameters(void)
{
    uint8_t cid = 0;
    PsAPNSetting apnSetting;
    int32_t ret;
    uint8_t networkMode = 0; // nb-iot network mode
    uint8_t bandNum = 1;
    uint8_t band = 28;

    ret = appSetBandModeSync(networkMode, bandNum, &band); // Conf Banda de operação NB-IoT
    if (ret == CMS_RET_SUCC)
    {
        printf("SetBand Result: %d\n", ret);
    }

    apnSetting.cid = 0;
    apnSetting.apnLength = strlen("iot.datatem.com.br");
    strcpy((char *)apnSetting.apnStr, "iot.datatem.com.br");
    apnSetting.pdnType = CMI_PS_PDN_TYPE_IP_V4V6;
    ret = appSetAPNSettingSync(&apnSetting, &cid);
}

static void sendQueueMsg(uint32_t msgId, uint32_t xTickstoWait) {
    eventCallbackMessage_t *queueMsg = NULL;
    queueMsg = malloc(sizeof(eventCallbackMessage_t));
    queueMsg->messageId = msgId;
    if (psEventQueueHandle)
    {
        if (pdTRUE != xQueueSend(psEventQueueHandle, &queueMsg, xTickstoWait))
        {
            HT_TRACE(UNILOG_MQTT, mqttAppTask80, P_INFO, 0, "xQueueSend error");
        }
    }
}


//Callback principal para eventos da rede NB-IoT
static INT32 registerPSUrcCallback(urcID_t eventID, void *param, uint32_t paramLen) {
    CmiSimImsiStr *imsi = NULL;
    CmiPsCeregInd *cereg = NULL;
    UINT8 rssi = 0;
    NmAtiNetifInfo *netif = NULL;

    switch(eventID)
    {
        case NB_URC_ID_SIM_READY: //SIM está funcional
        {
            imsi = (CmiSimImsiStr *)param;
            memcpy(gImsi, imsi->contents, imsi->length);
            simReady = 1;
            break;
        }
        case NB_URC_ID_MM_SIGQ: //Nível de sinal RSSI
        {
            rssi = *(UINT8 *)param;
            HT_TRACE(UNILOG_MQTT, mqttAppTask81, P_INFO, 1, "RSSI signal=%d", rssi);
            break;
        }
        case NB_URC_ID_PS_BEARER_ACTED: //Ativação da conexao
        {
            HT_TRACE(UNILOG_MQTT, mqttAppTask82, P_INFO, 0, "Default bearer activated");
            break;
        }
        case NB_URC_ID_PS_BEARER_DEACTED: //desativação da conexao
        {
            HT_TRACE(UNILOG_MQTT, mqttAppTask83, P_INFO, 0, "Default bearer Deactivated");
            break;
        }
        case NB_URC_ID_PS_CEREG_CHANGED: // ID da célula mudou
        {
            cereg = (CmiPsCeregInd *)param;
            gCellID = cereg->celId;
            HT_TRACE(UNILOG_MQTT, mqttAppTask84, P_INFO, 4, "CEREG changed act:%d celId:%d locPresent:%d tac:%d", cereg->act, cereg->celId, cereg->locPresent, cereg->tac);
            break;
        }
        case NB_URC_ID_PS_NETINFO: // Rede conectada, dispara evento NW_IPV4_READY
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
                sendQueueMsg(QMSG_ID_NW_IPV4_READY, 0);
            break;
        }

        default:
            break;
    }
    return 0;
}

/*===========================================================
Fim - Configuração de rede 
=============================================================*/

/*===========================================================
Inicio - Funções para uso do MQTT
=============================================================*/

static uint8_t mqttEpSlpHandler = 0xff; // Para Sleep Management


static MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

#if MQTT_TLS_ENABLE == 1
static MqttClientContext mqtt_client_ctx;
#endif

uint8_t HT_MQTT_Connect(MQTTClient *mqtt_client, Network *mqtt_network, char *addr, int32_t port, uint32_t send_timeout, uint32_t rcv_timeout, char *clientID,
                        char *username, char *password, uint8_t mqtt_version, uint32_t keep_alive_interval, uint8_t *sendbuf,
                        uint32_t sendbuf_size, uint8_t *readbuf, uint32_t readbuf_size)
{

#if MQTT_TLS_ENABLE == 1
    mqtt_client_ctx.caCertLen = 0;
    mqtt_client_ctx.port = port;
    mqtt_client_ctx.host = addr;
    mqtt_client_ctx.timeout_ms = MQTT_GENERAL_TIMEOUT;
    mqtt_client_ctx.isMqtt = true;
    mqtt_client_ctx.timeout_r = MQTT_GENERAL_TIMEOUT;
    mqtt_client_ctx.timeout_s = MQTT_GENERAL_TIMEOUT;
#endif

    connectData.MQTTVersion = mqtt_version;
    connectData.clientID.cstring = clientID;
    connectData.username.cstring = username;
    connectData.password.cstring = password;
    connectData.keepAliveInterval = keep_alive_interval;
    connectData.will.qos = QOS0;
    connectData.cleansession = false;

#if MQTT_TLS_ENABLE == 1

    printf("Starting TLS handshake...\n");

    if (HT_MQTT_TLSConnect(&mqtt_client_ctx, mqtt_network) != 0)
    {
        printf("TLS Connection Error!\n");
        return 1;
    }

    MQTTClientInit(mqtt_client, mqtt_network, MQTT_GENERAL_TIMEOUT, (unsigned char *)sendbuf, sendbuf_size, (unsigned char *)readbuf, readbuf_size);

    if ((MQTTConnect(mqtt_client, &connectData)) != 0)
    {
        mqtt_client->ping_outstanding = 1;
        return 1;
    }
    else
    {
        mqtt_client->ping_outstanding = 0;
    }

#else

    NetworkInit(mqtt_network);
    MQTTClientInit(mqtt_client, mqtt_network, MQTT_GENERAL_TIMEOUT, (unsigned char *)sendbuf, sendbuf_size, (unsigned char *)readbuf, readbuf_size);

    if ((NetworkSetConnTimeout(mqtt_network, send_timeout, rcv_timeout)) != 0)
    {
        mqtt_client->keepAliveInterval = connectData.keepAliveInterval;
        mqtt_client->ping_outstanding = 1;
    }
    else
    {

        if ((NetworkConnect(mqtt_network, addr, port)) != 0)
        {
            mqtt_client->keepAliveInterval = connectData.keepAliveInterval;
            mqtt_client->ping_outstanding = 1;

            return 1;
        }
        else
        {
            if ((MQTTConnect(mqtt_client, &connectData)) != 0)
            {
                mqtt_client->ping_outstanding = 1;
                return 1;
            }
            else
            {
                mqtt_client->ping_outstanding = 0;
            }
        }
    }

#endif

    return 0;
}

void HT_MQTT_Publish(MQTTClient *mqtt_client, char *topic, uint8_t *payload, uint32_t len, enum QoS qos, uint8_t retained, uint16_t id, uint8_t dup)
{
    HT_GPIO_WritePin(LED_AZUL_PIN,LED_AZUL_INSTANCE,0);
    MQTTMessage message;

    message.qos = qos;
    message.retained = retained;
    message.id = id;
    message.dup = dup;
    message.payload = payload;
    message.payloadlen = len;

    MQTTPublish(mqtt_client, topic, &message);
    HT_GPIO_WritePin(LED_AZUL_PIN,LED_AZUL_INSTANCE,1);
}

QueueHandle_t buzzerQueue;


void HT_Yield_Task(void *arg);
uint8_t comando = 0;

/*------MQTT-----*/
MQTTClient mqttClient;
static Network mqttNetwork;

//Buffer that will be published.
static uint8_t mqttSendbuf[HT_MQTT_BUFFER_SIZE] = {0};
static uint8_t mqttReadbuf[HT_MQTT_BUFFER_SIZE] = {0};

static const char clientID[] = {"SIP_HTNB32L-XXX"};
static const char username[] = {""};
static const char password[] = {""};

//MQTT broker host address
static const char addr[] = {"131.255.82.115"};

// MQTT Topics to subscribe
char topic_buzzer[] = {"hana/mesanino/smartdoor/buzzer"};

// MQTT Topics to Publish
char topic_light[] = {"hana/mesanino/smartdoor/lux"};
char topic_door[] = {"hana/mesanino/smartdoor/door"};

//Buffers
static uint8_t subscribed_payload[HT_SUBSCRIBE_BUFF_SIZE] = {0}; // PayLoad Recebida
static uint8_t subscribed_topic[255] = {0}; 

static void HT_FSM_Topic_Subscribe(void) {
    
    HT_MQTT_Subscribe(&mqttClient, topic_buzzer, QOS0);
    printf("Subscribe Done!\n");
}

void HT_MQTT_Subscribe(MQTTClient *mqtt_client, char *topic, enum QoS qos) {
    MQTTSubscribe(mqtt_client, (const char *)topic, qos, HT_MQTT_SubscribeCallback);
}

void HT_MQTT_SubscribeCallback(MessageData *msg) {
    printf("Subscribe recebido: %s\n", msg->message->payload);

    subscribe_callback = 1;
    HT_FSM_SetSubscribeBuff(msg);

    // 1º Verificar se é o tópico
    // 2º Verificar se a mensagem recebida é ON
    if (strncmp((char *)msg->topicName->lenstring.data, topic_buzzer, msg->topicName->lenstring.len) == 0) {

        if (strncmp((char *)msg->message->payload, "ON", msg->message->payloadlen) == 0) {
            comando = 1;
            xQueueSend(buzzerQueue, &comando, 0);
        } else if (strncmp((char *)msg->message->payload, "OFF", msg->message->payloadlen) == 0) {
            comando = 0;
            xQueueSend(buzzerQueue, &comando, 0);
        }
    }
    memset(msg->message->payload, 0, msg->message->payloadlen);
}

static HT_ConnectionStatus HT_FSM_MQTTConnect(void) {

    // Connect to MQTT Broker using client, network and parameters needded. 
    if(HT_MQTT_Connect(&mqttClient, &mqttNetwork, (char *)addr, HT_MQTT_PORT, HT_MQTT_SEND_TIMEOUT, HT_MQTT_RECEIVE_TIMEOUT,
                (char *)clientID, (char *)username, (char *)password, HT_MQTT_VERSION, HT_MQTT_KEEP_ALIVE_INTERVAL, mqttSendbuf, HT_MQTT_BUFFER_SIZE, mqttReadbuf, HT_MQTT_BUFFER_SIZE)) {
        return HT_NOT_CONNECTED;   
    }

    return HT_CONNECTED;
}

void HT_FSM_SetSubscribeBuff(MessageData *msg) {
    memset(subscribed_payload, 0, HT_SUBSCRIBE_BUFF_SIZE);
    memset(subscribed_topic, 0, strlen((char *)subscribed_topic));
    memcpy(subscribed_payload, msg->message->payload, msg->message->payloadlen);
    memcpy(subscribed_topic, msg->topicName->lenstring.data, msg->topicName->lenstring.len);
}

// para manter a conexão MQTT ativa
void HT_Yield_Task(void *arg) {
    while (1) {
        int rc = MQTTYield(&mqttClient, 100); // 100ms para evitar uso excessivo da CPU

        if (rc != 0) {
            printf("MQTT Disconnected,Trying to reconnect...\n");
            HT_GPIO_WritePin(LED_VERDE_PIN,LED_VERDE_INSTANCE,1);
            // Tentar reconectar indefinidamente
            if (HT_FSM_MQTTConnect() == HT_NOT_CONNECTED) {
                vTaskDelay(pdMS_TO_TICKS(3000)); // espera 3s antes de tentar de novo
                printf("Retrying MQTT connection...\n");
            }

            HT_FSM_Topic_Subscribe(); // Reinscreve após reconectar
            printf("MQTT Reconnected!\n");
            HT_GPIO_WritePin(LED_VERDE_PIN,LED_VERDE_INSTANCE,0);

        }

        vTaskDelay(pdMS_TO_TICKS(100)); // um pequeno delay para não travar CPU
    }
}

/*===========================================================
FIM - Funções para uso do MQTT
=============================================================*/

//DEVE SER EXECUTADO PRIMEIRO PARA INICIAR A REDE NB-IoT E EM SEGUIDA DO O MQTT

void NbiotMqttInit(void *arg){
    int32_t ret;
    uint8_t psmMode = 0, actType = 0;
    uint16_t tac = 0;
    uint32_t tauTime = 0, activeTime = 0, cellID = 0, nwEdrxValueMs = 0, nwPtwMs = 0;

    eventCallbackMessage_t *queueItem = NULL;

    registerPSEventCallback(NB_GROUP_ALL_MASK, registerPSUrcCallback);
    psEventQueueHandle = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(eventCallbackMessage_t*));
    if (psEventQueueHandle == NULL)
    {
        HT_TRACE(UNILOG_MQTT, mqttAppTask0, P_INFO, 0, "psEventQueue create error!");
        return;
    }

    slpManApplyPlatVoteHandle("EP_MQTT",&mqttEpSlpHandler);
    slpManPlatVoteDisableSleep(mqttEpSlpHandler, SLP_ACTIVE_STATE); //SLP_SLP2_STATE 
    HT_TRACE(UNILOG_MQTT, mqttAppTask1, P_INFO, 0, "first time run mqtt example");


    while(!simReady);
    HT_SetConnectioParameters();

    while (1)
    {
        if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY))
        {
            switch(queueItem->messageId)
            {
                case QMSG_ID_NW_IPV4_READY:
                case QMSG_ID_NW_IPV6_READY:
                case QMSG_ID_NW_IPV4_6_READY:
                    appGetImsiNumSync((CHAR *)gImsi);
                    HT_STRING(UNILOG_MQTT, mqttAppTask2, P_SIG, "IMSI = %s", gImsi);
                
                    appGetNetInfoSync(gCellID, &gNetworkInfo);
                    if ( NM_NET_TYPE_IPV4 == gNetworkInfo.body.netInfoRet.netifInfo.ipType)
                        HT_TRACE(UNILOG_MQTT, mqttAppTask3, P_INFO, 4,"IP:\"%u.%u.%u.%u\"", ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[0],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[1],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[2],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[3]);
                    ret = appGetLocationInfoSync(&tac, &cellID);
                    HT_TRACE(UNILOG_MQTT, mqttAppTask4, P_INFO, 3, "tac=%d, cellID=%d ret=%d", tac, cellID, ret);
                    // edrxModeValue = CMI_MM_ENABLE_EDRX_AND_ENABLE_IND;
                    // actType = CMI_MM_EDRX_NB_IOT;
                    actType = CMI_MM_EDRX_NO_ACT_OR_NOT_USE_EDRX;
                    //reqEdrxValueMs = 20480;
                    ret = appSetEDRXSettingSync(CMI_MM_DISABLE_EDRX, actType, 0);
                    ret = appGetEDRXSettingSync(&actType, &nwEdrxValueMs, &nwPtwMs);
                    HT_TRACE(UNILOG_MQTT, mqttAppTask5, P_INFO, 4, "actType=%d, nwEdrxValueMs=%d nwPtwMs=%d ret=%d", actType, nwEdrxValueMs, nwPtwMs, ret);
                    printf("actType=%d, nwEdrxValueMs=%d nwPtwMs=%d ret=%d\n", actType, nwEdrxValueMs, nwPtwMs, ret);

                    psmMode = 0;
                    tauTime = 0;
                    activeTime = 0;

                    {
                        appSetPSMSettingSync(psmMode, tauTime, activeTime);
                        appGetPSMSettingSync(&psmMode, &tauTime, &activeTime);
                        HT_TRACE(UNILOG_MQTT, mqttAppTask6, P_INFO, 3, "Get PSM info mode=%d, TAU=%d, ActiveTime=%d", psmMode, tauTime, activeTime);
                        printf("Get PSM info mode=%d, TAU=%d, ActiveTime=%d\n", psmMode, tauTime, activeTime);
                    }

                    HT_Fsm();

                    break;
                case QMSG_ID_NW_DISCONNECT:
                    printf("NB Disconected\n");
                    break;

                default:
                    break;
            }
            free(queueItem);
        }
        osDelay(pdMS_TO_TICKS(10));
    }

}

//Chama as Tasks
void HT_Fsm(void) {

    // Initialize MQTT Client and Connect to MQTT Broker defined in global variables
    if(HT_FSM_MQTTConnect() == HT_NOT_CONNECTED) {
        printf("\n MQTT Connection Error!\n");
    }
    else
    {
        printf("MQTT Connection Success!\n");
        HT_GPIO_WritePin(LED_VERDE_PIN,LED_VERDE_INSTANCE,0);
    }
    HT_FSM_Topic_Subscribe();
    xTaskCreate(HT_Yield_Task, "MQTT_Yield",configMINIMAL_STACK_SIZE, NULL, 2, NULL);
}


