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
 * @file thermistor_Mux.cpp
 * @author Nestor Garcia (Nestor212@email.arizona.edu)
 * @brief Main file, complete code cycles through 32 mosfets; each conencted to a thermistor,
 * and uses ADC module MC3561R to convert analog to digital data. Internal temperature data of
 * ADC chip is also gathered. 
 * @version INW
 * @date 2022-03-31
 *
 * @copyright Copyright (c) 2022
 */

//#include <Arduino.h>
#include "teensySPI.h"
#include "network.h"
#include "set_ADC.h"
#include <avr/interrupt.h>
#include <SPI.h>
//#include <InternalTemperature.h>

/*
Questions:
1) What kind of network will this be connected to? MQTT
2) How to set/change skew?
3) 
*/

#define CS 10
#define RAW_DATA 23
#define SOME_CALCULATION 0
#define INTERRUPT_PIN 23

/*
Array representing 32 Mosfets
mosfet[0] = header pin 0; mosfet Q1
mosfet[1] = header pin 1; mosfet Q2
...
mosfet[31] = header pin 22; mosfet Q32
*/
const int mosfet[32] = {0,1,2,3,4,5,6,7,8,9,24,25,26,27,28,29,30,31,
                        32,36,37,40,41,14,15,16,17,18,19,20,21,22};

void setup() {

  Serial.begin(9600); //For debugging

  /*
  Enable global interrupts. 
  Set up ADC interrupt feature on teensy pin 23. 
  Upon recieving an interrupt from ADC(indicating new data is available in ADC),
  function is called to read data from ADC Data register.
  */
  sei(); 
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), *read_ADCDATA, CHANGE);
  
  // MOSFET digital control I/O ports, set to output. All MOSFETS turned off (pins set to LOW)
  // Slowest slew rate?? How
  for (int mosfetRef = 0; mosfetRef < 32; mosfetRef++) {
    pinMode(mosfet[mosfetRef], OUTPUT);  
    digitalWrite(mosfet[mosfetRef], LOW);
  }
  //INW: figure out how to set skew

  initTeensySPI(); //Enable teensy command of ADC chip
  initADC(); //Set up ADC with desired configuration
  //initNetwork(); //Ethernet enable
}

void loop() {

    /*
    Cycle through mofets; setting digital control pin high, calls on function that
    sets mux register to read thermistor inputs. 
    */
    for(int mosfetRef = 0; mosfetRef < 1; mosfetRef++) {
      digitalWrite(mosfet[mosfetRef], HIGH);
      //Serial.print("Thermistor ");
      //Serial.print(mosfetRef + 1);
      //Serial.print(": ");
      setThermistorMuxRead();
      delay(100);
      digitalWrite(mosfet[mosfetRef], LOW);


    }
    /*
    Calls on function that sets mux register to reat internal ADC temperature. 
    */
    setADCInternalTempRead();
    delay(100);
}



