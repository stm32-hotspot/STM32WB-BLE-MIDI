/**
  ******************************************************************************
  * @file    simple_midi_parser.c
  * @author  MCD Application Team
  * @brief   Midi file parsing
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
#include "simple_midi_parser.h"

/* Private typedef -----------------------------------------------------------*/
enum MetaEventName
{
  MetaSequence            = 0x00,
  MetaText                = 0x01,
  MetaCopyright           = 0x02,
  MetaTrackName           = 0x03,
  MetaInstrumentName      = 0x04,
  MetaLyrics              = 0x05,
  MetaMarker              = 0x06,
  MetaCuePoint            = 0x07,
  MetaChannelPrefix       = 0x20,
  MetaEndOfTrack          = 0x2F,
  MetaSetTempo            = 0x51,
  MetaSMPTEOffset         = 0x54,
  MetaTimeSignature       = 0x58,
  MetaKeySignature        = 0x59,
  MetaSequencerSpecific   = 0x7F,
};

/* Private defines -----------------------------------------------------------*/ 
#define MIDI_FILE_HEADER        (0x4d546864U)
#define MIDI_CHUNCK_HEADER      (0x4d54726BU)

#define AFTER_TOUCH             (0xA0U)
#define CONTROL_CHANGE          (0xB0U)
#define PROGRAM_CHANGE          (0xC0U)
#define CHANNEL_PRESSURE        (0xD0U)
#define PITCH_BEND              (0xE0U)
#define SYSTEM_EXCLUSIVE        (0xF0U)

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void    Rev_Memcpy( uint8_t *dst, const uint8_t *src, size_t n );
static uint8_t Read32(uint32_t* dst,uint8_t *src);
static uint8_t Read16(uint16_t* dst,uint8_t *src);
static uint8_t ReadValue(uint32_t* dst,uint8_t *src);
static void    ReadString(uint8_t* buffer, uint8_t* src, uint32_t nLength);

uint8_t ParseMidi(uint8_t* flash, uint8_t* trackname, uint16_t* ticks_per_beat, 
                  uint32_t* tempo, Midi_Note_Event_t* song, uint16_t* index);

/* Functions Definition ------------------------------------------------------*/

/* Start of utils functions ------------------------------------------------- */

/*
 * @brief Copy n bytes from source to destination, the MSO of the src become the LSO of the dst
 *
 * @param dst   pointer to the destination buffer
 * @param src   pointer to the source buffer
 * @param n     size in bytes to copy
 */
static void Rev_Memcpy(uint8_t *dst, const uint8_t *src, size_t n )
{
  uint8_t i;
  for(i = 0; i < n; i++)
  {
    dst[n - 1 - i] = src[i];
  }
}

/*
 * @brief Read a 32bit data from midi file buffer
 *
 * @param dst   pointer to the destination buffer
 * @param src   pointer to the source buffer
 *
 * @retval      number of bytes readed
 */
static uint8_t Read32(uint32_t* dst,uint8_t *src)
{
  Rev_Memcpy((uint8_t*)dst, src, sizeof(uint32_t));
  return sizeof(uint32_t);
}

/*
 * @brief Read a 16bit data from midi file buffer
 *
 * @param dst   pointer to the destination buffer
 * @param src   pointer to the source buffer
 *
 * @retval      number of bytes readed
 */
static uint8_t Read16(uint16_t* dst,uint8_t *src)
{
  Rev_Memcpy((uint8_t*)dst, src, sizeof(uint16_t));
  return sizeof(uint16_t);
}

/*
 * @brief Read a variable length data from midi file buffer
 *
 * @param dst   pointer to the destination buffer
 * @param src   pointer to the source buffer
 *
 * @retval      number of bytes readed
 */
static uint8_t ReadValue(uint32_t* dst, uint8_t *src)
{
  uint32_t value = 0;
  uint8_t byte = 0;
  uint8_t length = 0;
  
  value = (uint8_t)(*src++);
  length++;
  if(value & 0x80)
  {
    value &= 0x7F;
    do
    {
      byte = *(src++);
      value = (value<<7) | (byte & 0x7F);
      length++;
    } while (byte & 0x80);
  }
  *dst = value; 
  /* return the readed length */   
  return length;
}

/*
 * @brief Read a string from midi file buffer
 *
 * @param dst   pointer to the destination buffer
 * @param src   pointer to the source buffer
 * @param length number of bytes to read
 *
 * @retval      number of bytes readed
 */
static void ReadString(uint8_t *buffer, uint8_t* src, uint32_t length)
{
  uint32_t i;
  for(i = 0; i < length; i++)
  {
    buffer[i] = src[i];
  }
  buffer[length]='\0';
}

