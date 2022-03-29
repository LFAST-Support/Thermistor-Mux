#include <avr/io.h>

#ifndef ADC_H
#define ADC_H


void initADC();
void setADCInternalTempRead();
void setThermistorMuxRead();
uint32_t read_ADCDATA();


#endif