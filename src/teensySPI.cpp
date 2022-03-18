#include <SPI.h>
#include <Arduino.h>
#include "teensySPI.h"

void initSPI() {

    SPISettings settingsA(16000000, LSBFIRST, SPI_MODE0);
  
    SPI.setCS(10);
    pinMode(10, OUTPUT); // Set CS pin to output 
    digitalWrite(10, HIGH); // Set CS to high

    SPI.setMOSI(11);
    SPI.setMISO(12);
    SPI.setSCK(13);

    SPI.begin();    
}
