#ifndef __H_WAGMAN__
#define __H_WAGMAN__

#include "Time.h"
#include "Device.h"

const byte MEDIA_SD = 0;
const byte MEDIA_EMMC = 1;
const byte MEDIA_INVALID = 255;

const byte RELAY_OFF = 0;
const byte RELAY_ON = 1;
const byte RELAY_TURNING_ON = 2;
const byte RELAY_TURNING_OFF = 3;

extern volatile byte heartbeatCounters[5]; // BAD! Clean this up!
extern Timer startTimer;

struct DateTime
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

namespace Wagman
{
    void init();

    void setRelay(int port, int mode);

    byte getHeartbeat(byte port);

    unsigned int getCurrent();
    unsigned int getCurrent(byte port);
    unsigned int getAddressCurrent(byte addr);

    unsigned int getThermistor(byte port);

    void setLEDAnalog(byte led, int level);
    void setLEDs(int mode);
    void setLED(byte led, bool on);
    bool getLED(byte led);
    void toggleLED(byte led);

    float getHumidity();
    float getTemperature();

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
};

#endif
