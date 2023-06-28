/**
  ******************************************************************************
  * @file    simple_midi_parser.h 
  * @author  MCD Application Team
  * @brief   Header for simple_midi_parser.c module
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
/* MIDI PARSER DEBUG TRACES LEVEL 
 *      0 - NO TRACES
 *      1 - LIGHT TRACES
 *      2 - FULL TRACES
 */
#define MIDI_PARSING_DEBUG_TRACES 1

#if (MIDI_PARSING_DEBUG_TRACES > 0)
#define MIDI_PARSER_DBG_MSG_LIGHT  APP_DBG_MSG
#else
#define MIDI_PARSER_DBG_MSG_LIGHT  PRINT_NO_MESG
#endif

#if (MIDI_PARSING_DEBUG_TRACES > 1)
#define MIDI_PARSER_DBG_MSG_FULL   APP_DBG_MSG
#else
#define MIDI_PARSER_DBG_MSG_FULL  PRINT_NO_MESG
#endif

#define MAX_EVENTS              (2000U)

#define MIDI_PARSING_DONE       (0U)
#define MIDI_PARSING_NO_FILE    (1U)

#define NOTE_ON                 (0x90U)
#define NOTE_OFF                (0x80U)

/* Exported types ----------------------------------------------------------- */
typedef struct
{
  uint32_t      Delta;
  uint8_t       Status;
  uint8_t       Note;
  uint8_t       Velocity;
} Midi_Note_Event_t;

/* Exported functions ------------------------------------------------------- */
uint8_t ParseMidi(uint8_t* flash, uint8_t* trackname, uint16_t* ticks_per_beat, 
                  uint32_t* tempo, Midi_Note_Event_t* song, uint16_t* index);

#endif /* __APP_MIDI_H */

