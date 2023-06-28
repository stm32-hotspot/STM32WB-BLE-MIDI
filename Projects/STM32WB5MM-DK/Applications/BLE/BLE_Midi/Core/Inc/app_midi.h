/**
  ******************************************************************************
  * @file    app_midi.h 
  * @author  MCD Application Team
  * @brief   Header for app_midi.c module
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
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_MIDI_H
#define __APP_MIDI_H

/* Includes ------------------------------------------------------------------*/

/* Defines -------------------------------------------------------------------*/
#define MAX_EVENTS              (2000U)

/* Exported functions ------------------------------------------------------- */
void MIDI_Init(void);
void Midi_Button_Switch_Mode(void);
void Midi_Button_Restart(void);
void Midi_Start_Measures(void);
void Midi_Stop_Measures(void);

#endif /* __APP_MIDI_H */

