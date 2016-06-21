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

enum
{
    SENSOR_HTU21D,
    SENSOR_HIH4030,
};

enum
{
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

    void setDeviceEnabled(int device, bool enabled);
    bool getDeviceEnabled(int device);
    
    unsigned long getLastBootTime();
    void setLastBootTime(unsigned long time);

    void setRelayBegin(int port);
    void setRelayEnd(int port);
    bool setRelayFailed(int port);

    void getSensorStatus(int sensor, SensorStatus &status);
    void setSensorStatus(int sensor, const SensorStatus &status);

    void getSensorStatus(int port, int sensor, SensorStatus &status);
    void setSensorStatus(int port, int sensor, const SensorStatus &status);

    unsigned int getBootAttempts(int device);
    void setBootAttempts(int device, unsigned int attempts);
    void incrementBootAttempts(int device);
    
    unsigned int getBootFailures(int device);
    void setBootFailures(int device, unsigned int failures);
    void incrementBootFailures(int device);
};

