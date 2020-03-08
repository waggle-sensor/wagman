#include "ACS764.h"
#include <Wire.h>

void ACS764::init(byte address)
{
	write_reg(address, ADDR_AVG_POINTS, AVG_POINTS & 0xFF);
	write_reg(address, ADDR_GAIN_RANGE, GAIN_RANGE & 0x03);
	write_reg(address, ADDR_FAULT_LVL, FAULT_LVL   & 0x0F);
}

void ACS764::write_reg(byte address, byte reg_add, byte data)
{
	for (byte attempts = 0; attempts < 10; attempts++) {
		Wire.beginTransmission(address);
		Wire.write(reg_add);
		Wire.write(0x00);
		Wire.write(0x00);
		Wire.write(data);
		if (Wire.endTransmission() == 0)
			break;
	}
}

unsigned int ACS764::getCurrent(byte addr)
{
	byte csb, lsb;
	byte attempts;
	byte timeout;

	for (attempts = 0; attempts < 10; attempts++) {

		/* request data from sensor */
		Wire.beginTransmission(addr);
		delay(5);
		Wire.write(ADDR_DATA);
		delay(5);

		/* retry on error */
		if (Wire.endTransmission(0) != 0)
		continue;

		delay(5);

		/* read data from sensor */
		if (Wire.requestFrom(addr, 3) != 3)
		continue;

		delay(5);

		for (timeout = 0; timeout < 100 && Wire.available() < 3; timeout++) {
			delay(5);
		}

		/* retry on error */
		if (timeout >= 100)
			continue;

		Wire.read();
		csb = Wire.read() & 0x01;
		lsb = Wire.read();

		/* retry on error */
		if (Wire.endTransmission(1) != 0)
			continue;

		delay(5);

		/* return milliamps from raw sensor data. */
		if (((csb << 8) | lsb) < MIN_STEP)
			return 0;
		else
			return ((csb << 8) | lsb) * MILLIAMPS_PER_STEP;
	}
	return 0xFFFF;
}