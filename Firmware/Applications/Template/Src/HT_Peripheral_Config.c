#include "HT_Peripheral_Config.h"
#include "HT_gpio_qcx212.h"

void GPIO_deviceInit(GpioDevice_t device){

  GPIO_InitType GPIO_InitStruct = {0};

  switch (device)
  { 
    case DEVICE_LED:

      GPIO_InitStruct.af = PAD_MuxAlt0;
      GPIO_InitStruct.pad_id = LED_PAD_ID;
      GPIO_InitStruct.gpio_pin = LED_PIN;
      GPIO_InitStruct.pin_direction = GPIO_DirectionOutput;
      GPIO_InitStruct.init_output = 0;
      GPIO_InitStruct.pull = PAD_AutoPull;
      GPIO_InitStruct.instance = LED_INSTANCE;
      GPIO_InitStruct.exti = GPIO_EXTI_DISABLED;

      HT_GPIO_Init(&GPIO_InitStruct);
      break;
    
    case DEVICE_SWITCH:

      GPIO_InitStruct.af = PAD_MuxAlt0;
      GPIO_InitStruct.pad_id = SWITCH_PAD_ID;
      GPIO_InitStruct.gpio_pin = SWITCH_PIN;
      GPIO_InitStruct.pin_direction = GPIO_DirectionInput;
      GPIO_InitStruct.pull = PAD_InternalPullUp;
      GPIO_InitStruct.instance = SWITCH_INSTANCE;
      GPIO_InitStruct.exti = GPIO_EXTI_DISABLED;
      GPIO_InitStruct.interrupt_config = GPIO_InterruptFallingEdge;

      HT_GPIO_Init(&GPIO_InitStruct);
      break;

    default:
      break;
    }
}
