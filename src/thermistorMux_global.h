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
 * @file thermistorMux_global.h
 * @author Rory Scobie (scobier@arizona.edu)
 * @brief Global definitions used by all files.
 * Originally created for VCM module, modified by Nestor, for thermistor Mux use
 * @version (see VCM_VERSION)
 * @date 2021-03-10
 *
 * @copyright Copyright (c) 2021
 */

#include <Arduino.h>
#include <math.h>
#include <EEPROM.h>
#include "SPI.h"


#ifndef THERMISTOR_MUX_GLOBAL_H
#define THERMISTOR_MUX_GLOBAL_H

// Overall version of the VCM module
#define THERMISTOR_MUX_VERSION    "1.0 dev 1"

// Overall version of the MQTT messages.  Increment this for any change to
// the messages: added, deleted, renamed, different type, different function.
#define COMMS_VERSION  2

// Enable this to display diagnostic messages on the serial port
#define DEBUG

//Only one of these should be defined, based on thermistor value.
#define thermistor_10K
//#define thermistor_2K

//TODO: add TEST flag maybe?

#define TEENSY_4_1

#define production_TEST

#define NUM_MODULES   32
#define MAX_BOARD_ID  (NUM_MODULES - 1)

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

#define ID_PIN_0 39
#define ID_PIN_1 38
#define ID_PIN_2 35
#define ID_PIN_3 34
#define ID_PIN_4 33

static int hardware_id = -1;

/*
Array representing 32 Mosfets
mosfet[0] = header pin 0; mosfet Q1
mosfet[1] = header pin 1; mosfet Q2
...
mosfet[31] = header pin 22; mosfet Q32
*/
static unsigned int mosfet[32] = {0,1,2,3,4,5,6,7,8,9,24,25,26,27,28,29,30,31,
                                  32,36,37,40,41,14,15,16,17,18,19,20,21,22};                                 
static int irqFlag = 0;
static float cal_data[32];


#endif
