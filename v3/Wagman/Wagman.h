#include "Time.h"

const byte MEDIA_SD = 0;
const byte MEDIA_EMMC = 1;
const byte MEDIA_INVALID = 255;

const byte RELAY_OFF = 0;
const byte RELAY_ON = 1;
const byte RELAY_UNKNOWN = 255;

namespace Wagman
{
    void init();
    
    void setRelay(byte port, bool on);
    byte getRelay(byte port);

    byte getHeartbeat(byte port);

    int getCurrent();
    int getCurrent(byte port);

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

    time_t getTime();
    void setTime(int year, int month, int day, int hour, int minute, int second);

    int getAddressCurrent(int addr);
};

