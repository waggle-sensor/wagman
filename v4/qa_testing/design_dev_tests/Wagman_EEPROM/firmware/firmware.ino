#include <Wire.h>    
 
#define disk1 0x50    //Address of 24LC256 eeprom chip
unsigned int address = 0;
void setup(void)
{
  SerialUSB.begin(115200);
  Wire.begin();   
  writeEEPROM(disk1, address, 123);
  SerialUSB.print(readEEPROM(disk1, address), DEC);
}
 
void loop(){
 SerialUSB.println(readEEPROM(disk1, address), DEC);
  writeEEPROM(disk1, address, address);
  SerialUSB.println(readEEPROM(disk1, address), DEC);
  address = address + 1;
  delay(1000);
  
  }
 
void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data ) 
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
 
  delay(5);
}
 
byte readEEPROM(int deviceaddress, unsigned int eeaddress ) 
{
  byte rdata = 0xFF;
 
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,1);
 
  if (Wire.available()) rdata = Wire.read();
 
  return rdata;
}
