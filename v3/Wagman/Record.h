#include <Arduino.h>

struct Version
{
    byte major;
    byte minor;
};

struct Range
{
    int min;
    int max;
};

struct SensorStatus
{
    byte health;
    Range range;
};

//enum
//{
//    SENSOR_HTU21D,
//    SENSOR_HIH4030,
//};

enum
{
    JOURNAL_UNKNOWN = 0,
    JOURNAL_ATTEMPT = 0xAA,
    JOURNAL_SUCCESS = 0x55,
};

namespace Record
{
    void init();

    void getHardwareVersion(Version &version);
    void setHardwareVersion(const Version &version);

    void getFirmwareVersion(Version &version);
    void setFirmwareVersion(const Version &version);

    unsigned long getBootCount();
    void setBootCount(unsigned long count);
    void incrementBootCount();

    void setDeviceEnabled(byte device, bool enabled);
    bool deviceEnabled(byte device);
    
    unsigned long getLastBootTime();
    void setLastBootTime(unsigned long time);

    unsigned long getLastBootTime(byte device);
    void setLastBootTime(byte device, unsigned long time);

    void setRelayBegin(byte port);
    void setRelayEnd(byte port);
    bool relayFailed(byte port);

//    void getSensorStatus(int sensor, SensorStatus &status);
//    void setSensorStatus(int sensor, const SensorStatus &status);
//
//    void getSensorStatus(byte port, int sensor, SensorStatus &status);
//    void setSensorStatus(byte port, int sensor, const SensorStatus &status);

    unsigned int getBootAttempts(byte device);
    void setBootAttempts(byte device, unsigned int attempts);
    void incrementBootAttempts(byte device);
    
    unsigned int getBootFailures(byte device);
    void setBootFailures(byte device, unsigned int failures);
    void incrementBootFailures(byte device);

    int getFaultCurrent(byte device);
    void setFaultCurrent(byte device, int current);

    unsigned long getFaultTimeout(byte device);
    unsigned long getHeartbeatTimeout(byte device);
    unsigned long getUnmanagedChangeTime(byte device);
    unsigned long getStopTimeout(byte device);
};

