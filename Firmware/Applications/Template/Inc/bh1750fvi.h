#include "htnb32lxxx_hal_i2c.h"
#include "Driver_I2C.h"
#include "FreeRTOS.h"
#include "task.h"

/*BH1750FVI C Driver*/
/*Reza Ebrahimi - https://github.com/ebrezadev"*/
/*Version 1.0*/
#ifndef BH1750FVI_H
#define BH1750FVI_H

#define ADDRESS1                         0x23 //(0x23 << 1)       /*address pin not connected*/
#define ADDRESS2                              //(0x5C << 1)       /*address pin high*/

#define LIMIT1                            10          /*anything below LIMIT1 is H2 mode*/      
#define LIMIT2                            1000        /*anything between LIMIT1 and LIMIT2 is H1 mode, above LIMIT2 is L mode*/

#define POWER_DOWN                        0x00
#define POWER_ON                          0x01
#define RESET                             0x07
#define CONTINUOUS_H_1_MODE               0x10        /*1 lx 120ms*/
#define CONTINUOUS_H_2_MODE               0x11        /*0.5 lx 120ms*/
#define CONTINUOUS_L_MODE                 0x13        /*4 lx 16ms*/
#define ONE_TIME_H_1_MODE                 0x20
#define ONE_TIME_H_2_MODE                 0x21
#define ONE_TIME_L_MODE                   0x23

#define CONTINUOUS_H_1_DELAY_DEFAULT      120
#define CONTINUOUS_H_2_DELAY_DEFAULT      120
#define CONTINUOUS_L_DELAY_DEFAULT        16
#define ONE_TIME_H_1_DELAY_DEFAULT        120
#define ONE_TIME_H_2_DELAY_DEFAULT        120
#define ONE_TIME_L_DELAY_DEFAULT          16

#define CONTINUOUS_AUTO                   0x24
#define ONE_TIME_AUTO                     0x25
#define AUTOMATIC_RESOLUTION              0x26
#define MANUAL_RESOLUTION                 0x27
#define CONTINUOUS_METER                  0x28
#define ONE_TIME_METER                    0x29

#define NO_OPTICAL_WINDOW                 0x01

void lightSensor_begin(uint8_t address, uint8_t mode_of_operation);
void lightSensor_mode(uint8_t command);
uint16_t lightSensor_meter();

void light_i2c_write_single(uint8_t device_address, uint8_t command);
void light_i2c_read(uint8_t device_address, uint8_t *data_array);
void bh1750_I2C_init();
void delay_function(uint32_t delay_milliseconds);

void BH1750_Task(void *arg);

#endif
