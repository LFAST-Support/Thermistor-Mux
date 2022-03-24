#include <avr/io.h>

#ifndef TEENSYSPI_H
#define    TEENSYSPI_H

void initSPI();
uint32_t readInternalTemp();
uint32_t read_ADCDATA();

#endif
