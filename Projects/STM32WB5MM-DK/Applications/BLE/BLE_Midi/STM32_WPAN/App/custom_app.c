/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    App/custom_app.c
  * @author  MCD Application Team
  * @brief   Custom Example Application (Server)
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "main.h"
#include "app_common.h"
#include "dbg_trace.h"
#include "ble.h"
#include "custom_app.h"
#include "custom_stm.h"
#include "stm32_seq.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_midi.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  /* s_midi */
  uint8_t               C_io_Notification_Status;
  /* USER CODE BEGIN CUSTOM_APP_Context_t */

  /* USER CODE END CUSTOM_APP_Context_t */

  uint16_t              ConnectionHandle;
} Custom_App_Context_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/**
 * START of Section BLE_APP_CONTEXT
 */

static Custom_App_Context_t Custom_App_Context;

/**
 * END of Section BLE_APP_CONTEXT
 */

uint8_t UpdateCharData[247];
uint8_t NotifyCharData[247];

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* s_midi */
static void Custom_C_io_Update_Char(void);
static void Custom_C_io_Send_Notification(void);

/* USER CODE BEGIN PFP */
void    Midi_Send_Note(const uint8_t state, const uint8_t channel, 
                       const uint8_t note, const uint8_t velocity);
/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void Custom_STM_App_Notification(Custom_STM_App_Notification_evt_t *pNotification)
{
  /* USER CODE BEGIN CUSTOM_STM_App_Notification_1 */

  /* USER CODE END CUSTOM_STM_App_Notification_1 */
  switch (pNotification->Custom_Evt_Opcode)
  {
    /* USER CODE BEGIN CUSTOM_STM_App_Notification_Custom_Evt_Opcode */

    /* USER CODE END CUSTOM_STM_App_Notification_Custom_Evt_Opcode */

    /* s_midi */
    case CUSTOM_STM_C_IO_READ_EVT:
      /* USER CODE BEGIN CUSTOM_STM_C_IO_READ_EVT */

      /* USER CODE END CUSTOM_STM_C_IO_READ_EVT */
      break;

    case CUSTOM_STM_C_IO_WRITE_EVT:
      /* USER CODE BEGIN CUSTOM_STM_C_IO_WRITE_EVT */

      /* USER CODE END CUSTOM_STM_C_IO_WRITE_EVT */
      break;

    case CUSTOM_STM_C_IO_NOTIFY_ENABLED_EVT:
      /* USER CODE BEGIN CUSTOM_STM_C_IO_NOTIFY_ENABLED_EVT */

      /* USER CODE END CUSTOM_STM_C_IO_NOTIFY_ENABLED_EVT */
      break;

    case CUSTOM_STM_C_IO_NOTIFY_DISABLED_EVT:
      /* USER CODE BEGIN CUSTOM_STM_C_IO_NOTIFY_DISABLED_EVT */

      /* USER CODE END CUSTOM_STM_C_IO_NOTIFY_DISABLED_EVT */
      break;

    default:
      /* USER CODE BEGIN CUSTOM_STM_App_Notification_default */

      /* USER CODE END CUSTOM_STM_App_Notification_default */
      break;
  }
  /* USER CODE BEGIN CUSTOM_STM_App_Notification_2 */

  /* USER CODE END CUSTOM_STM_App_Notification_2 */
  return;
}

void Custom_APP_Notification(Custom_App_ConnHandle_Not_evt_t *pNotification)
{
  /* USER CODE BEGIN CUSTOM_APP_Notification_1 */

  /* USER CODE END CUSTOM_APP_Notification_1 */

  switch (pNotification->Custom_Evt_Opcode)
  {
    /* USER CODE BEGIN CUSTOM_APP_Notification_Custom_Evt_Opcode */

    /* USER CODE END P2PS_CUSTOM_Notification_Custom_Evt_Opcode */
    case CUSTOM_CONN_HANDLE_EVT :
      /* USER CODE BEGIN CUSTOM_CONN_HANDLE_EVT */
      Midi_Start_Measures();
      /* USER CODE END CUSTOM_CONN_HANDLE_EVT */
      break;

    case CUSTOM_DISCON_HANDLE_EVT :
      /* USER CODE BEGIN CUSTOM_DISCON_HANDLE_EVT */
      Midi_Stop_Measures();
      /* USER CODE END CUSTOM_DISCON_HANDLE_EVT */
      break;

    default:
      /* USER CODE BEGIN CUSTOM_APP_Notification_default */

      /* USER CODE END CUSTOM_APP_Notification_default */
      break;
  }

  /* USER CODE BEGIN CUSTOM_APP_Notification_2 */

  /* USER CODE END CUSTOM_APP_Notification_2 */

  return;
}

void Custom_APP_Init(void)
{
  /* USER CODE BEGIN CUSTOM_APP_Init */
  UNUSED(Custom_App_Context);

  /* Dummy calls to prevent build warning of unused function */
  Custom_C_io_Update_Char();
  Custom_C_io_Send_Notification();
  
  MIDI_Init();
  /* USER CODE END CUSTOM_APP_Init */
  return;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/

/* s_midi */
void Custom_C_io_Update_Char(void) /* Property Read */
{
  uint8_t updateflag = 0;

  /* USER CODE BEGIN C_io_UC_1*/

  /* USER CODE END C_io_UC_1*/

  if (updateflag != 0)
  {
    Custom_STM_App_Update_Char(CUSTOM_STM_C_IO, (uint8_t *)UpdateCharData);
  }

  /* USER CODE BEGIN C_io_UC_Last*/

  /* USER CODE END C_io_UC_Last*/
  return;
}

void Custom_C_io_Send_Notification(void) /* Property Notification */
{
  uint8_t updateflag = 0;

  /* USER CODE BEGIN C_io_NS_1*/
  
  /* USER CODE END C_io_NS_1*/

  if (updateflag != 0)
  {
    Custom_STM_App_Update_Char(CUSTOM_STM_C_IO, (uint8_t *)NotifyCharData);
  }

  /* USER CODE BEGIN C_io_NS_Last*/

  /* USER CODE END C_io_NS_Last*/

  return;
}

/* USER CODE BEGIN FD_LOCAL_FUNCTIONS*/

/*
 * @brief Send a midi note event
 * 
 * @param state         NOTE_ON or NOTE_OFF
 * @param channel       Midi event channel number from 0 to 15
 * @param note          Note from 0 to 127 (21 == A2, 33 == A1)
 * @param velocity      Velocity of the played note from 0 to 127
 */
void Midi_Send_Note(const uint8_t state, const uint8_t channel, const uint8_t note, const uint8_t velocity)
{
  /*  13-bit millisecond-resolution timestamps is used to express the render 
   *  time and event spacing of MIDI messages. But in our case for simplicity
   *  we will send each event at the same timestamp as it has almost no noticable
   *  differences 
   */
  NotifyCharData[0] = 0b10000000; /* Dummy timestamp part1 */
  NotifyCharData[1] = 0b10000001; /* Dummy timestamp part2 */
  NotifyCharData[2] = state | channel;
  /* Next bytes are masked to be sure there are only 7 bits used */
  NotifyCharData[3] = (note & 0x7F);
  NotifyCharData[4] = (velocity & 0x7F);       
  
  Custom_STM_App_Update_Char(CUSTOM_STM_C_IO, (uint8_t *)NotifyCharData);
}



/* USER CODE END FD_LOCAL_FUNCTIONS*/
