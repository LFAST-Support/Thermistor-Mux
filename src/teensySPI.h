#include <avr/io.h>

#ifndef TEENSYSPI_H
#define    TEENSYSPI_H

void initSPI();
void setADCInternalTemp();
void setThermistorMux();
uint32_t read_ADCDATA();

#endif
