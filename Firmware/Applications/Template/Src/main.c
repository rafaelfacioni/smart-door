#include "string.h"
#include "bsp.h"
#include "ostask.h"
#include "debug_log.h"
#include "FreeRTOS.h"
#include "main.h"
#include "bh1750fvi.h"

/**
  \fn          static void appInit(void *arg)
  \brief
  \return
*/

static void readLux(void *args);
static void readSwitch(void *arg);

extern USART_HandleTypeDef huart1;

static void readSwitch(void *arg)
{
    while (1)
    {
      bool switchState = GPIO_PinRead(SWITCH_INSTANCE, SWITCH_PIN);
      GPIO_PinWrite(LED_INSTANCE, 1 << LED_PIN, (switchState ? 1 << LED_PIN : 0));
    }
}
// 1124 lux para 20% no app
//
static void readLux(void *args)
{
  printf("Teste Task readLux\n");
  lightSensor_begin(ADDRESS1,CONTINUOUS_AUTO);
  int lux = 0;
  while (1)
  {
    lux = lightSensor_meter();
    printf("Luminosidade: %i lux\n", lux);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void appInit(void *arg)
{
    GPIO_deviceInit(DEVICE_LED);
    GPIO_deviceInit(DEVICE_SWITCH);
    slpManNormalIOVoltSet(IOVOLT_3_30V);

    uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE |
                         ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);

    xTaskCreate(readSwitch, "ReadSwitch", 2048, NULL, 1, NULL);
    xTaskCreate(readLux, "ReadLux", 2048, NULL, 1, NULL);
    return;
}

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void)
{ 
    
    BSP_CommonInit();
   
    osKernelInitialize();
    registerAppEntry(appInit, NULL);
    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }
    while(1);
}
