/**
  ******************************************************************************
  * @file    app_midi.c
  * @author  MCD Application Team
  * @brief   Midi application file
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

/* Includes ------------------------------------------------------------------*/
#include "app_common.h"
#include "dbg_trace.h"
#include "stm32_seq.h"
#include "stm32_lcd.h"
#include "stm32wb5mm_dk_lcd.h"
#include "app_vl53l0x.h"
#include "custom_app.h"

#include "simple_midi_parser.h"
#include "app_midi.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint8_t               Check_Distance_Timer_Id;        /*!< Distance measurements CB timer id */
  uint8_t               Midi_Seq_Timer_Id;              /*!< Sequencer CB timer id */
  uint8_t               run; 				/*!< Player mode status (0 not running , else running) */
  Midi_Note_Event_t     song[MAX_EVENTS];               /*!< Array of Midi NoteOn or Off events with their deltas */
  uint16_t              index;				/*!< Index of the first empty event of the song */
  uint16_t              cpt;				/*!< Current index in the song */
  uint16_t              ticks_per_beat;		        /*!< Ticks per beat */
  uint32_t              tempo;				/*!< Tempo in microseconds per quarter note */
  uint8_t               trackname[100];                 /*!< Track name buffer passed to the parser */        
  uint64_t              songLength;			/*!< Song lenth in ticks */
  uint64_t              currentLength;		        /*!< Cumulated length to the current event in ticks */
  uint8_t               distance;			/*!< ToF sensor distance in cm */
} Midi_App_Context_t;

/* Private defines -----------------------------------------------------------*/ 
#define DK_EXTERNAL_FLASH_ADDRESS (uint8_t *)(0x90000000U)

#define LCD_CHAR_WIDTH          (18U)

#define BASE_NOTE               (50U)

#define MEASUREMENTS_PERIOD     (100U)
/* Private variables ---------------------------------------------------------*/
static Midi_App_Context_t Midi_App_Context;

/* Private function prototypes -----------------------------------------------*/
static uint8_t IsNotEmpty(char* str);

static void    Check_distance_cb(void);
static void    Check_distance(void);
static void    Midi_seq_cb(void);
static void    Midi_seq(void);
static void    Update_progress_bar(void);

/* Functions Definition ------------------------------------------------------*/
void MIDI_Init()
{
  BSP_LCD_Clear(0, SSD1315_COLOR_BLACK);
  UTIL_LCD_DisplayStringAt(0, 0, (uint8_t *)"WB BLE MIDI", CENTER_MODE);
  UTIL_LCD_DisplayStringAt(0, LINE(2), (uint8_t *)"Parsing file...", LEFT_MODE);
  BSP_LCD_Refresh(0);
  
  Midi_App_Context.index = 0;
  uint8_t* flash_address = DK_EXTERNAL_FLASH_ADDRESS;
  uint8_t status = ParseMidi(flash_address, Midi_App_Context.trackname,
                             &Midi_App_Context.ticks_per_beat, &Midi_App_Context.tempo,
                             Midi_App_Context.song, &Midi_App_Context.index);
  
  UTIL_LCD_ClearStringLine(2);
  if(status != MIDI_PARSING_NO_FILE)
  {
    if(IsNotEmpty((char*)Midi_App_Context.trackname))
    {
      UTIL_LCD_DisplayStringAt(0, LINE(2), (uint8_t *)Midi_App_Context.trackname, LEFT_MODE);
    }
    else
    {
      UTIL_LCD_DisplayStringAt(0, LINE(2), (uint8_t *)"No track name", LEFT_MODE);
    }

    UTIL_LCD_DisplayStringAt(0, LINE(4), (uint8_t *)"|>  ", RIGHT_MODE);
    UTIL_LCD_DisplayStringAt(0, LINE(4), (uint8_t *)"  |<<", LEFT_MODE);
  }
  else
  {
    UTIL_LCD_DisplayStringAt(0, LINE(2), (uint8_t *)"No midi file found", LEFT_MODE);    
  }
  BSP_LCD_Refresh(0);
  
  Midi_App_Context.currentLength = 0;
  Midi_App_Context.songLength = 0;
  uint32_t i;
  for(i = 0; i < Midi_App_Context.index; i++)
  {
    Midi_App_Context.songLength += Midi_App_Context.song[i].Delta;
  } 
  
  /* Task and timer for the distance measurement */
  UTIL_SEQ_RegTask(1<<CFG_TASK_CHECK_DISTANCE, UTIL_SEQ_RFU, Check_distance);
  HW_TS_Create(CFG_TIM_PROC_ID_ISR,
        &Midi_App_Context.Check_Distance_Timer_Id,
        hw_ts_Repeated,
        Check_distance_cb);
  
  /* Task and timer for the midi sequencer */
  UTIL_SEQ_RegTask(1<<CFG_TASK_MIDI_SEQ, UTIL_SEQ_RFU, Midi_seq);
  HW_TS_Create(CFG_TIM_PROC_ID_ISR,
        &Midi_App_Context.Midi_Seq_Timer_Id,
        hw_ts_SingleShot,
        Midi_seq_cb);
  
  return;
}

