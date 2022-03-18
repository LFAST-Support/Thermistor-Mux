#include <Arduino.h>
#include "teensySPI.h"
#include "network.h"
//#include <InternalTemperature.h>

/*
Questions:
1) What kind of network will this be connected to?
2) ADC to average 16 reading; is that referring to the ADC chip itself?
3) 

*/


#define RAW_DATA 23
#define SOME_CALCULATION 0


// Array representing 32 Mosfets
// mosfet[0] = header pin 0; mosfet Q1
// mosfet[1] = header pin 1; mosfet Q2
// ...
// mosfet[31] = header pin 22; mosfet Q32
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
  //INW: figure out how to set skew


  initSPI(); //Enable teensy command of ADC chip
  initNetwork(); //Ethernet enable

  //Set analog parameters
  analogReadResolution(12); // 10-bits is default, 12 is max.  
  analogReadAveraging(16);  //Set ADC to average 16 reading?

}

void loop() {

  float rawVoltage;
  float temp;
  
    //Cycle through mofets
    for(int mosfetRef = 0; mosfetRef < 32; mosfetRef++) {
      digitalWrite(mosfet[mosfetRef], HIGH);
      delay(100);
      rawVoltage = analogRead(RAW_DATA);
      temp = SOME_CALCULATION; // TBD
      digitalWrite(mosfet[mosfetRef], LOW);
      //Print data to serial monitor for debugging 
      Serial.print("Mosfet ");
      Serial.println(mosfetRef + 1);
      Serial.print("Temp = ");
      Serial.println(temp);
      Serial.print("Voltage = ");
      Serial.println(rawVoltage);

      //INW: Print to a file or directory of some sort, need to find out what sort of network this will be connected to.

    }

//Convert raw analog input into temp; refer to appropriate datasheet of thermistor
}
