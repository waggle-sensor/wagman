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

namespace Wagman
{
    void init();

    void setRelay(byte port, bool on);
    byte getRelay(byte port);

    byte getHeartbeat(byte port);

    unsigned int getCurrent();
    unsigned int getCurrent(byte port);
    unsigned int getAddressCurrent(byte addr);

    unsigned int getThermistor(byte port);

    void setLED(byte led, bool on);
    bool getLED(byte led);
    void toggleLED(byte led);

    float getHumidity();
    float getTemperature();

    byte getBootMedia(byte selector);
    void setBootMedia(byte selector, byte media);
    void toggleBootMedia(byte selector);

    bool validPort(byte port);
    bool validLED(byte led);
    bool validBootSelector(byte selector);

    void getTime(time_t &time);
    void setTime(byte year, byte month, byte day, byte hour, byte minute, byte second);

    void deviceKilled(Device &device);

    void setWireEnabled(bool enabled);
    bool getWireEnabled();
};

#endif
