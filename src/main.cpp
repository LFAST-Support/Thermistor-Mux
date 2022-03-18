#include <Arduino.h>
#include <iostream>
#include <tuple>
#include "teensySPI.h"
#include "network.h"

// Array representing 32 Mosfets
// mosfet[0] = header pin 0; refDes Q1
// mosfet[1] = header pin 1; refDes Q2
// ...
// mosfet[31] = header pin 22; refDes Q32
const int mosfet[32] = {0,1,2,3,4,5,6,7,8,9,24,25,26,27,28,29,30,31,
                        32,36,37,40,41,14,15,16,17,18,19,20,21,22};


void setup() {
  Serial.begin(9600);


  // MOSFET Control I/O ports, set to Output. All MOSFETS turned off (pins set to LOW)
  // Will probably replace with for loop in "loop" code.
  // Slowest slew rate?? How

  for (int mosfetRef = 0; mosfetRef < 32; mosfetRef++) {
    pinMode(mosfet[mosfetRef], OUTPUT);  
    digitalWrite(mosfet[mosfetRef], LOW);
  }

  //Enable teensy command of ADC chip
  initSPI();
  //Ethernet enable?
  initNetwork();

  //Set analog parameters
  analogReadResolution(12); // 10-bits is default, 12 is max.  
  analogReadAveraging(16);
}

void loop() {
  
   for(int mosfetRef = 0; mosfetRef < 32; mosfetRef++) {
      digitalWrite(mosfet[mosfetRef], HIGH);
      delay(3500);
      float volt = analogRead(23);
      float temp = 0;
      digitalWrite(mosfet[mosfetRef], LOW);
      Serial.print("Mosfet ");
      Serial.println(mosfetRef + 1);
      Serial.print("Temp = ");
      Serial.println(temp);
      Serial.print("Voltage = ");
      Serial.println(volt);
    }
//Convert Analog input into temp; refer to appropriate datasheet of thermistor
}
