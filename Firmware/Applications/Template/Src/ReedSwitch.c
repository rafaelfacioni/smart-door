#include "ReedSwitch.h"
#include "HT_MQTT_Api.h"

#include <stdio.h>

#include <pad_qcx212.h>
#include <HT_gpio_qcx212.h>
#include <FreeRTOS.h>
#include <task.h>

#define SAMPLES 32000

// GPIO02 - ReedSwitch
#define REED_SWITCH_INSTANCE 0               /**</ REED_SWITCH pin instance. */
#define REED_SWITCH_PIN 2                    /**</ REED_SWITCH pin number. */
#define REED_SWITCH_PAD_ID 13                /**</ REED_SWITCH Pad ID. */
#define REED_SWITCH_PAD_ALT_FUNC PAD_MuxAlt0 /**</ Button pin alternate function. */

void ReedSwitchInit(void)
{
    GPIO_InitType GPIO_InitStruct = {0};
    GPIO_InitStruct.af = REED_SWITCH_PAD_ALT_FUNC;
    GPIO_InitStruct.pad_id = REED_SWITCH_PAD_ID;
    GPIO_InitStruct.gpio_pin = REED_SWITCH_PIN;
    GPIO_InitStruct.pin_direction = GPIO_DirectionInput;
    GPIO_InitStruct.instance = REED_SWITCH_INSTANCE;

    HT_GPIO_Init(&GPIO_InitStruct);
}

ReadState ReedSwitchGetState(void)
{
    uint32_t filter = 0;
    for (int i = 0; i < SAMPLES; i++)
    {
        filter += HT_GPIO_PinRead(REED_SWITCH_INSTANCE, REED_SWITCH_PIN);
    }
    filter /= SAMPLES;
    if (filter == 1)
    {
        return kOpen;
    }
    return kClose;
}

uint8_t ChangeState(void)
{
    static uint8_t estadoAnterior = 0;
    uint8_t estadoAtual = ReedSwitchGetState();

    if (estadoAtual != estadoAnterior)
    {
        if (estadoAnterior == 1 && estadoAtual == 0)
        {
            HT_MQTT_Publish(&mqttClient, topic_door,(uint8_t*)"OPEN", strlen("OPEN"), QOS0, 0, 0, 0);
        }
        else if (estadoAnterior == 0 && estadoAtual == 1)
        {
            HT_MQTT_Publish(&mqttClient, topic_door,(uint8_t*)"CLOSE", strlen("CLOSE"), QOS0, 0, 0, 0);
        }
        estadoAnterior = estadoAtual;
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    return estadoAtual;
}

