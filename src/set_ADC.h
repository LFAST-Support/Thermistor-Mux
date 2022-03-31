/*******************************************************************************
Copyright 2021
Steward Observatory Engineering & Technical Services, University of Arizona
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*******************************************************************************/

/**
 * @file set_ADC.h
 * @author Nestor Garcia (Nestor212@email.arizona.edu)
 * @brief ADC specific definitions and function prototypes for ADC initializtion, changes and 
 * data gathering and analysis.  
 * @version INW
 * @date 2022-03-31
 *
 * @copyright Copyright (c) 2022
 */

#include <avr/io.h>

#ifndef ADC_H
#define ADC_H


void initADC();
void setADCInternalTempRead();
void setThermistorMuxRead();
void read_ADCDATA();
void convert_internal_temp(uint32_t);
void convert_thermistor_temp(uint32_t);


#endif