/* End of utils functions --------------------------------------------------- */

/*
 * @brief Parse a midi file. Adding Note On and Note Off events with their deltas to the song buffer.
 * @note  The parser only parse the first two tracks. If there is more tracks they will be ignored.
 * 		  This is done in order to prevent the next track note's to be append to the first track and not in parallel.
 * 		  Midi file used for testing was using the first track for tempo and name info and the second track for notes.
 * @param flash specifies the start address of the file (in file or not as long as this address is accessible)
 * @param trackname is a pointer to a buffer for the trackname string
 * @param ticks_per_beat is a pointer to write the parsed ticks_per_beat
 * @param tempo is a pointer to write the first tempo found in file (unit is microseconds per quarter note)
 * @param song is a buffer to store the events with their deltas (only Note On/Off events are stored)
 * @param index is the index of the first empty element in the buffer. Just after the last event.
 */
uint8_t ParseMidi(uint8_t* flash, uint8_t *trackname, uint16_t* ticks_per_beat, uint32_t* tempo,
                  Midi_Note_Event_t* song, uint16_t* index)
{
  uint32_t m_nTempo = 0;
  uint32_t m_nBPM = 0;
  
  uint8_t buffer[200];
  uint8_t channel;
  uint8_t pressure;
  uint8_t sequence1;
  uint8_t sequence2;
  uint8_t NoteID;
  uint8_t NoteVelocity; 
  uint8_t ControlID;
  uint8_t ControlValue;
  uint8_t ProgramID;
  uint8_t nLS7B;
  uint8_t nMS7B;
  uint8_t prefix;
  uint8_t n;
  uint8_t d;
  uint8_t minor;
  uint8_t key;
  
  /* UNUSED set to prevent build warning depending on dbg traces level macro */
  UNUSED(channel);
  UNUSED(m_nBPM);
  UNUSED(pressure);
  UNUSED(sequence1);
  UNUSED(sequence2);
  UNUSED(NoteID);
  UNUSED(NoteVelocity); 
  UNUSED(ControlID);
  UNUSED(ControlValue);
  UNUSED(ProgramID);
  UNUSED(nLS7B);
  UNUSED(nMS7B);
  UNUSED(prefix);
  UNUSED(n);
  UNUSED(d);
  UNUSED(minor);
  UNUSED(key);
  
  MIDI_PARSER_DBG_MSG_LIGHT("Accessing %p\n\r", flash);
  
  uint32_t string_header;
  flash += Read32(&string_header, flash);
  
  if(string_header == MIDI_FILE_HEADER)
  {
    MIDI_PARSER_DBG_MSG_LIGHT("Midi file detected\n\r");

    uint32_t header_length;
    flash += Read32(&header_length, flash);
    if(header_length != 6)
    {
      MIDI_PARSER_DBG_MSG_LIGHT("Midi file header length shall be 6 bytes not %ld\n\r", header_length);
    }
    else
    {
      uint16_t format, n, division;
      flash += Read16(&format, flash);
      flash += Read16(&n, flash);
      flash += Read16(&division, flash);
      MIDI_PARSER_DBG_MSG_LIGHT("%d tracks at %d ticks per beat\n\r", n, division);
      *ticks_per_beat = division;
      
      for(uint16_t nChunck = 0; nChunck < n && nChunck < 2; nChunck++)
      {
        flash += Read32(&string_header, flash);
        if(string_header != MIDI_CHUNCK_HEADER)
        {
          MIDI_PARSER_DBG_MSG_LIGHT("No track chunck found\n\r");
        }
        else
        {
          MIDI_PARSER_DBG_MSG_LIGHT("======== Track %d ========\n\r",nChunck);
          uint32_t track_length;
          flash += Read32(&track_length, flash);
          MIDI_PARSER_DBG_MSG_LIGHT("Track length = %ld bytes\n\r", track_length);

          uint8_t* trackStart = flash;
          uint8_t trackEnd = 0;
          uint8_t previousStatus;
          
          /*
           * Enter the loop to read the chunck. Until the mandatory end of chunck
           * info is found or if we exceed the length that were indicated for 
           * that chunck
           */
          while(!trackEnd && ((flash - trackStart) < track_length))
          {
            uint32_t delta;
              
            flash += ReadValue(&delta, flash);
            uint8_t status = (*flash++);
            
            MIDI_PARSER_DBG_MSG_FULL("Delta = %d\n\r",delta);
            MIDI_PARSER_DBG_MSG_FULL("Status = %x\n\r",status);
   
            if(status < 0x80)
            {
              /* 
               * In this case the status byte is not here but it is the start 
               * of the event and we have to take in account the last status 
               * byte to interpret this event ( called midi running status )
               */
              status = previousStatus;
              flash--;
            }
            
            switch(status & 0xF0)
            {
              case NOTE_OFF:
              {
                previousStatus = status;
                NoteID = *flash++;
                NoteVelocity = *flash++;
                Midi_Note_Event_t evt;
                evt.Delta = delta;
                evt.Status = status;
                evt.Note = NoteID;
                evt.Velocity = NoteVelocity;
                if((*index) < MAX_EVENTS)
                {
                  song[(*index)++] = evt;
                }
                channel = status & 0x0F;
                MIDI_PARSER_DBG_MSG_FULL("Note OFF, channel %d,note %d, velocity %d\n\r", channel, NoteID, NoteVelocity);
                break;
              }              
              case NOTE_ON:
              {
                previousStatus = status;
                NoteID = *flash++;
                NoteVelocity = *flash++;
                Midi_Note_Event_t evt;
                evt.Delta = delta;
                evt.Status = status;
                evt.Note = NoteID;
                evt.Velocity = NoteVelocity;
                if((*index) < MAX_EVENTS)
                {
                  song[(*index)++] = evt;
                }
                channel = status & 0x0F;
                MIDI_PARSER_DBG_MSG_FULL("Note ON, channel %d,note %d, velocity %d\n\r", channel, NoteID, NoteVelocity);
                break;
              }
                
              case AFTER_TOUCH:
              {
                previousStatus = status;
                channel = status & 0x0F;
                NoteID = *flash++;
                NoteVelocity = *flash++;
                MIDI_PARSER_DBG_MSG_FULL("After touch, channel %d,note %d, velocity %d\n\r", channel, NoteID, NoteVelocity);
                break;
              }
                
              case CONTROL_CHANGE:
              {
                previousStatus = status;
                channel = status & 0x0F;
                ControlID = *flash++;
                ControlValue = *flash++;
                MIDI_PARSER_DBG_MSG_FULL("Control change, channel %d,ID %d, value %d\n\r", channel, ControlID, ControlValue);
                break;
              }
                
              case PROGRAM_CHANGE:
              {
                previousStatus = status;
                channel = status & 0x0F;
                ProgramID = *(flash++);
                MIDI_PARSER_DBG_MSG_FULL("Program change, channel %d,ID %d\n\r", channel, ProgramID);
                break;
              }   
              
              case CHANNEL_PRESSURE:
              {
                previousStatus = status;
                channel = status & 0x0F;
                pressure = *flash++;
                MIDI_PARSER_DBG_MSG_FULL("Channel pressure, channel %d,pressure %d\n\r", channel, pressure);
                break;
              } 
              
              case PITCH_BEND:
              {
                previousStatus = status;
                channel = status & 0x0F;
                nLS7B = *flash++;
                nMS7B = *flash++;
                MIDI_PARSER_DBG_MSG_FULL("Pitch bend, channel %d,lsb %d,msb %d\n\r", channel, nLS7B, nMS7B);
                break;
              }
                
              case SYSTEM_EXCLUSIVE:
              {
                previousStatus=0;
                if(status == 0xFF)
                {
                  uint8_t nType = *flash++;
                  uint32_t nLength;
                  flash += ReadValue(&nLength, flash);
                  
                  MIDI_PARSER_DBG_MSG_FULL("nType = %ld , length = %ld\n\r",nType,nLength);
                  
                  switch (nType)
                  {
   
                    case MetaSequence:
                    {
                      sequence1 = *flash++;
                      sequence2 = *flash++;
                      MIDI_PARSER_DBG_MSG_FULL("Sequence Number: %d%d\n\r", sequence1, sequence2);
                      break;
                    }
                    
                    case MetaText:
                      ReadString(buffer, flash, nLength);
                      flash += nLength;
                      MIDI_PARSER_DBG_MSG_FULL("Text: %s\n\r", buffer);
                      break;
                      
                    case MetaCopyright:
                      ReadString(buffer,flash, nLength);
                      flash += nLength;
                      MIDI_PARSER_DBG_MSG_FULL("Copyright: %s\n\r", buffer);
                      break;
                      
                    case MetaTrackName:
                      ReadString(buffer, flash, nLength);
                      flash += nLength;
                      MIDI_PARSER_DBG_MSG_LIGHT("Track Name: %s\n\r", buffer);
                      if(nChunck == 0)
                      {
                        /* For simplicity we only tack the first trackname */
                        memcpy(trackname,buffer,nLength);
                      }
                      break;
                      
                    case MetaInstrumentName:
                      ReadString(buffer, flash, nLength);
                      flash += nLength;
                      MIDI_PARSER_DBG_MSG_FULL("Instrument Name: %s\n\r", buffer);
                      break;
                      
                    case MetaLyrics:
                      ReadString(buffer, flash, nLength);
                      flash += nLength;
                      MIDI_PARSER_DBG_MSG_FULL("Lyrics: %s\n\r", buffer);
                      break;
                      
                    case MetaMarker:
                      ReadString(buffer, flash, nLength);
                      flash += nLength;
                      MIDI_PARSER_DBG_MSG_FULL("Marker: %s\n\r", buffer);
                      break;
                      
                    case MetaCuePoint:
                      ReadString(buffer, flash, nLength);
                      flash += nLength;
                      MIDI_PARSER_DBG_MSG_FULL("Cue: %s\n\r", buffer);
                      break;
                      
                    case MetaChannelPrefix:
                    {
                      prefix = *flash++;
                      MIDI_PARSER_DBG_MSG_FULL("Prefix: %d\n\r", prefix);
                      break;
                    }
                      
                    case MetaEndOfTrack:
                      trackEnd = 1;
                      MIDI_PARSER_DBG_MSG_FULL("End of track \n\r");
                      break;
                      
                    case MetaSetTempo:
                      /* Tempo is in microseconds per quarter note */
                      Rev_Memcpy((uint8_t*)&m_nTempo, flash, 3);
                      flash += 3;
                      if (*tempo == 0)
                      {
                        m_nBPM = (60000000 / m_nTempo);
                        MIDI_PARSER_DBG_MSG_LIGHT("Tempo: %ld(%ld bpm)\n\r", m_nTempo, m_nBPM);
                        *tempo = m_nTempo;
                      }
                      else
                      {
                        MIDI_PARSER_DBG_MSG_FULL("Tempo found (not first one)\n\r");
                      }
                      break;
                      
                    case MetaSMPTEOffset:
                      flash += 5;
                      MIDI_PARSER_DBG_MSG_FULL("SMPTE \n\r");
                      break;
                      
                    case MetaTimeSignature:
                    {
                      n = *flash++;
                      d = (*flash++)*2;
                      MIDI_PARSER_DBG_MSG_FULL("Time Signature: %d/%d\n\r", n, d);
                      MIDI_PARSER_DBG_MSG_FULL("ClocksPerTick: %d\n\r", *flash++);

                      /* A MIDI "Beat" is 24 ticks, so specify how many 32nd notes 
                       * constitute a beat */
                      MIDI_PARSER_DBG_MSG_FULL("32per24Clocks: %d\n\r",*flash++);
                      break;
                    }

                    case MetaKeySignature:
                    {
                      key = *flash++;
                      minor = *flash++;
                      MIDI_PARSER_DBG_MSG_FULL("Key Signature: %d\n\r", key);
                      MIDI_PARSER_DBG_MSG_FULL("Minor Key: %d\n\r", minor);
                      break;
                    }

                    case MetaSequencerSpecific:
                      ReadString(buffer, flash, nLength);
                      flash += nLength;
                      MIDI_PARSER_DBG_MSG_FULL("Sequencer specific: %s\n\r", buffer);
                      break;

                    default:
                      MIDI_PARSER_DBG_MSG_FULL("Unrecognised MetaEvent: %d\n\r", nType);
                  }
                }
                else if(status == 0xF0)
                {
                  uint32_t nLength;
                  flash += ReadValue(&nLength, flash);
                  ReadString(buffer, flash, nLength);
                  flash += nLength;
                  MIDI_PARSER_DBG_MSG_FULL("Sys ex message begin: %s\n\r", buffer);
                }
                else if(status == 0xF7)
                {
                  uint32_t nLength;
                  flash += ReadValue(&nLength, flash);
                  ReadString(buffer, flash, nLength);
                  flash += nLength;
                  MIDI_PARSER_DBG_MSG_FULL("Sys ex message end: %s\n\r",buffer);
                }
                break;
              } /* End of system exclusive case */
              
              default:
                MIDI_PARSER_DBG_MSG_FULL("Unrecognized status byte\n\r");
                break;
            } /* End of status switch */                              
          }
        }
      }
    }
  }
  else
  {
    MIDI_PARSER_DBG_MSG_LIGHT("No Midi file detected : %lx\n\r", string_header);

    return MIDI_PARSING_NO_FILE;
  }
  
  return MIDI_PARSING_DONE;
}
