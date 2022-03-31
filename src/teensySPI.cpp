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
 * @file teensySPI.cpp
 * @author Nestor Garcia (Nestor212@email.arizona.edu)
 * @brief Intializes Teensy 4.1 SPI communication with desired pins. 
 * @version INW
 * @date 2022-03-31
 *
 * @copyright Copyright (c) 2022
 */

#include <SPI.h>
//#include <Arduino.h>
#include "teensySPI.h"

#define CS 10
#define MOSI 11
#define MISO 12
#define SCK 13

static SPISettings settingsA(5000000, MSBFIRST, SPI_MODE0);

void initTeensySPI() {

    pinMode(CS, OUTPUT); // Set CS pin to output 
    digitalWrite(CS, HIGH); // Set CS to high

    SPI.setMOSI(MOSI);
    SPI.setMISO(MISO);
    SPI.setSCK(SCK);
    delay(5000);

    SPI.begin();
    SPI.beginTransaction(settingsA);
}



