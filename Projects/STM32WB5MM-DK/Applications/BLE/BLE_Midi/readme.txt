/**
  @page BLE_Midi Application

  @verbatim
  ******************************************************************************
  * @file    BLE/BLE_Midi/readme.txt 
  * @author  MCD Application Team
  * @brief   Description of the BLE_Midi application
  ******************************************************************************
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @endverbatim

@par Application Description

This application act as a MIDI controller. The controller can send events to a MIDI over BLE Compatible device
(this application was tested with : Apple Garage Band on iOS, SynprezFM on Android). 
Note that this is a proof of concept and demonstrate a simple working implementation of MIDI over BLE but is 
not a fully MIDI Specification compliant library.   

@par Keywords

Connectivity, BLE, Sensors, IPCC, HSEM, RTC, UART, PWR, BLE protocol, BLE profile, Dual core

@par Directory contents 
  
  - BLE/BLE_Midi/Core/Inc/app_common.h               Header for all modules with common definition
  - BLE/BLE_Midi/Core/Inc/app_conf.h                 Parameters configuration file of the application
  - BLE/BLE_Midi/Core/Inc/app_debug.h                Header for app_debug.c module
  - BLE/BLE_Midi/Core/Inc/app_entry.h                Parameters configuration file of the application
  - BLE/BLE_Midi/Core/Inc/app_vl53l0x.h              Header for app_vl53l0x.c module
  - BLE/BLE_Midi/Core/Inc/app_midi.h                 Header for app_midi.c module
  - BLE/BLE_Midi/Core/Inc/hw_conf.h                  Configuration file of the HW
  - BLE/BLE_Midi/Core/Inc/hw_if.h                    HW interface
  - BLE/BLE_Midi/Core/Inc/main.h                     Header for main.c module
  - BLE/BLE_Midi/Core/Inc/simple_midi_parser.h       Simple midi parser header
  - BLE/BLE_Midi/Core/Inc/s25fl128s_conf.h           Configuration of the S25FL128S QSPI memory
  - BLE/BLE_Midi/Core/Inc/stm32wb5mm_dk_conf.h       Configuration file of the Discovery board
  - BLE/BLE_Midi/Core/Inc/stm32wbxx_hal_conf.h       HAL configuration file
  - BLE/BLE_Midi/Core/Inc/stm32wbxx_it.h             Interrupt handlers header file
  - BLE/BLE_Midi/Core/Inc/stm32_lpm_if.h             Header for stm32_lpm_if.c module (LP management)
  - BLE/BLE_Midi/Core/Inc/utilities_conf.h           Configuration file of the utilities
  - BLE/BLE_Midi/STM32_WPAN/App/app_ble.h            Header for app_ble.c module
  - BLE/BLE_Midi/STM32_WPAN/App/ble_conf.h           BLE Services configuration
  - BLE/BLE_Midi/STM32_WPAN/App/ble_dbg_conf.h       BLE Traces configuration of the BLE services
  - BLE/BLE_Midi/STM32_WPAN/App/tl_dbg_conf.h        Debug configuration file for stm32wpan transport layer interface
  - BLE/BLE_Midi/Core/Src/vl53l0x/                   Proximity sensor files
  - BLE/BLE_Midi/Core/Src/app_debug.c                Debug capabilities source file for STM32WPAN Middleware
  - BLE/BLE_Midi/Core/Src/app_entry.c                Initialization of the application
  - BLE/BLE_Midi/Core/Src/app_vl53l0x.c              Proximity Application file
  - BLE/BLE_Midi/Core/Src/app_midi.c                 Midi Application file
  - BLE/BLE_Midi/Core/Src/hw_timerserver.c           Timer Server based on RTC 
  - BLE/BLE_Midi/Core/Src/hw_uart.c                  UART Driver
  - BLE/BLE_Midi/Core/Src/main.c                     Main program
  - BLE/BLE_Midi/Core/Src/simple_midi_parser.c       Simple midi parser code
  - BLE/BLE_Midi/Core/Src/stm32wbxx_hal_msp.c        MSP Initialization and de-Initialization code
  - BLE/BLE_Midi/Core/Src/stm32wbxx_it.c             Interrupt handlers
  - BLE/BLE_Midi/Core/Src/stm32_lpm_if.c             Low Power Manager Interface
  - BLE/BLE_Midi/Core/Src/system_stm32wbxx.c         stm32wbxx system source file  
  - BLE/BLE_Midi/STM32_WPAN/App/app_ble.c            BLE Profile implementation
  - BLE/BLE_Midi/STM32_WPAN/App/custom_app.c         MIDI over BLE Interface
  - BLE/BLE_Midi/STM32_WPAN/App/custom_stm.c         MIDI over BLE Service (Custom STM)
  - BLE/BLE_Midi/STM32_WPAN/Target/hw_ipcc.c         IPCC Driver

  
@par Hardware and Software environment

    - This application runs on STM32WB5MM Discovery Kit devices
    - This application was developped with CubeWB package v1.16.0 and CubeMX 6.8.1.
    - STM32WB5MM Discovery Kit board Set-up    
       - Connect the Board to your PC with a USB cable type A to mini-B to ST-LINK connector (USB_STLINK).

@par How to use it ? 

This application requires having the stm32wb5x_BLE_Stack_full_fw.bin binary flashed on the Wireless Coprocessor.
If it is not the case, you need to use STM32CubeProgrammer to load the appropriate binary.
For the detailed procedure to change the
Wireless Coprocessor binary or see following wiki for Hardware setup:
https://wiki.st.com/stm32mcu/wiki/Connectivity:STM32WB_BLE_Hardware_Setup
   
In order to make the program work, you must do the following :
 - Open your toolchain 
 - Rebuild all files and flash the board with the executable file

1. Flash the BLE wireless stack **full** on the STM32WB5MM-DK board.

2. Build the BLE_Midi project and flash the board with it.

3. Load the midi file in the external flash following this instructions :
  
    A) Copy and rename your midi file with the .bin extension. This is because Cube Programmer will filter on file extensions and won’t let you flash a .mid file

    B) Open STM32CubeProgrammer and user the external loader plugin to load your file in the external flash.

 4. **Unplug and replug your board** this is needed to apply the changes in the external flash.

 5. You can now connect with your smartphone (or another MIDI over BLE compliant reveiver) to your board and use it.

Available Wiki pages:
  - https://wiki.st.com/stm32mcu/wiki/Category:Bluetooth_Low_Energy

For more details refer to the Application Note: 
  AN5289 - Building a Wireless application  
 
 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
