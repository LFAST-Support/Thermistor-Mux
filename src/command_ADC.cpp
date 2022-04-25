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
 * @brief Configures ADC registers with desired funtionality settings. Contains all
 * funtions relating to changing ADC settings and gathering data from the ADC.  
 * @version INW
 * @date 2022-03-31
 *
 * @copyright Copyright (c) 2022
 */

#include "command_ADC.h"
#include "thermistorMux_global.h"

#define CS 10

/*
Resistance at 25 degrees C
The beta coefficient of the thermistor (usually 3000-4000)
*/
#ifdef thermistor_10K
    #define THERMISTORNOMINAL 10000
    #define BCOEFFICIENT 2.514458134e-4 // = 1/3977, B = 3997 K
#elif thermistor_2K 
    #define THERMISTORNOMINAL 2200   
    #define BCOEFFICIENT 2.544529262e-4 // = 1/3930, B = 3930 K
#else 
    #error A thermistor value must be defined.
#endif

// temp. for nominal resistance (almost always 25 C = 298.15 K)
#define TEMPERATURENOMINAL 298.15   

/*
COMMAND Byte CMD[7:0]
Device Address(Hard Coded into device) - CMD[7:6] 
Register Address/ Fast COMMAND bits    - CMD[5:2]
COMMAND type                           - CMD[1:0]
*/
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
#define V_REF_MUX_SET 0b10111100 // Multiplexer regiter byte: 0x06, set to read Vref
                                //   1011 : REFIN+
                                //   1100 : REFIN-  
/*
Scan Register & Timer registers not used
OffsetCal & GainCal registers not used
*/

//Data bytes for debugging
static uint32_t temp_data_buff;
//int myFlag = 0;

/*
Initializes ADC with desired settings(defined above). 
*/
bool initADC() {
    
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
    delay(10);

    return true;
}

/*
Set Mux inputs to internal temperature probes.
*/
void  setADCInternalTempRead() {
    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    SPI.transfer(POINT_MUX_WRITE); //Command byte - set register address to 0x06; MUX Register
    SPI.transfer(ADC_TEMP_MUX_SET); //Set Mux register to read internal ADC temp
    delay(1);
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer  
}

/*
Sets Mux inputs to ch0/ch1; thermistors.
*/
void setThermistorMuxRead() {
    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    SPI.transfer(POINT_MUX_WRITE); //Command byte - set register address to 0x06; Mux Register
    SPI.transfer(THERM_MUX_SET); //Set Mux to original settings; CH0 & CH1 inputs
    delay(1);
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer
}

/*
Starts/Restarts conversion to gather new data.
*/
void start_conversion(){
    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    SPI.transfer(START_CONVERSION); //Restart conversion fast command to gather new data. 
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer
}

float read_ADCDATA() {
    digitalWrite(CS, LOW); //Set CS to Low to begin data transfer
    temp_data_buff = SPI.transfer32(0x41000000); //Send read ADC_DATA register, 32 bit command, & saves output(status byte + 24 data bytes) on a uint32 buffer. 
    digitalWrite(CS, HIGH); //Set CS to high to end data transfer

    /*
    Mask status byte and check for valid data.
    When VIN * Gain > VREF – 1 LSb, the 24-bit ADC code (SGN+DATA[22:0]) will saturate and be locked at
    0x7FFFFF. When VIN * Gain < -VREF, the 24-bit ADC code will saturate and be locked at 0x800000. (pg 42 ADC data sheet)
    */
    if (((temp_data_buff & 0x00FFFFFF) == 0x007FFFFF) || ((temp_data_buff & 0x00FFFFFF) == 0x00800000)){ 
        Serial.printf("Invalid temperature data.\n");
    }
    /*
    Reads status of Mux register to determine source of output data. 
    Output structure;0xXX(status byte)XX(Mux register read data)
    0x1701: Mux register inputs are thermistors
    0x17DE: Mux register inputs are internal temp probes. 
    If/else statement then sends data to appropriate conversion function. 
    */
    else {
        digitalWrite(CS, LOW);//Set CS to Low to begin data transfer
        uint16_t MUX_REG_STATUS = SPI.transfer16(0x5900);
        digitalWrite(CS, HIGH); //Set CS to high to end data transfer

        //Mask Status byte, ensure only data is sent to conversion functions. 
        temp_data_buff = (temp_data_buff & 0x00FFFFFF);
        if(MUX_REG_STATUS == 0x1701) {
            return convert_thermistor_temp(temp_data_buff);
        }
        else if(MUX_REG_STATUS == 0x17DE) {
            return convert_internal_temp(temp_data_buff);
        }
        else {
            Serial.println("Invalid data return.");
            return(0);
        }
    } 
    return(0);
}

/**
Datasheet tranfer equation is for V_ref = 3.3 V & Gain = 1.
    Temp (C) = [0.00133 * ADCDATA(LSB)] - 267.146
We are implementing V_ref = 2.4 V & Gain = 2.
    Temp (C) = [0.00133 * (V_ref/3.3V) * (ADCDATA(LSB))] - 267.146
**/
float convert_internal_temp(uint32_t masked_internal_data) {

     //Two's Complement conversion for negative ADC output data.
    if(((masked_internal_data & 0x00FFFFFF) >> 23) == 1) {
        masked_internal_data = -((masked_internal_data ^ 0xFFFFFF) + 1);
    }
    //ADC internal temp tranfer function for V_ref = 2.4V & Gain = 1  
    float ADCtemp_Celsius = (0.00133 * (2.4/3.3) * (masked_internal_data)) - 267.146; 
    //float ADCtemp_Farenheit = (ADCtemp_Celsius * (1.8)) + 32; // Celsius to Farenheit conversion
     return(ADCtemp_Celsius);
}

/**
Steinhart-Hart B coefficient provided in data sheet:
https://www.tme.eu/Document/32a31570f1c819f9b3730213e5eca259/TT7-10KC3-11.pdf

    Simplified B parameter Steinhart-Hart equation:

    1/T = (1/T_o) + (1/B)*ln(R/R_o) 

    T = measured temperature (Kelvin)
    T_o = room temperature (25 C = 298.15 K)
    B = Beta Constant provided in data sheet
    R = measured resistance (thermistance)
    R_o = resistance at room temperature (10K or 2.2K ohms)
**/
float convert_thermistor_temp(uint32_t masked_therm_data){
    float ADC_output_voltage;
    float thermistance;

    //Two's Complement conversion for negative ADC output data.
    if(((temp_data_buff & 0x00FFFFFF) >> 23) == 1) { 
        masked_therm_data = -((masked_therm_data ^ 0xFFFFFF) + 1);
    }    
   
    //Converts ADC DATA output to measured voltage 
    ADC_output_voltage  = (2.33 / pow(2,23)) * masked_therm_data;
    //Voltage divider, solving for measured thermistace
    thermistance = (ADC_output_voltage*10000)/(2.33 - ADC_output_voltage);
    float stein_temp_Celsius = (1/((1/TEMPERATURENOMINAL) + BCOEFFICIENT*log(thermistance/THERMISTORNOMINAL))) - 272.15;
    //float stein_temp_Farenheit = (stein_temp_Celsius * (1.8)) + 32; 

    return(stein_temp_Celsius);
}
