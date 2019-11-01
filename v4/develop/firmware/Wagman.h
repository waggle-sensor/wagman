// This file is part of the Waggle Platform.Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.For more details on the Waggle project, visit:
// http://www.wa8.gl
#ifndef __H_WAGMAN__
#define __H_WAGMAN__

#include "Device.h"
#include "Time.h"

const byte MEDIA_SD = 0;
const byte MEDIA_EMMC = 1;
const byte MEDIA_INVALID = 255;

const byte RELAY_OFF = 0;
const byte RELAY_ON = 1;
const byte RELAY_TURNING_ON = 2;
const byte RELAY_TURNING_OFF = 3;

extern unsigned int heartbeatCounters[5];
extern DurationTimer startTimer;

struct DateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

namespace Wagman {
void init();

void setRelay(int port, int mode);

unsigned int getCurrent();
unsigned int getCurrent(byte port);
unsigned int getAddressCurrent(byte addr);

unsigned int getVoltage(int port);
unsigned int getThermistor(int port);

void setLEDAnalog(byte led, int level);
void setLEDs(int mode);
void setLED(byte led, bool on);
bool getLED(byte led);
void toggleLED(byte led);

bool getLight(unsigned int *raw);
bool getHumidity(unsigned int *raw, float *hrf);
bool getTemperature(unsigned int *raw, float *hrf);

byte getBootMedia(byte selector);
void setBootMedia(byte selector, byte media);
void toggleBootMedia(byte selector);

void getID(byte id[8]);

bool validPort(byte port);
bool validLED(byte led);
bool validBootSelector(byte selector);

void getTime(time_t &time);
void setTime(const time_t &time);

void getDateTime(DateTime &dt);
void setDateTime(const DateTime &dt);

void deviceKilled(Device &device);

void setWireEnabled(bool enabled);
bool getWireEnabled();
};  // namespace Wagman

#endif
