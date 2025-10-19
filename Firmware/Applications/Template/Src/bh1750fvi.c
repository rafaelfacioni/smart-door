/*BH1750FVI C Driver*/
/*Reza Ebrahimi - https://github.com/ebrezadev*/
/*Version 1.0*/
#include "bh1750fvi.h"
#include <stdint.h>
#include <stdio.h>

static uint8_t sensor_address; /*accepts one of two possible addresses for bh1750fvi i2c*/
static uint16_t measure_result;
static uint8_t current_mode;          /*current mode of operation: POWER_DOWN, POWER_ON, CONTINUOUS_H_1_MODE, CONTINUOUS_H_2_MODE, etc*/
static uint8_t auto_or_manual;        /*automatic or manual resolution selecting indicator: AUTOMATIC_RESOLUTION and MANUAL_RESOLUTION*/
static uint8_t continuous_or_onetime; /*CONTINUOUS_METER or ONE_TIME_METER indicator*/

struct delay_time
{
  uint16_t continuous_h_1;
  uint16_t continuous_h_2;
  uint16_t continuous_l;
  uint16_t one_time_h_1;
  uint16_t one_time_h_2;
  uint16_t one_time_l;
};

struct delay_time delay_time = {
    .continuous_h_1 = CONTINUOUS_H_1_DELAY_DEFAULT,
    .continuous_h_2 = CONTINUOUS_H_2_DELAY_DEFAULT,
    .continuous_l = CONTINUOUS_L_DELAY_DEFAULT,
    .one_time_h_1 = ONE_TIME_H_1_DELAY_DEFAULT,
    .one_time_h_2 = ONE_TIME_H_2_DELAY_DEFAULT,
    .one_time_l = ONE_TIME_L_DELAY_DEFAULT};

static void lightSensor_readBuffer();
static void lightSensor_automaticResolutionSet(uint16_t meter_value);

/*this should be called prior to lightSensor_meter().
 *address could be either ADDRESS1 or ADDRESS2, mode could be POWER_DOWN, CONTINUOUS_H_1_MODE... in manual or CONTINUOUS_AUTO or ONE_TIME_AUTO*/
void lightSensor_begin(uint8_t address, uint8_t mode_of_operation)
{
  bh1750_I2C_init();
  sensor_address = address;
  lightSensor_mode(POWER_DOWN);
  lightSensor_mode(RESET);
  switch (mode_of_operation)
  {
  case CONTINUOUS_AUTO:
    lightSensor_mode(CONTINUOUS_L_MODE); /*both auto modes start with low resolution mode*/
    auto_or_manual = AUTOMATIC_RESOLUTION;
    break;
  case ONE_TIME_AUTO:
    lightSensor_mode(ONE_TIME_L_MODE); /*both auto modes start with low resolution mode*/
    auto_or_manual = AUTOMATIC_RESOLUTION;
    break;
  default:
    lightSensor_mode(mode_of_operation); /*contains 3 continuous, 3 one-time modes and wrong commands. wrong commands are treated as one time H1 resolution mode*/
    auto_or_manual = MANUAL_RESOLUTION;
    break;
  }
}

/*function to set modes*/
void lightSensor_mode(uint8_t command)
{
  switch (command)
  {
  case POWER_DOWN:
    light_i2c_write_single(sensor_address, POWER_DOWN);
    break;
  case POWER_ON:
    light_i2c_write_single(sensor_address, POWER_ON);
    break;
  case RESET:
    light_i2c_write_single(sensor_address, RESET);
    break;
  case CONTINUOUS_H_1_MODE:
    light_i2c_write_single(sensor_address, CONTINUOUS_H_1_MODE);
    delay_function(delay_time.continuous_h_1);
    continuous_or_onetime = CONTINUOUS_METER;
    break;
  case CONTINUOUS_H_2_MODE:
    light_i2c_write_single(sensor_address, CONTINUOUS_H_2_MODE);
    delay_function(delay_time.continuous_h_2);
    continuous_or_onetime = CONTINUOUS_METER;
    break;
  case CONTINUOUS_L_MODE:
    light_i2c_write_single(sensor_address, CONTINUOUS_L_MODE);
    delay_function(delay_time.continuous_l);
    continuous_or_onetime = CONTINUOUS_METER;
    break;
  case ONE_TIME_H_1_MODE:
    light_i2c_write_single(sensor_address, ONE_TIME_H_1_MODE);
    delay_function(delay_time.one_time_h_1);
    continuous_or_onetime = ONE_TIME_METER;
    break;
  case ONE_TIME_H_2_MODE:
    light_i2c_write_single(sensor_address, ONE_TIME_H_2_MODE);
    delay_function(delay_time.one_time_h_2);
    continuous_or_onetime = ONE_TIME_METER;
    break;
  case ONE_TIME_L_MODE:
    light_i2c_write_single(sensor_address, ONE_TIME_L_MODE);
    delay_function(delay_time.one_time_l);
    continuous_or_onetime = ONE_TIME_METER;
    break;
  default:
    light_i2c_write_single(sensor_address, ONE_TIME_H_1_MODE);
    delay_function(delay_time.one_time_h_1);
    command = ONE_TIME_H_1_MODE; /*since the default mode contains possible wrong commands*/
    continuous_or_onetime = ONE_TIME_METER;
    break;
  }
  current_mode = command;
}

