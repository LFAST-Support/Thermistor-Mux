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
#define CONFIG2_SET 0b10010111  // Config2 register byte: 0x03
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
    /*
    digitalWrite(CS, LOW);
    SPI.transfer(START_CONVERSION);
    digitalWrite(CS, HIGH);
    */
    delay(3000);
}

/*
Sets Mux inputs to internal temperature probes, then restarts conversion to gather new data.
*/
void  setADCInternalTempRead() {

    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    //delay(5);
    SPI.transfer(POINT_MUX_WRITE); //Command byte - set register address to 0x06; MUX Register
    SPI.transfer(ADC_TEMP_MUX_SET); //Set Mux register to read internal ADC temp
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer
    //delay(5);
    
    
    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    //delay(5);
    SPI.transfer(START_CONVERSION); //Restart conversion fast command to gather new data. 
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer
    //delay(5); 
}

/*
Sets Mux inputs to ch0/ch1; thermistors, then restarts conversion to gather new data.
*/
void setThermistorMuxRead() {

    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    //delay(5);
    SPI.transfer(POINT_MUX_WRITE); //Command byte - set register address to 0x06; Mux Register
    SPI.transfer(THERM_MUX_SET); //Set Mux to original settings; CH0 & CH1 inputs
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer
    //delay(5);

    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    //delay(5);
    SPI.transfer(START_CONVERSION); //Restart conversion fast command to gather new data. 
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer
    //delay(5);
}

/* 

*/
 void read_ADCDATA() {

    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    temp_data = SPI.transfer32(0x41000000); //Send read ADC_DATA register, 32 bit command, & saves output(status byte + 24 data bytes) on a variable. 
    //Serial.println(temp_data);
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer

    /*
    Reads status of Mux register to determine source of output data. 
    Output structure;0xXX(status byte)XX(Mux register read data)
    0x1701: Mux register inputs are thermistors
    0x17DE: Mux register inputs are internal temp probes. 
    If/else statement then sends data to appropriate conversion function. 
    */
    digitalWrite(CS, LOW);//Set CS to Low to begin data transfer
    uint16_t MUX_REG_STATUS = SPI.transfer16(0x5900);
    if(MUX_REG_STATUS == 0x1701) {
        convert_thermistor_temp(temp_data);
    }
    else if(MUX_REG_STATUS == 0x17DE) {
        convert_internal_temp(temp_data);
    }
    else {
        Serial.println("Invalid data return");
    }
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer
    delay(1000);
}

/*
INW: Will take 32 bit output from ADC, extract 24 bits of temperature data, and 
covert it to temperature value.  
*/

/*
Datasheet tranfer equation is for V_ref = 3.3 V & Gain = 1.
    Temp (C) = [0.00133 * ADCDATA(LSB)] - 267.146
We are implementing V_ref = 2.4 V & Gain = 2.
    Temp (C) = [0.00133 * (V_ref/3.3V) * (ADCDATA(LSB)/2)] - 267.146
*/
void convert_internal_temp(uint32_t temp_data) {

    float temp_Celsius = (temp_data & 0x000FFFFF);
    temp_Celsius = (0.00133 * (2.4/3.3) * (temp_Celsius/2)) - 267.147;
    float temp_Farenheit = (temp_Celsius * (9/5)) + 32;

    Serial.printf("Internal ADC temp: %0.2f C = %0.2f F\n",temp_Celsius, temp_Farenheit);
}

/*
INW: Will take 32 bit output from ADC, extract 24 bits of temperature data, and 
covert it to temperature value.  
*/
void convert_thermistor_temp(uint32_t temp_data){
    Serial.printf("Mosfet 1 temp data: %d \n",temp_data);
}
