#include <Wire.h>

//MCP7941x I2C Addresses
#define RTC_ADDR 0x6F
#define EEPROM_ADDR 0x57

//MCP7941x Register Addresses
#define TIME_REG 0x00        //7 registers, Seconds, Minutes, Hours, DOW, Date, Month, Year
#define DAY_REG 0x03         //the RTC Day register contains the OSCON, VBAT, and VBATEN bits
#define YEAR_REG 0x06        //RTC year register
#define CTRL_REG 0x07        //control register
#define CALIB_REG 0x08       //calibration register
#define UNLOCK_ID_REG 0x09   //unlock ID register
#define ALM0_REG 0x0A        //alarm 0, 6 registers, Seconds, Minutes, Hours, DOW, Date, Month
#define ALM1_REG 0x11        //alarm 1, 6 registers, Seconds, Minutes, Hours, DOW, Date, Month
#define ALM0_DAY 0x0D        //DOW register has alarm config/flag bits
#define PWRDWN_TS_REG 0x18   //power-down timestamp, 4 registers, Minutes, Hours, Date, Month
#define PWRUP_TS_REG 0x1C    //power-up timestamp, 4 registers, Minutes, Hours, Date, Month
#define TIMESTAMP_SIZE 8     //number of bytes in the two timestamp registers
#define SRAM_START_ADDR 0x20 //first SRAM address
#define SRAM_SIZE 64         //number of bytes of SRAM
#define EEPROM_SIZE 128      //number of bytes of EEPROM
#define EEPROM_PAGE_SIZE 8   //number of bytes on an EEPROM page
#define UNIQUE_ID_ADDR 0xF0  //starting address for unique ID
#define UNIQUE_ID_SIZE 8     //number of bytes in unique ID

#if defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#include <TinyWireM.h>
#define i2cBegin TinyWireM.begin
#define i2cBeginTransmission TinyWireM.beginTransmission
#define i2cEndTransmission TinyWireM.endTransmission
#define i2cRequestFrom TinyWireM.requestFrom
#define i2cRead TinyWireM.receive
#define i2cWrite TinyWireM.send

#elif ARDUINO >= 100
#include <Wire.h>
#define i2cBegin Wire.begin
#define i2cBeginTransmission Wire.beginTransmission
#define i2cEndTransmission Wire.endTransmission
#define i2cRequestFrom Wire.requestFrom
#define i2cRead Wire.read
#define i2cWrite Wire.write

#else
#include <Wire.h>
#define i2cBegin Wire.begin
#define i2cBeginTransmission Wire.beginTransmission
#define i2cEndTransmission Wire.endTransmission
#define i2cRequestFrom Wire.requestFrom
#define i2cRead Wire.receive
#define i2cWrite Wire.send

#endif

byte uniqueID[UNIQUE_ID_SIZE];

void idRead(void)
{
    i2cBeginTransmission(EEPROM_ADDR);
    i2cWrite(UNIQUE_ID_ADDR);
    i2cEndTransmission();
    i2cRequestFrom( EEPROM_ADDR, UNIQUE_ID_SIZE );
    for (byte i=0; i<UNIQUE_ID_SIZE; i++)
    {
        uniqueID[i] = i2cRead();
        SerialUSB.print(uniqueID[i],HEX);
    }
}


void setup() {
    Wire.begin();        // join i2c bus (address optional for master)
    SerialUSB.begin(115200);  // start serial for output
}

void loop() {
    SerialUSB.print("MCP7941x uniqueID:");
    idRead();
    SerialUSB.println('\r');
    delay(1000);
}