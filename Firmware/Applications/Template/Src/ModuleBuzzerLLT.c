#include "ModuleBuzzerLLT.h"

#include <stdio.h>

#include "clock_qcx212.h"
#include <HT_gpio_qcx212.h>

// GPIO04- Module_Buzzer_low_level_Triger
#define BUZZER_INSTANCE 0               /**</ BUZZER pin instance. */
#define BUZZER_PIN 10                   /**</ BUZZER pin number. */
#define BUZZER_PAD_ID 25                /**</ BUZZER Pad ID. */
#define BUZZER_PAD_ALT_FUNC PAD_MuxAlt5 /**</ BUZZER pin alternate function. */

#define PWM_CLOCK_ID (GPR_TIMER0FuncClk)        // PWM clock id for clock configuration
#define PWM_CLOCK_SOURCE (GPR_TIMER0ClkSel_26M) // PWM clock source select
#define DUTY_CYCLE_PERCENT (50)                 // PWM initial duty cycle percent

void BuzzerInit(void)
{
    // Configurações de pino
    GPIO_InitType GPIO_InitStruct = {0};
    GPIO_InitStruct.af = BUZZER_PAD_ALT_FUNC; // PWM
    GPIO_InitStruct.pad_id = BUZZER_PAD_ID;
    GPIO_InitStruct.gpio_pin = BUZZER_PIN;
    GPIO_InitStruct.init_output = 1;
    GPIO_InitStruct.pin_direction = GPIO_DirectionOutput;
    GPIO_InitStruct.instance = BUZZER_INSTANCE;

    HT_GPIO_Init(&GPIO_InitStruct);
}

void PWM_Init(void)
{
    // Clock do PWM
    CLOCK_SetClockSrc(PWM_CLOCK_ID, PWM_CLOCK_SOURCE); 
    CLOCK_SetClockDiv(PWM_CLOCK_ID, 1);

    TIMER_DriverInit();
}
