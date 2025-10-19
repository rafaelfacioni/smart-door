/*BH1750FVI C Driver*/
/*Reza Ebrahimi - https://github.com/ebrezadev*/
/*Version 1.0*/
#include "bh1750fvi.h"


extern ARM_DRIVER_I2C Driver_I2C0;
extern I2C_HandleTypeDef hi2c0;

static void HT_I2C_MasterReceiveCallback(uint32_t event);

/*transmits one byte of command to bh1750fvi. in case of bh1750 sensor, there's no internal register map. each 'write' to i2c is a 'command'*/
void light_i2c_write_single(uint8_t device_address, uint8_t command)
{
    HAL_I2C_MasterTransmit_IT(&hi2c0, device_address, &command, 1);
}

/*function to read two bytes of measured illuminance from bh1750fvi. note: there's no register address, just request two bytes of data from bh1750 (slave)*/
void light_i2c_read(uint8_t device_address, uint8_t *data_array)
{
    HAL_I2C_MasterReceive_IT(&hi2c0,device_address,data_array,2);
}

/*==============================================
Funções e variaveis para o init do i2c
================================================*/
static volatile uint8_t i2c_tx_callback = 0;
static volatile uint8_t i2c_rx_callback = 0;

static void HT_I2C_MasterReceiveCallback(uint32_t event) {

    if(event & ARM_I2C_EVENT_TX_DONE) {
        i2c_tx_callback = 1;
    } else if(event & ARM_I2C_EVENT_RX_DONE) {
        i2c_rx_callback = 1;
    }
}


void bh1750_I2C_init()
{
    HAL_I2C_InitClock(HT_I2C0);

    HAL_I2C_Initialize(HT_I2C_MasterReceiveCallback, &hi2c0);
    HAL_I2C_PowerControl(ARM_POWER_FULL, &hi2c0);
    
    HAL_I2C_Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD, &hi2c0);
    HAL_I2C_Control(ARM_I2C_BUS_CLEAR, 1, &hi2c0);
}



/*function for delay in milliseconds*/
void delay_function(uint32_t delay_milliseconds)
{
    vTaskDelay(pdMS_TO_TICKS(delay_milliseconds));
}