/*
 * @brief Check if a buffer is not a string composed of nothing or only non printable characters
 *
 * @param str pointer to the string to check 
 *
 * @retval 0 if not empty
 */
static uint8_t IsNotEmpty(char* str)
{
  int i = 0;
  while(str[i] != '\0')
  {
    /* White space = 32, so check that there is not only whitespace or displayable characters */
    if(str[i] > 32 && str[i] < 127)
      return 1;
    i++;
  }
  return 0;
}

/*
 * @brief Timer callback to set the distance check task
 */
static void Check_distance_cb(void)
{
 UTIL_SEQ_SetTask(1<<CFG_TASK_CHECK_DISTANCE, CFG_SCH_PRIO_0); 
}

/*
 * @brief If something is in the TOF send a notification
 */
static void Check_distance(void)
{
  /* If sequencer should be running disable hadn playing */
  /* We still let the timer run (not optimum way to do it) */
  if(!Midi_App_Context.run)
  {
    static uint32_t prevTick;
    static uint8_t ongoingNote;
    uint32_t tick = HAL_GetTick();
    const uint16_t debounce = 400;
    uint16_t prox_value = 0;
    prox_value = VL53L0X_PROXIMITY_GetDistance();
    Midi_App_Context.distance = prox_value / 10;
    if( Midi_App_Context.distance < 100)
    {
      float average = 0.0;
      uint8_t valid=0;
      if(tick - prevTick > debounce)
      {
        if(!ongoingNote)
        {
          for(int i=0;i<4;i++)
          {
            uint16_t measure = VL53L0X_PROXIMITY_GetDistance();
            if(measure > 0 && measure < 1200)
            {
                average += measure;
                valid += 1;
            }
          }
          average /= valid;
          Midi_App_Context.distance = (uint8_t)(average / 10);
          if(Midi_App_Context.distance != 0)
          {
            APP_DBG_MSG("Send : %d\n\r", Midi_App_Context.distance);
            uint8_t note_offset = BASE_NOTE + Midi_App_Context.distance / 2;
            Midi_Send_Note(NOTE_ON, 0, note_offset, 127);
            HAL_Delay(100);
            Midi_Send_Note(NOTE_OFF, 0, note_offset, 127);
          }
          ongoingNote = 1;
        }
        prevTick=tick;
      }
    }
    else
    {
      ongoingNote = 0;
    }
  }
  
  return;
}

/*
 * @brief Update the progress bar on the LCD screen
 * @note  Will overwrite what was on the 3rd line.
 */
