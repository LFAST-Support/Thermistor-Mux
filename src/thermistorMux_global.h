/*******************************************************************************
Copyright 2021
Steward Observatory Engineering & Technical Services, University of Arizona

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*******************************************************************************/

/**
 * @file vcm_global.h
 * @author Rory Scobie (scobier@arizona.edu)
 * @brief Global definitions used by all files.
 * @version (see VCM_VERSION)
 * @date 2021-03-10
 *
 * @copyright Copyright (c) 2021
 */

#ifndef THERMISTOR_MUX_GLOBAL_H
#define THERMISTOR_MUX_GLOBAL_H


// Overall version of the VCM module
#define THERMISTOR_MUX_VERSION    "1.0 dev 1"

// Overall version of the MQTT messages.  Increment this for any change to
// the messages: added, deleted, renamed, different type, different function.
#define COMMS_VERSION  2

#include <Arduino.h>

// Enable this to display diagnostic messages on the serial port
#define DEBUG

//Only one of these should be defined, based on thermistor value.
#define thermistor_10K
//#define thermistor_2K

//TODO: add TEST flag maybe?

#define TEENSY_4_1

#define production_TEST

#define NUM_MODULES   6
#define MAX_BOARD_ID  (NUM_MODULES-1)

// Display diagnostic messages on serial port if debugging is enabled
#ifdef DEBUG
#define DebugPrint( msg )       Serial.println( msg )
#define DebugPrintNoEOL( msg )  Serial.print( msg )
#else
#define DebugPrint( msg )
#define DebugPrintNoEOL( msg )
#endif

#define TEENSY_VERSION ", Teensy 4.1"

#ifdef DEBUG
  #define DEBUG_VERSION ", DEBUG"
#else
  #define DEBUG_VERSION ""
#endif




#endif