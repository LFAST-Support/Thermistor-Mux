#include <SPI.h>
#include <Arduino.h>
#include "teensySPI.h"

#define CS 10
#define MOSI 11
#define MISO 12
#define SCK 13

static SPISettings settingsA(5000000, MSBFIRST, SPI_MODE0);

void initTeensySPI() {

    pinMode(CS, OUTPUT); // Set CS pin to output 
    digitalWrite(CS, HIGH); // Set CS to high

    //SPI.setMOSI(MOSI);
    //SPI.setMISO(MISO);
    //SPI.setSCK(SCK);
    delay(5000);

    SPI.begin();
    SPI.beginTransaction(settingsA);
}



