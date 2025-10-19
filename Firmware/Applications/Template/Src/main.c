#include "string.h"
#include "bsp.h"
#include "ostask.h"
#include "debug_log.h"
#include "FreeRTOS.h"
#include "main.h"
#include "bh1750fvi.h"
#include "ReedSwitch.h"
#include "HT_MQTT_Api.h"
#include "ModuleBuzzerLLT.h"

extern USART_HandleTypeDef huart1;

void BH1750_Task(void *arg);
void ReedSwitch_Task(void *arg);
void Buzzer_Task(void *arg);

/**
  \fn          static void appInit(void *arg)
  \brief
  \return
*/
static void appInit(void *arg)
{
    xTaskCreate(NbiotMqttInit, "NbiotMqttInit", configMINIMAL_STACK_SIZE, NULL, 4, NULL);
    xTaskCreate(BH1750_Task, "BH1750", configMINIMAL_STACK_SIZE, NULL, 4, NULL);
    xTaskCreate(ReedSwitch_Task, "ReedSwitch", configMINIMAL_STACK_SIZE, NULL, 4, NULL);
    xTaskCreate(Buzzer_Task, "Buzzer", 512, NULL, 4, NULL);
}

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void)
{
    uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE |
                           ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);
    BSP_CommonInit();
    LedVerdeInit();
    LedAzulInit();
    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);

    osKernelInitialize();
    registerAppEntry(appInit, NULL);

    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }
    while (1);
}

/*=========================================
TASK PARA LEITURA DO SENSOR DE LUMINOSIDADE
===========================================*/

void BH1750_Task(void *arg)
{
    lightSensor_begin(ADDRESS1, CONTINUOUS_H_2_MODE);

    LampState estadoAnterior = LAMP_OFF;
    LampState estadoAtual;

    while (1)
    {
        uint16_t lux = lightSensor_meter();
        // Determino o estado atual da lampada com base no treshold
        if (lux > LUX_TRESHOLD)
        {
            estadoAtual = LAMP_ON;
        }
        else
        {
            estadoAtual = LAMP_OFF;
        }

        // Verificando mudançada de estado
        if (estadoAtual != estadoAnterior)
        {
            if (estadoAtual == LAMP_ON)
            {
                HT_MQTT_Publish(&mqttClient, topic_light, (uint8_t *)"ON", strlen("ON"), QOS0, 0, 0, 0);
            }
            else
            {
                HT_MQTT_Publish(&mqttClient, topic_light, (uint8_t *)"OFF", strlen("OFF"), QOS0, 0, 0, 0);
            }
            estadoAnterior = estadoAtual;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void ReedSwitch_Task(void *arg)
{
    ReedSwitchInit();
    while (1)
    {
        ChangeState();
    }
}

void Buzzer_Task(void *arg)
{
    BuzzerInit();
    PWM_Init();
    uint8_t buzzer_enabled = 0;

    buzzerQueue = xQueueCreate(4, sizeof(uint8_t));
    if (buzzerQueue == NULL)
    {
        printf("Erro ao criar fila do buzzer\n");
    }

    while (1)
    {
        if (xQueueReceive(buzzerQueue, &comando, 0) == pdTRUE)
        {
            buzzer_enabled = comando; // 1 = ON, 0 = OFF
        }

        if (buzzer_enabled == 1)
        {
            if (buzzer_enabled == 1)
            {
                timer_pwm_config_t pwmConfig;

                // DING - som mais agudo
                pwmConfig.pwmFreq_HZ = 440;
                pwmConfig.srcClock_HZ = GPR_GetClockFreq(GPR_TIMER0FuncClk);
                pwmConfig.dutyCyclePercent = 20;
                TIMER_SetupPwm(TIMER_INSTANCE, &pwmConfig);

                TIMER_Start(TIMER_INSTANCE);
                vTaskDelay(pdMS_TO_TICKS(250));
                TIMER_Stop(TIMER_INSTANCE);

                vTaskDelay(pdMS_TO_TICKS(100));

                // DONG - som mais grave
                pwmConfig.pwmFreq_HZ = 349;
                pwmConfig.srcClock_HZ = GPR_GetClockFreq(GPR_TIMER0FuncClk);
                pwmConfig.dutyCyclePercent = 20;
                TIMER_SetupPwm(TIMER_INSTANCE, &pwmConfig);

                TIMER_Start(TIMER_INSTANCE);
                vTaskDelay(pdMS_TO_TICKS(400));
                TIMER_Stop(TIMER_INSTANCE);

                vTaskDelay(pdMS_TO_TICKS(2000));
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(200)); // Em off
        }
    }
}