/*main function that commands BH1750FVI to meter light, either in one time or continuous modes, and in auto or manual modes*/
uint16_t lightSensor_meter()
{
  switch (auto_or_manual)
  {
  case MANUAL_RESOLUTION:
    if (continuous_or_onetime == CONTINUOUS_METER)
    {
      lightSensor_readBuffer();
      return measure_result;
    }
    else if (continuous_or_onetime == ONE_TIME_METER)
    {
      lightSensor_mode(current_mode);
      lightSensor_readBuffer();
      return measure_result;
    }
    break;
  case AUTOMATIC_RESOLUTION:
    if (continuous_or_onetime == CONTINUOUS_METER)
    {
      lightSensor_readBuffer();
      lightSensor_automaticResolutionSet(measure_result);
      return measure_result;
    }
    else if (continuous_or_onetime == ONE_TIME_METER)
    {
      lightSensor_mode(current_mode);
      lightSensor_readBuffer();
      lightSensor_automaticResolutionSet(measure_result);
      return measure_result;
    }
    break;
  }
  // fallback seguro caso nenhuma condição seja satisfeita
  return 0;
}

static void lightSensor_readBuffer()
{
  uint8_t data_array[2];
  light_i2c_read(sensor_address, data_array);
  measure_result = (uint16_t)(data_array[1]) + (uint16_t)(data_array[0] << 8);
  if ((current_mode == ONE_TIME_H_2_MODE) || (current_mode == CONTINUOUS_H_2_MODE))
  {
    measure_result >>= 1; /*since H2 mode has a resolution of 0.5 lux*/
  }
}

/*function to set resolution based on current metered value*/
static void lightSensor_automaticResolutionSet(uint16_t meter_value)
{
  uint8_t change_flag = 0;
  if ((meter_value < LIMIT1) && (current_mode != CONTINUOUS_H_2_MODE) && (current_mode != ONE_TIME_H_2_MODE))
  {
    if (continuous_or_onetime == CONTINUOUS_METER)
    {
      current_mode = CONTINUOUS_H_2_MODE;
    }
    else if (continuous_or_onetime == ONE_TIME_METER)
    {
      current_mode = ONE_TIME_H_2_MODE;
    }
    change_flag = 1;
  }
  else if ((meter_value >= LIMIT1) && (meter_value < LIMIT2) && (current_mode != CONTINUOUS_H_1_MODE) && (current_mode != ONE_TIME_H_1_MODE))
  {
    if (continuous_or_onetime == CONTINUOUS_METER)
    {
      current_mode = CONTINUOUS_H_1_MODE;
    }
    else if (continuous_or_onetime == ONE_TIME_METER)
    {
      current_mode = ONE_TIME_H_1_MODE;
    }
    change_flag = 1;
  }
  else if ((meter_value >= LIMIT2) && (current_mode != CONTINUOUS_L_MODE) && (current_mode != ONE_TIME_L_MODE))
  {
    if (continuous_or_onetime == CONTINUOUS_METER)
    {
      current_mode = CONTINUOUS_L_MODE;
    }
    else if (continuous_or_onetime == ONE_TIME_METER)
    {
      current_mode = ONE_TIME_L_MODE;
    }
    change_flag = 1;
  }
  if (change_flag)
  {
    lightSensor_mode(current_mode);
  }
}
