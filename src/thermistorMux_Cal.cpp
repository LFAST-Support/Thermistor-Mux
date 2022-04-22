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
 * @file thermistorCal.cpp
 * @author Nestor Garcia (Nestor212@email.arizona.edu)
 * @brief INW
 * @version INW
 * @date 2022-03-31
 *
 * @copyright Copyright (c) 2022
 */

#include "thermistorMux_Cal.h"
#include "thermistorMux_global.h"
#include "set_ADC.h"


float cal_thermistor(float set_temp){
    irqFlag = 0;
    Serial.printf("Set temp is %0.2f, calibration begun.\n", set_temp);
    setThermistorMuxRead();
    delay(1);
    for(int mosfetRef = 0; mosfetRef < 32; mosfetRef++) {
        digitalWrite(mosfet[mosfetRef], HIGH);
        start_conversion();
        
        while (irqFlag == 0) {
          delay(1); //Wait for interrupt 
        }
        irqFlag = 0;

        float actual_temp = read_ADCDATA();
        cal_data[mosfetRef] = set_temp - actual_temp;  

        Serial.printf("Read thermistor temp = %0.2f \nCalculated cal value = %0.2f \n", actual_temp, cal_data[mosfetRef]);
        digitalWrite(mosfet[mosfetRef], LOW);
    }
    Serial.println("Calibration complete.");
    return (0);
}
