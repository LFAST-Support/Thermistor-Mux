#include <avr/io.h>

#ifndef ADC_H
#define ADC_H


void initADC();
void setADCInternalTempRead();
void setThermistorMuxRead();
void read_ADCDATA();


#endif