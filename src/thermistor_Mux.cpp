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

#include "teensySPI.h"
#include "set_ADC.h"
#include "thermistorMux_network.h"
#include "thermistorMux_hardware.h"
#include "thermistorMux_global.h"

/*
Questions:
1) What kind of network will this be connected to? MQTT
2) How to set/change skew?
3) 
*/

/*
typedef struct thermistor_data {
    int mosfetRef;
    float cal_data;
    float uncal_temp;
    float cal_temp = uncal_temp + cal_data;
} thermData; 

#define RESET_DATA thermistor_temp[32] = NULL; ADC_internal_temp = 0.0;
*/
   
#define CS 10
#define INTERRUPT_PIN 23

/*
Array representing 32 Mosfets
mosfet[0] = header pin 0; mosfet Q1
mosfet[1] = header pin 1; mosfet Q2
...
mosfet[31] = header pin 22; mosfet Q32
*/

static float cal_data[32];

static unsigned int mosfet[32] = {0,1,2,3,4,5,6,7,8,9,24,25,26,27,28,29,30,31,
                                  32,36,37,40,41,14,15,16,17,18,19,20,21,22};


volatile int irqFlag = 0;
void IRQ() {
  irqFlag = 1;
}

void setup() {
  Serial.begin(9600); //For debugging

  //MOSFET digital control I/O ports, set to output. All MOSFETS turned off (pins set to LOW).
  for (int mosfetRef = 0; mosfetRef < 32; mosfetRef++) {
    pinMode(mosfet[mosfetRef], OUTPUT);  
    digitalWrite(mosfet[mosfetRef], LOW);
  }
  //INW: figure out how to set skew

  /*
  Enable global interrupts. 
  Set up ADC interrupt feature on teensy pin 23. 
  Upon recieving an interrupt from ADC(indicating new data is available in ADC),
  IRQ flag is triggered.
  */
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), IRQ, FALLING);
  sei();

  initTeensySPI(); //Enable teensy command of ADC chip
  initADC(); //Set up ADC with desired configuration
  hardwareID_init();
}

void loop() {
  int mosfetRef;
  int avgCount = 0;
  float thermistor_temp[32] = {0.00};
  float ADC_internal_temp = 0;

  //Cycle through mofets; setting digital control pin high, calls on 
  //function that sets mux register to read thermistor inputs.
  //Takes average of 10 data values for each mosfet & internal temp, then resets data buffers.
  while(avgCount < 10) {
  setThermistorMuxRead();
  delay(1);
  for(mosfetRef = 0; mosfetRef < 32; mosfetRef++) {
    digitalWrite(mosfet[mosfetRef], HIGH);
    start_conversion();
    
    while (irqFlag == 0) {
      delay(1); //Wait for interrupt 
    }
    irqFlag = 0;

   if(thermistor_temp[mosfetRef] == 0.00) {
      thermistor_temp[mosfetRef] = read_ADCDATA();
      //Serial.println("Initial value");
    }
    else {
      thermistor_temp[mosfetRef] = (thermistor_temp[mosfetRef] + read_ADCDATA()) / (2); 
      //erial.println("Running average");
    }
    digitalWrite(mosfet[mosfetRef], LOW);
  }
  //Calls on function that sets mux register to read internal ADC temperature. 
  setADCInternalTempRead();
  delay(1);
  start_conversion();
  
  while (irqFlag == 0) {
    delay(1); //Wait for interrupt
  }
  irqFlag = 0;

  if (ADC_internal_temp == 0) {
    ADC_internal_temp = read_ADCDATA();
  }
  else {
    ADC_internal_temp = (ADC_internal_temp + read_ADCDATA()) / (2);
  }
  avgCount++;
  }

  Serial.printf("Internal ADC temperature: %0.2f C\n", ADC_internal_temp);
  for (mosfetRef = 0; mosfetRef < 32; mosfetRef++){
    Serial.printf("Thermistor %d temperature:%0.2f C\n",mosfetRef + 1, thermistor_temp[mosfetRef]);
  }
  Serial.println();
  publish_data(thermistor_temp, ADC_internal_temp);
}





