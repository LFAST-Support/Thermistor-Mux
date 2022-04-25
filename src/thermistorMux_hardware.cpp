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
 * @file thermistorMux_hardware.cpp
 * @author Nestor Garcia (Nestor212@email.arizona.edu)
 * @brief Enables Device ID pins
 * @version INW
 * @date 2022-04-19
 *
 * @copyright Copyright (c) 2022
 */

#include "thermistorMux_global.h"
#include "thermistorMux_hardware.h"

#define CS 10
#define MOSI 11
#define MISO 12
#define SCK 13

static SPISettings settingsA(5000000, MSBFIRST, SPI_MODE0);

bool initTeensySPI() {

    pinMode(CS, OUTPUT); // Set CS pin to output 
    digitalWrite(CS, HIGH); // Set CS to high

    SPI.setMOSI(MOSI);
    SPI.setMISO(MISO);
    SPI.setSCK(SCK);

    SPI.begin();
    SPI.beginTransaction(settingsA);

    return true;
}


bool hardwareID_init(){

  pinMode(ID_PIN_0,INPUT_PULLUP);
  pinMode(ID_PIN_1,INPUT_PULLUP);
  pinMode(ID_PIN_2,INPUT_PULLUP);
  pinMode(ID_PIN_3,INPUT_PULLUP);
  pinMode(ID_PIN_4,INPUT_PULLUP);

  // Wait for pin inputs to settle
  delay(100);

  // Read the jumpers.  This must only be done once, even if they produce an
  // invalid ID, in order to ensure that they are correctly read.
  // The pins have inverted sense, so the raw values have been reversed.
  int pin0 = digitalRead(ID_PIN_0) ? 0 : 1;
  int pin1 = digitalRead(ID_PIN_1) ? 0 : 1;
  int pin2 = digitalRead(ID_PIN_2) ? 0 : 1;
  int pin3 = digitalRead(ID_PIN_3) ? 0 : 1;
  int pin4 = digitalRead(ID_PIN_4) ? 0 : 1;

  hardware_id = ( pin4 << 4 ) +
                ( pin3 << 3 ) +
                ( pin2 << 2 ) +
                ( pin1 << 1 ) +
                ( pin0 << 0 );

  delay(10000);
  DebugPrintNoEOL("Hardware ID = ");
  DebugPrint(hardware_id);

  // Make sure ID is valid
  if(hardware_id < 0 || hardware_id > MAX_BOARD_ID){
      DebugPrint("invalid board ID detected, check jumpers");
      return false;
  }

  return true; //Success
}

int get_hardware_id() {
  return hardware_id;
}


