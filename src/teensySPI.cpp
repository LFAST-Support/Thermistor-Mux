#include <SPI.h>
#include <Arduino.h>
#include "teensySPI.h"

#define CS 10
#define MOSI 11
#define MISO 12
#define SCK 13

#define CONFIG_ADC 00000000 //Some configuration byte; refer to datasheet



void initSPI() {

    SPISettings settingsA(16000000, LSBFIRST, SPI_MODE0);
  
    SPI.setCS(CS);
    pinMode(CS, OUTPUT); // Set CS pin to output 
    digitalWrite(CS, HIGH); // Set CS to high

    SPI.setMOSI(MOSI);
    SPI.setMISO(MISO);
    SPI.setSCK(SCK);

    SPI.begin();

    SPI.beginTransaction(settingsA);

    digitalWrite(CS, LOW); // Set CS to Low to begin data transfer
    SPI.transfer(CONFIG_ADC); //Send some ADC command byte
    digitalWrite(CS, HIGH); // Set CS to high to end data transfer

    SPI.endTransaction();
}
