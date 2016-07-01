/*
 HTU21D Humidity Sensor Library
 By: Nathan Seidle
 SparkFun Electronics
 Date: September 22nd, 2013
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

 This library allows an Arduino to read from the HTU21D low-cost high-precision humidity sensor.

 If you have feature suggestions or need support please use the github support page: https://github.com/sparkfun/HTU21D

 Hardware Setup: The HTU21D lives on the I2C bus. Attach the SDA pin to A4, SCL to A5. If you are using the SparkFun
 breakout board you *do not* need 4.7k pull-up resistors on the bus (they are built-in).

 Link to the breakout board product:

 Software:
 Call HTU21D.Begin() in setup.
 HTU21D.ReadHumidity() will return a float containing the humidity. Ex: 54.7
 HTU21D.ReadTemperature() will return a float containing the temperature in Celsius. Ex: 24.1
 HTU21D.SetResolution(byte: 0b.76543210) sets the resolution of the readings.
 HTU21D.check_crc(message, check_value) verifies the 8-bit CRC generated by the sensor
 HTU21D.read_user_register() returns the user register. Used to set resolution.
 */

#include <Wire.h>
#include "HTU21D.h"

HTU21D::HTU21D()
{
  //Set initial values for private vars
}

//Begin
/*******************************************************************************************/
//Start I2C communication
void HTU21D::begin(void)
{
  return;
}

//Read the humidity
/*******************************************************************************************/
//Calc humidity and return it to the user
//Returns 998 if I2C timed out
//Returns 999 if CRC is wrong
float HTU21D::readHumidity(void)
{
    byte timeout;

	//Request a humidity reading
	Wire.beginTransmission(HTDU21D_ADDRESS);
	Wire.write(TRIGGER_HUMD_MEASURE_NOHOLD); //Measure humidity with no bus holding
	Wire.endTransmission();

	//Hang out while measurement is taken. 50mS max, page 4 of datasheet.
	delay(55);

	//Comes back in three bytes, data(MSB) / data(LSB) / Checksum
	Wire.requestFrom(HTDU21D_ADDRESS, 3);

	//Wait for data to become available
	timeout = 0;
   
	while (Wire.available() < 3)
	{
		delay(1);

        if(timeout++ > 100) {
            return 998;
        }
	}

	byte msb, lsb, checksum;
	msb = Wire.read();
	lsb = Wire.read();
	checksum = Wire.read();

	/* //Used for testing
	byte msb, lsb, checksum;
	msb = 0x4E;
	lsb = 0x85;
	checksum = 0x6B;*/

	unsigned int rawHumidity = ((unsigned int) msb << 8) | (unsigned int) lsb;

	if(check_crc(rawHumidity, checksum) != 0) return(999); //Error out

	//sensorStatus = rawHumidity & 0x0003; //Grab only the right two bits
	rawHumidity &= 0xFFFC; //Zero out the status bits but keep them in place

	//Given the raw humidity data, calculate the actual relative humidity
	float tempRH = rawHumidity / (float)65536; //2^16 = 65536
	float rh = -6 + (125 * tempRH); //From page 14

	return(rh);
}

//Read the temperature
/*******************************************************************************************/
//Calc temperature and return it to the user
//Returns 998 if I2C timed out
//Returns 999 if CRC is wrong
float HTU21D::readTemperature(void)
{
	//Request the temperature
	Wire.beginTransmission(HTDU21D_ADDRESS);
	Wire.write(TRIGGER_TEMP_MEASURE_NOHOLD);
	Wire.endTransmission();

	//Hang out while measurement is taken. 50mS max, page 4 of datasheet.
	delay(55);

	//Comes back in three bytes, data(MSB) / data(LSB) / Checksum
	Wire.requestFrom(HTDU21D_ADDRESS, 3);

	//Wait for data to become available
	int counter = 0;
	while(Wire.available() < 3)
	{
		counter++;
		delay(1);
		if(counter > 100) return 998; //Error out
	}

	unsigned char msb, lsb, checksum;
	msb = Wire.read();
	lsb = Wire.read();
	checksum = Wire.read();

	/* //Used for testing
	byte msb, lsb, checksum;
	msb = 0x68;
	lsb = 0x3A;
	checksum = 0x7C; */

	unsigned int rawTemperature = ((unsigned int) msb << 8) | (unsigned int) lsb;

	if(check_crc(rawTemperature, checksum) != 0) return(999); //Error out

	//sensorStatus = rawTemperature & 0x0003; //Grab only the right two bits
	rawTemperature &= 0xFFFC; //Zero out the status bits but keep them in place

	//Given the raw temperature data, calculate the actual temperature
	float tempTemperature = rawTemperature / (float)65536; //2^16 = 65536
	float realTemperature = (float)(-46.85 + (175.72 * tempTemperature)); //From page 14

	return(realTemperature);
}

//Set sensor resolution
/*******************************************************************************************/
//Sets the sensor resolution to one of four levels
//Page 12:
// 0/0 = 12bit RH, 14bit Temp
// 0/1 = 8bit RH, 12bit Temp
// 1/0 = 10bit RH, 13bit Temp
// 1/1 = 11bit RH, 11bit Temp
//Power on default is 0/0

void HTU21D::setResolution(byte resolution)
{
  byte userRegister = read_user_register(); //Go get the current register state
  userRegister &= B01111110; //Turn off the resolution bits
  resolution &= B10000001; //Turn off all other bits but resolution bits
  userRegister |= resolution; //Mask in the requested resolution bits

  //Request a write to user register
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(WRITE_USER_REG); //Write to the user register
  Wire.write(userRegister); //Write the new resolution bits
  Wire.endTransmission();
}

//Read the user register
byte HTU21D::read_user_register(void)
{
  byte userRegister;

  //Request the user register
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(READ_USER_REG); //Read the user register
  Wire.endTransmission();

  //Read result
  Wire.requestFrom(HTDU21D_ADDRESS, 1);

  userRegister = Wire.read();

  return(userRegister);
}

//Give this function the 2 byte message (measurement) and the check_value byte from the HTU21D
//If it returns 0, then the transmission was good
//If it returns something other than 0, then the communication was corrupted
//From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
//POLYNOMIAL = 0x0131 = x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
#define SHIFTED_DIVISOR 0x988000 //This is the 0x0131 polynomial shifted to farthest left of three bytes

byte HTU21D::check_crc(uint16_t message_from_sensor, uint8_t check_value_from_sensor)
{
  //Test cases from datasheet:
  //message = 0xDC, checkvalue is 0x79
  //message = 0x683A, checkvalue is 0x7C
  //message = 0x4E85, checkvalue is 0x6B

  uint32_t remainder = (uint32_t)message_from_sensor << 8; //Pad with 8 bits because we have to add in the check value
  remainder |= check_value_from_sensor; //Add on the check value

  uint32_t divsor = (uint32_t)SHIFTED_DIVISOR;

  for (int i = 0 ; i < 16 ; i++) //Operate on only 16 positions of max 24. The remaining 8 are our remainder and should be zero when we're done.
  {
    //Serial.print("remainder: ");
    //Serial.println(remainder, BIN);
    //Serial.print("divsor:    ");
    //Serial.println(divsor, BIN);
    //Serial.println();

    if( remainder & (uint32_t)1<<(23 - i) ) //Check if there is a one in the left position
      remainder ^= divsor;

    divsor >>= 1; //Rotate the divsor max 16 times so that we have 8 bits left of a remainder
  }

  return (byte)remainder;
}

