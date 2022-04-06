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
 * @file set_ADC.cpp
 * @author Nestor Garcia (Nestor212@email.arizona.edu)
 * @brief Configures ADC registers with desired fuuntionality settings. Contains all
 * funtions relating to changing ADC settings and gathering data from the ADC.  
 * @version INW
 * @date 2022-03-31
 *
 * @copyright Copyright (c) 2022
 */

#include "set_ADC.h"
#include "SPI.h"
#include <math.h>

/*
CONVERSION Byte CMD[7:0]
Device Address(Hard Coded into device) - CMD[7:6] 
Register Address/ Fast CONVERSION bits    - CMD[5:2]
CONVERSION type                           - CMD[1:0]
*/
#define CS 10

#define STANDBY 0b01101100
#define POINT_CONFIG0_WRITE 0b01000110 //Command byte: Incremental write starting at Config0 register
                                //      01 : Device address
                                //    0001 : Register address; Config0
                                //      10 : Incremental write; starting at register 0x1   
#define POINT_MUX_WRITE 0b01011010 //CONVERSION byte; Incremental write starting at Mux register
                                //      01 : Device address
                                //    0110 : Register address; Mux Reg
                                //      10 : Incremental write; starting at register 0x06
#define CONFIG0_SET 0b11100011  //Config0 register byte: 0x01
                                //     11 : Full shutdown mode disable
                                //     10 : Internal digital clock selected; no output
                                //     00 : No current applied to ADC inputs
                                //     11 : ADC Conversion Mode selected
#define CONFIG1_SET 0b00101000  //Config1 register byte: 0x02
                                //     00 : Prescaler AMCLK = MCLK (default)
                                //   1010 : Oversampling ratio; OSR = 20480 (data rate is 60 samples/sec)
                                //     00 : Reserved = '00'
#define CONFIG2_SET 0b10001111  // Config2 register byte: 0x03
                                //     10 : Channel current x 1
                                //    010 : Gain x 2
                                //      1 : Analog input multiplexer auto-zeroing algorithm enabled
                                //     11 : Reserved = '11'
#define CONFIG3_SET 0b10000000  // Config3 register byte: 0x04
                                //     10 : One-shot conversion or one-shot cycle in SCAN mode. It sets ADC_MODE[1:0] to ‘10’ (standby) at
                                //          the end of the conversion or at the end of the conversion cycle in SCAN mode.
                                //     00 : 24-bit (default ADC coding): 24-bit ADC data. It does not allow overrange (ADC code locked to
                                //          0xFFFFFF or 0x800000).
                                //      0 : 16-bit wide (CRC-16 only) (default)
                                //      0 : CRC on communications disabled (default)
                                //      0 : Digital offset cal disabled (default)
                                //      0 : Digital gain cal diabled (default)
#define IRQ_SET 0b00000010      // IRQ: Interrupt request register byte: 0x05
                                //      x : Unimplemented, read as '0'
                                //      x : ADCDATA has not been updated since last reading or last Reset (default)
                                //      x : CRC error has not occurred for the Configuration registers (default)
                                //      x : POR has not occurred since the last reading (default)
                                //      0 : IRQ output is selected. All interrupts can appear on the IRQ/MDAT pin.
                                //      0 : The Inactive state is high-Z (requires a pull-up resistor to DV_DD) (default)
                                //      1 : Enable Fast Commands in the command Byte
                                //      0 : Disable Conversion Start Interrupt Output
#define THERM_MUX_SET 0b00000001 // Multiplexer regiter byte: 0x06, set to thermistor output data
                                //   0000 : CH0 = MUX_VIN+ Input  
                                //   0001 : Ch1 = MUX_VIN- Input
#define ADC_TEMP_MUX_SET 0xDE // Multiplexer regiter byte: 0x06, set to internal temperature data
                                //   1101 : Internal temp diode P 
                                //   1110 : Internal temp diode M                                 
#define ADCDATA_READ 0b01000001 //Command byte: Read ADC Conversion Data
                                //      01 : Device address
                                //    0000 : Register address 
                                //      01 : Static Read   
#define IRQ_READ 0b01010101 //Command byte: Read IRQ register data
                                //      01 : Device address
                                //    0101 : Register address 
                                //      01 : Static Read   
#define START_CONVERSION 0b01101000 
/*
Scan Register & Timer registers not used
OffsetCal & GainCal registers not used??
*/

//Data bytes for debugging
volatile uint32_t temp_data;
int myFlag = 0;

/*
Initializes ADC with desired settings(defined above). 
*/
void initADC() {
    
    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    //ADC offers incremental write feature, after one register is written, moves on to
    //the next in the incremental write loop. (see figure 6-3 of ADC datasheet).
    SPI.transfer(POINT_CONFIG0_WRITE); //ADC Command byte; Incremental write starting at reg 0x01
    SPI.transfer(CONFIG0_SET);
    SPI.transfer(CONFIG1_SET);
    SPI.transfer(CONFIG2_SET);
    SPI.transfer(CONFIG3_SET);
    SPI.transfer(IRQ_SET);
    SPI.transfer(THERM_MUX_SET);
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer
}

/*
Sets Mux inputs to internal temperature probes, then restarts conversion to gather new data.
*/
void  setADCInternalTempRead() {

    cli();
    myFlag = 0;
    sei();

    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    SPI.transfer(POINT_MUX_WRITE); //Command byte - set register address to 0x06; MUX Register
    SPI.transfer(ADC_TEMP_MUX_SET); //Set Mux register to read internal ADC temp
    delay(1);
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer  
    
    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    SPI.transfer(START_CONVERSION); //Restart conversion fast command to gather new data. 
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer

    while( myFlag == 0 ) { 
        //Serial.println("No ADC interrupt."); //Do Nothing
        delay(1);
    }
}

