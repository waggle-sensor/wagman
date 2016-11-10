#ifndef ACS764_H
#define ACS764_H

#include <Arduino.h>

class ACS764 {
public:
	static const byte AVG_POINTS = 0x10; // 32 points
	static const byte GAIN_RANGE = 0x02; // 15.66 mA per LSB
	static const byte FAULT_LVL  = 0x00;

	void init(byte address);
	void write_reg(byte address, byte reg_add, byte data);
	unsigned int getCurrent(byte addr);
private:
	//communication register
	static const byte ADDR_DATA       = 0x00;
	static const byte ADDR_AVG_POINTS = 0x02;
	static const byte ADDR_GAIN_RANGE = 0x04;
	static const byte ADDR_FAULT_LVL  = 0x06;

	// mA per LSB
	static const unsigned int MILLIAMPS_PER_STEP = 15.66;
	static const unsigned int MIN_STEP = 15;
};
#endif
