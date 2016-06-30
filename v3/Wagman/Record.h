#include <Arduino.h>
#include "Time.h"

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

    void getBootCount(unsigned long &count);
    void setBootCount(const unsigned long &count);
    void incrementBootCount();

    void getLastBootTime(time_t &time);
    void setLastBootTime(const time_t &time);

    void setDeviceEnabled(byte device, bool enabled);
    bool deviceEnabled(byte device);

    void getDeviceBootTime(byte device, time_t &time);
    void setDeviceBootTime(byte device, const time_t &time);

    void setRelayBegin(byte port);
    void setRelayEnd(byte port);
    bool relayFailed(byte port);

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

