/* USER CODE BEGIN Header */
/**
  ******************************************************************************
 * @file    app_vl53l0x.c
 * @author  MCD Application Team
 * @brief   Proximity Application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_vl53l0x.h"
#include "stm32_seq.h"
#include "stm32wb5mm_dk.h"
#include "stm32wb5mm_dk_lcd.h"
#include "stm32_lcd.h"
#include "stm32wb5mm_dk_bus.h"

/* Private defines -----------------------------------------------------------*/ 
#define PROXIMITY_UPDATE_PERIOD       (uint32_t)(0.5*1000*1000/CFG_TS_TICK_VAL) /*500ms*/
#define DISTANCE_MAX_PROXIMITY        2000  /* 2m */

#define PROXIMITY_I2C_ADDRESS            0x53U

/* Private variables ---------------------------------------------------------*/   

/* Proximity */ 
VL53L0X_Dev_t Dev =
{
  .I2cHandle = &hbus_i2c3,
  .I2cDevAddr = PROXIMITY_I2C_ADDRESS
};

uint8_t VL53L0X_PROXIMITY_Update_Timer_Id;

/* Private function prototypes -----------------------------------------------*/

/**
  * @brief  VL53L0X proximity sensor Initialization.
  */
void VL53L0X_PROXIMITY_Init(void)
{
  uint16_t vl53l0x_id = 0; 
  VL53L0X_DeviceInfo_t VL53L0X_DeviceInfo;
  
  /* Initialize IO interface */
  STM32WB5MM_DK_I2C_Init();
  
  memset(&VL53L0X_DeviceInfo, 0, sizeof(VL53L0X_DeviceInfo_t));
  
  if (VL53L0X_ERROR_NONE == VL53L0X_GetDeviceInfo(&Dev, &VL53L0X_DeviceInfo))
  {  
    if (VL53L0X_ERROR_NONE == VL53L0X_RdWord(&Dev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, (uint16_t *) &vl53l0x_id))
    {
      if (vl53l0x_id == VL53L0X_ID)
      {
        if (VL53L0X_ERROR_NONE == VL53L0X_DataInit(&Dev))
        {
          Dev.Present = 1;
          SetupSingleShot(Dev);
        }
        else
        { 
          while(1){}  // VL53L0X Time of Flight Failed to send its ID!
        }
      }
    }
    else
    {
      while(1){} // VL53L0X Time of Flight Failed to Initialize!
    }
  }
  else
  {
    while(1){} // VL53L0X Time of Flight Failed to get infos!
  } 

}



/**
  * @brief  Get distance from VL53L0X proximity sensor.
  * @param  None
  * @retval Distance in mm
  */
uint16_t VL53L0X_PROXIMITY_GetDistance(void)
{
  VL53L0X_RangingMeasurementData_t RangingMeasurementData;
  
  VL53L0X_PerformSingleRangingMeasurement(&Dev, &RangingMeasurementData);
  
  return RangingMeasurementData.RangeMilliMeter;  
}

/**
  * @brief  Print distance measure from VL53L0X proximity sensor on the OLED screen.
  * @param  None
  * @retval None
  */
void VL53L0X_PROXIMITY_PrintValue(void){
      UTIL_LCD_ClearStringLine(2);
      char distanceText[18];
      uint16_t prox_value = 0;
      uint16_t distance = 0;
      prox_value = VL53L0X_PROXIMITY_GetDistance();
      if(prox_value < DISTANCE_MAX_PROXIMITY){
        distance = prox_value / 10;
        sprintf(distanceText,"Distance : %3d cm",distance);
        UTIL_LCD_DisplayStringAtLine(2,(uint8_t*)distanceText);
      }else{
        UTIL_LCD_DisplayStringAtLine(2,(uint8_t*)"Distance > 200 cm");
      }
      BSP_LCD_Refresh(0);
}

