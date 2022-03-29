#include "set_ADC.h"
#include "SPI.h"

/*
Command Byte CMD[7:0]
Device Address(Hard Coded into device) - CMD[7:6] 
Register Address/ Fast command bits    - CMD[5:2]
Command type                           - CMD[1:0]
*/
#define CS 10

#define RESTART 0b0110100
#define STANDBY 0b01101100

#define POINT_CONFIG0_WRITE 0b01000110 //Command byte: Incremental write starting at Config0 register
                                //      01 : Device address
                                //    0001 : Register address; Config0
                                //      10 : Incremental write; starting at register 0x1   
#define POINT_MUX_WRITE 0b01011010 //Command byte; Incremental write starting at Mux register
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
#define CONFIG2_SET 0b10010011  // Config2 register byte: 0x03
                                //     10 : Channel current x 1
                                //    010 : Gain x 2
                                //      1 : Analog input multiplexer auto-zeroing algorithm enabled
                                //     11 : Reserved = '11'
#define CONFIG3_SET 0b10000100  // Config3 register byte: 0x04
                                //     10 : One-shot conversion or one-shot cycle in SCAN mode. It sets ADC_MODE[1:0] to ‘10’ (standby) at
                                //          the end of the conversion or at the end of the conversion cycle in SCAN mode.
                                //     01 : 24-bit (default ADC coding): 24-bit ADC data. It does not allow overrange (ADC code locked to
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
                                //      1 : Enable Fast Commands in the COMMAND Byte
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

/*
Scan Register & Timer registers not used
OffsetCal & GainCal registers not used??
*/

//Status bytes for debugging
byte status_byte1;
byte status_byte2;
byte status_byte3;


static SPISettings settingsA(20000000, MSBFIRST, SPI_MODE0);

void initADC() {
    
    Serial.begin(9600); //For debugging

    SPI.begin();
    SPI.beginTransaction(settingsA);
    digitalWrite(CS, LOW); // Set CS to Low to begin data transfer
    delay(5);

    //ADC offers incremental write feature, after one register is written, moves on to
    //the next in the incremental write loop. (see figure 6-3 of ADC datasheet).
    status_byte1 = SPI.transfer(POINT_CONFIG0_WRITE); //Send ADC Command byte w/ adress or fast command
    delay(3000);
    SPI.transfer(CONFIG0_SET);
    delay(1);
    SPI.transfer(CONFIG1_SET);
    delay(1);
    SPI.transfer(CONFIG2_SET);
    delay(1);
    SPI.transfer(CONFIG3_SET);
    delay(1);
    SPI.transfer(IRQ_SET);
    delay(1);
    SPI.transfer(THERM_MUX_SET);
    delay(1);
    
    SPI.transfer(IRQ_READ);
    Serial.print("Clear flags with IRQ read: ");
    Serial.println(SPI.transfer(0));

    digitalWrite(CS, HIGH); //Set CS to high to end data transfer
    SPI.endTransaction();
    SPI.end();
    delay(5000);
    Serial.print("Status byte 1: ");
    Serial.println(status_byte1);
}

void  setADCInternalTempRead() {

    SPI.begin();
    SPI.beginTransaction(settingsA);
    digitalWrite(CS, LOW);
    delay(5);

    status_byte2 = SPI.transfer(POINT_MUX_WRITE); //Command byte - set register address to 0x06; MUX Register
    SPI.transfer(ADC_TEMP_MUX_SET); //Set Mux register to read internal ADC temp
    SPI.transfer(RESTART); //Restart ADC
    delay(10);
    Serial.print("Status byte 2: ");
    Serial.println(status_byte2);

    digitalWrite(CS, HIGH);
    SPI.endTransaction();
    SPI.end();
}

void setThermistorMux() {

    SPI.begin();
    SPI.beginTransaction(settingsA);
    digitalWrite(CS, LOW);
    delay(5);

    SPI.transfer(POINT_MUX_WRITE); //Command byte -  set register address to 0x06; Mux Register
    SPI.transfer(THERM_MUX_SET); //Set Mux to original settings; CH0 & CH1 inputs
    SPI.transfer(RESTART); //Restart ADC
    delay(1);

    digitalWrite(CS, HIGH);
    SPI.endTransaction();
    SPI.end();
}

 uint32_t read_ADCDATA() {

    uint32_t data_buff;

    SPI.begin();
    SPI.beginTransaction(settingsA);
    digitalWrite(CS, LOW);
    delay(5);

    status_byte3 = SPI.transfer(ADCDATA_READ);
    //SPI.transfer(RESTART);
    delay(10);
    data_buff = SPI.transfer32(0);
   
    Serial.print("Status byte 3: ");
    Serial.println(status_byte3);
    Serial.print("data = ");
    Serial.println(data_buff);
    
    digitalWrite(CS, HIGH);
    SPI.endTransaction();
    SPI.end();
    return data_buff;
}