/*
Sets Mux inputs to ch0/ch1; thermistors, then restarts conversion to gather new data.
*/
void setThermistorMuxRead() {

    cli();
    myFlag = 0;
    sei();

    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    SPI.transfer(POINT_MUX_WRITE); //Command byte - set register address to 0x06; Mux Register
    SPI.transfer(THERM_MUX_SET); //Set Mux to original settings; CH0 & CH1 inputs
    delay(1);
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer

    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    SPI.transfer(START_CONVERSION); //Restart conversion fast command to gather new data. 
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer

    while( myFlag == 0 ) { 
       // Serial.println("No mux interrupt."); //Do Nothing
       delay(1);
    }
}

 void read_ADCDATA() {

    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    temp_data = SPI.transfer32(0x41000000); //Send read ADC_DATA register, 32 bit command, & saves output(status byte + 24 data bytes) on a variable. 
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer

    if (((temp_data & 0x00FFFFFF) >> 1) == 0x007FFFFF) {
        Serial.print("Invalid temperature data: ");
    }
    /*
    Status byte generates two consecutive ADC data reads, due to a change in interrupt status bit
    data bytes remain unchanged. 
    Else if statement below limits data to one print. 
    */
    else if ((temp_data & 0xFF000000) == 0x13000000) {
        /*
        Reads status of Mux register to determine source of output data. 
        Output structure;0xXX(status byte)XX(Mux register read data)
        0x1701: Mux register inputs are thermistors
        0x17DE: Mux register inputs are internal temp probes. 
        If/else statement then sends data to appropriate conversion function. 
        */
        digitalWrite(CS, LOW);//Set CS to Low to begin data transfer
        uint16_t MUX_REG_STATUS = SPI.transfer16(0x5900);
        digitalWrite(CS, HIGH); //Set CS to high to end data transfer

        //Mask Status byte, ensure only data is sent to conversion functions. 
        temp_data = (temp_data & 0x00FFFFFF);
        if(MUX_REG_STATUS == 0x1701) {
            convert_thermistor_temp(temp_data);
        }
        else if(MUX_REG_STATUS == 0x17DE) {
            convert_internal_temp(temp_data);
        }
        else {
            Serial.println("Invalid data return.");
        }
    } 
    myFlag = 1;
}

/*
Datasheet tranfer equation is for V_ref = 3.3 V & Gain = 1.
    Temp (C) = [0.00133 * ADCDATA(LSB)] - 267.146
We are implementing V_ref = 2.4 V & Gain = 2.
    Temp (C) = [0.00133 * (V_ref/3.3V) * (ADCDATA(LSB)/2)] - 267.146
*/
void convert_internal_temp(uint32_t masked_internal_data) {

     //Two's Complement conversion for negative ADC output data.
    if(((masked_internal_data & 0x00FFFFFF) >> 23) == 1) {
        masked_internal_data = -((masked_internal_data ^ 0xFFFFFF) + 1);
    }  

    float ADCtemp_Celsius = (0.00133 * (2.4/3.3) * (masked_internal_data)) - 267.147; //ADC internal temp tranfer function for V_ref = 2.4V & Gain = 2
    float ADCtemp_Farenheit = (ADCtemp_Celsius * (1.8)) + 32; // Celsius to Farenheit conversion

    Serial.printf("Internal ADC temperature: %0.2f C = %0.2f F\n", ADCtemp_Celsius, ADCtemp_Farenheit);

    delay(1000); //Delay to slow outflow of data for debugging
}

/*
natural Log best fit equation from data table provided in data sheet:
https://www.tme.eu/Document/32a31570f1c819f9b3730213e5eca259/TT7-10KC3-11.pdf

T(C) = -24.03ln((2.715E-5)*R)
*/
void convert_thermistor_temp(uint32_t masked_therm_data){

    float ADC_output_voltage;
    float thermistance;

     //Two's Complement conversion for negative ADC output data.
    if(((temp_data & 0x00FFFFFF) >> 23) == 1) { 
        masked_therm_data = -((masked_therm_data ^ 0xFFFFFF) + 1);
    }    
   
    //Converts ADC DATA output to measured voltage 
    ADC_output_voltage  = (2.33 / pow(2,23)) * masked_therm_data;

    //Voltage divider, solving for measured thermistace
    thermistance = (ADC_output_voltage*10000)/(2.33 - ADC_output_voltage);

    //Natural log best fit equation developed from graphed data table for 10K thermistor
    float thermistor_temp_Celsius = -24.03*log(thermistance*2.715E-5);
    /*//Quartic best fit equation developed from graphed data table for 10K thermistor
    float thermistor_temp_Celsius = (2.818E-19*pow(thermistance,4)) - (1.982E-13*pow(thermistance, 3)) +
                                    (4.594E-8*pow(thermistance, 2)) - (0.004021 * thermistance) + 84.02);
    */
    //Celsius to Farenheit conversion 
    float thermistor_temp_Farenheit = (thermistor_temp_Celsius * (1.8)) + 32; 

    Serial.printf("temperature: %0.2f C = %0.2f F\n", thermistor_temp_Celsius, thermistor_temp_Farenheit);
    
    //Serial.println((2.818E-19*pow(thermistance,4)) - (1.982E-13*pow(thermistance, 3)) +
    //                (4.594E-8*pow(thermistance, 2)) - (0.004021 * thermistance) + 84.02);

    delay(1000); //Delay to slow outflow of data for debugging
}
