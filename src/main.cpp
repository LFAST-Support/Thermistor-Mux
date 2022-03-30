#include <Arduino.h>
#include "teensySPI.h"
#include "network.h"
#include "set_ADC.h"
#include <avr/interrupt.h>
#include <SPI.h>
//#include <InternalTemperature.h>

/*
Questions:
1) What kind of network will this be connected to? MQTT
2) ADC to average 16 reading; is that referring to the ADC chip itself?
3) 

*/

#define CS 10
#define RAW_DATA 23
#define SOME_CALCULATION 0
#define INTERRUPT_PIN 23


// Array representing 32 Mosfets
// mosfet[0] = header pin 0; mosfet Q1
// mosfet[1] = header pin 1; mosfet Q2
// ...
// mosfet[31] = header pin 22; mosfet Q32
const int mosfet[32] = {0,1,2,3,4,5,6,7,8,9,24,25,26,27,28,29,30,31,
                        32,36,37,40,41,14,15,16,17,18,19,20,21,22};

//uint32_t thermistorData;
uint32_t thermistor_temp;
uint32_t internalADC_temp;

void setup() {

  Serial.begin(9600); //For debugging

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
    //Cycle through mofets
    for(int mosfetRef = 0; mosfetRef < 2; mosfetRef++) {

      Serial.print("Thermistor ");
      Serial.print(mosfetRef + 1);
      Serial.print(": ");
      setThermistorMuxRead();

      digitalWrite(mosfet[mosfetRef], HIGH);
      delay(1000);

      //thermistor_temp = read_ADCDATA() + 1; //INW: convert thermistor raw data to temperature value
      
      digitalWrite(mosfet[mosfetRef], LOW);

      //Serial.println(thermistor_temp);
      //printData("Thermistor Temp:", thermistor_temp);

    }
    Serial.print("Internal ADC temperature: ");
    setADCInternalTempRead();
    
    //internalADC_temp = ((0.00133 * read_ADCDATA()) - 267.146); //Temperature sensor tranfer function(see datasheet eq. 5-1)
    delay(1000);
    //Serial.println(internalADC_temp);;
    //printData("ADC Internal Temp:", internalADC_temp);
    //setThermistorMuxRead()
}