static void Update_progress_bar(void)
{
  /* LCD Progress bar */
  uint8_t progress = (uint8_t)(((float)Midi_App_Context.currentLength / (float)Midi_App_Context.songLength) * 100);
  char progressBar[LCD_CHAR_WIDTH];
  progressBar[0] = '[';
  progressBar[LCD_CHAR_WIDTH-1] = ']';
  uint8_t i;
  for(i = 1; i < LCD_CHAR_WIDTH - 1; i++)
  {
    if(progress > ((float)(i - 1) / (float)(LCD_CHAR_WIDTH - 2) * 100))
    {
      progressBar[i] = '=';
    }
    else
    {
      progressBar[i] = ' ';
    }
  }
  UTIL_LCD_ClearStringLine(3);
  UTIL_LCD_DisplayStringAt(0, LINE(3), (uint8_t *)progressBar, LEFT_MODE);
  BSP_LCD_Refresh(0);
  
  return;
}

/*
 * @brief Timer callback to set the sequencer task
 */
static void Midi_seq_cb(void)
{
  UTIL_SEQ_SetTask(1<<CFG_TASK_MIDI_SEQ, CFG_SCH_PRIO_0); 
  
  return;
}

/*
 * @brief Play an event and recursively trigger himself for the next event at the the next delta time
 */
static void Midi_seq(void)
{
  /* If sequencer should be running */
  if(Midi_App_Context.run)
  {
    /* If not at the end of the song */
    if(Midi_App_Context.cpt < Midi_App_Context.index)
    {
      Midi_App_Context.currentLength += Midi_App_Context.song[Midi_App_Context.cpt].Delta;
      Update_progress_bar();
    
      Midi_Note_Event_t evt = Midi_App_Context.song[Midi_App_Context.cpt];
      Midi_Send_Note((evt.Status & 0xF0), (evt.Status & 0x0F), evt.Note, evt.Velocity);
      
      APP_DBG_MSG("Midi event : status %x note %d velocity %d\n\r",evt.Status, evt.Note, evt.Velocity);
      
      Midi_App_Context.cpt++;
      
      if(Midi_App_Context.song[Midi_App_Context.cpt].Delta != 0)
      {
        /* Program the sequencer to send the next event after next delta */
        uint32_t delta_us = Midi_App_Context.tempo * Midi_App_Context.song[Midi_App_Context.cpt].Delta / Midi_App_Context.ticks_per_beat;
        HW_TS_Start(Midi_App_Context.Midi_Seq_Timer_Id, (delta_us / CFG_TS_TICK_VAL));
      }
      else
      {
        /* In case delta is null between events we need to quickly send the next one to
         * prevent excessive latency so we call recursively this function directly 
         * without using it by setting the task */ 
        Midi_seq();
      }
    }
  }
  
  return;
}

/*
 * @brief Switch between pause and play
 */
void Midi_Button_Switch_Mode(void)
{
  Midi_App_Context.run = ~Midi_App_Context.run;
  if(Midi_App_Context.run)
  {
    UTIL_LCD_DisplayStringAt(0, LINE(4), (uint8_t *)"||  ", RIGHT_MODE);
  }
  else
  {
    UTIL_LCD_DisplayStringAt(0, LINE(4), (uint8_t *)"|>  ", RIGHT_MODE);
  }
  BSP_LCD_Refresh(0);
  UTIL_SEQ_SetTask(1<<CFG_TASK_MIDI_SEQ, CFG_SCH_PRIO_0); 
  
  return;
}

/*
 * @brief Restart the midi player at the beginning
 */
void Midi_Button_Restart(void)
{
  Midi_App_Context.cpt = 0;
  Midi_App_Context.currentLength = 0;
  Update_progress_bar();
  if(Midi_App_Context.run)
  {
    UTIL_SEQ_SetTask(1<<CFG_TASK_MIDI_SEQ, CFG_SCH_PRIO_0); 
  }
  
  return;
}

/*
 * @brief Start the periodic distance measurement and check
 */
void Midi_Start_Measures(void)
{
  HW_TS_Start(Midi_App_Context.Check_Distance_Timer_Id,MEASUREMENTS_PERIOD);
  
  return;
}

/*
 * @brief Stop the periodic distance measurement and check
 */
void Midi_Stop_Measures(void)
{
  HW_TS_Stop(Midi_App_Context.Check_Distance_Timer_Id);
  
  return;
}