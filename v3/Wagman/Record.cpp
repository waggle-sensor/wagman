#include <EEPROM.h>
#include "Record.h"
#include "Wagman.h"

static const unsigned long MAGIC = 0xADA1ADA1;

static const byte DEVICE_COUNT = 5;

// Config EEPROM Spec

static const unsigned int
    EEPROM_MAGIC_ADDR = 0,
    EEPROM_HARDWARE_VERSION = 4,
    EEPROM_FIRMWARE_VERSION = 6,
    EEPROM_BOOT_COUNT = 8,
    EEPROM_LAST_BOOT_TIME = 12;

// Wagman EEPROM Spec

static const unsigned int WAGMAN_REGION_START = 128;

static const unsigned int WAGMAN_LAST_BOOT_TIME = 0;

static const unsigned int WAGMAN_HTU21D_HEALTH = 4;
static const unsigned int WAGMAN_HTU21D_MIN = 5;
static const unsigned int WAGMAN_HTU21D_MAX = 7;

static const unsigned int WAGMAN_HIH4030_HEALTH = 9;
static const unsigned int WAGMAN_HIH4030_MIN = 10;
static const unsigned int WAGMAN_HIH4030_MAX = 12;

static const unsigned int WAGMAN_CURRENT_HEALTH = 14;
static const unsigned int WAGMAN_CURRENT_MIN = 15;
static const unsigned int WAGMAN_CURRENT_MAX = 17;

// Device EEPROM Spec

static const unsigned int EEPROM_PORT_REGIONS_START = 256;
static const unsigned int EEPROM_PORT_REGIONS_SIZE = 128;
static const unsigned int
    EEPROM_PORT_ENABLED = 0,
    EEPROM_PORT_MANAGED = 1,

    EEPROM_PORT_BOOT_SELECT = 2,
    
    EEPROM_PORT_LAST_BOOT_TIME = 3,
    EEPROM_PORT_BOOT_ATTEMPTS = 7,
    EEPROM_PORT_BOOT_FAILURES = 11,
    
    EEPROM_PORT_THERM_HEALTH = 15,
    EEPROM_PORT_THREM_MIN = 16,
    EEPROM_PORT_THREM_MAX = 18,
    
    EEPROM_PORT_CURRENT_HEALTH = 20,
    EEPROM_PORT_CURRENT_MIN = 21,
    EEPROM_PORT_CURRENT_MAX = 23,
    EEPROM_PORT_CURRENT_FAULT_LEVEL = 25,
    
    EEPROM_PORT_CURRENT_FAULT_TIMEOUT = 27,
    EEPROM_PORT_CURRENT_HEARTBEAT_TIMEOUT = 31,

    EEPROM_PORT_RELAY_HEALTH = 35,
    EEPROM_PORT_RELAY_JOURNAL = 36;

namespace Record
{

unsigned int deviceRegion(byte device)
{
    return EEPROM_PORT_REGIONS_START + device * EEPROM_PORT_REGIONS_SIZE;
}

bool initialized()
{
    unsigned long magic;
    EEPROM.get(EEPROM_MAGIC_ADDR, magic);
    return magic == MAGIC;
}

void init()
{
    if (initialized())
        return;

    Record::setBootCount(0);
    Record::setLastBootTime(0);

    Version version;

    version.major = 3;
    version.minor = 1;
    Record::setHardwareVersion(version);

    version.major = 1;
    version.minor = 0;
    Record::setFirmwareVersion(version);

    for (byte i = 0; i < DEVICE_COUNT; i++) {
        setBootAttempts(i, 0);
        setBootFailures(i, 0);
        setRelayState(i, RELAY_OFF);
    }

    // default setup is just node controller and single guest node.
    setDeviceEnabled(0, true);
    setDeviceEnabled(1, true);
    setDeviceEnabled(2, true);
    setDeviceEnabled(3, false);
    setDeviceEnabled(4, false);
    
    EEPROM.put(EEPROM_MAGIC_ADDR, MAGIC);
}

void getHardwareVersion(Version &version)
{
    EEPROM.get(EEPROM_HARDWARE_VERSION, version);
}

void setHardwareVersion(const Version &version)
{
    EEPROM.put(EEPROM_HARDWARE_VERSION, version);
}

void getFirmwareVersion(Version &version)
{
    EEPROM.get(EEPROM_FIRMWARE_VERSION, version);
}

void setFirmwareVersion(const Version &version)
{
    EEPROM.put(EEPROM_FIRMWARE_VERSION, version);
}

void getBootCount(unsigned long &count)
{
    EEPROM.get(EEPROM_BOOT_COUNT, count);
}

void setBootCount(const unsigned long &count)
{
    EEPROM.put(EEPROM_BOOT_COUNT, count);
}

void incrementBootCount()
{
    unsigned long count;
    
    getBootCount(count);
    count++;
    setBootCount(count);
}

void setDeviceEnabled(byte device, bool enabled)
{
    EEPROM.write(deviceRegion(device) + EEPROM_PORT_ENABLED, enabled);
}

bool getDeviceEnabled(byte device)
{
    if (!Wagman::validPort(device)) {
        return false;
    }

    // we never allow the node controller port to be disabled!
    if (device == 0) {
        return true;
    }

    return EEPROM.read(deviceRegion(device) + EEPROM_PORT_ENABLED);
}

void getLastBootTime(time_t &time)
{
    EEPROM.get(EEPROM_LAST_BOOT_TIME, time);
}

void setLastBootTime(const time_t &time)
{
    EEPROM.put(EEPROM_LAST_BOOT_TIME, time);
}

void getLastBootTime(byte device, time_t &time)
{
    EEPROM.get(deviceRegion(device) + EEPROM_PORT_LAST_BOOT_TIME, time);
}

void setLastBootTime(byte device, const time_t &time)
{
    EEPROM.put(deviceRegion(device) + EEPROM_PORT_LAST_BOOT_TIME, time);
}

unsigned int getBootAttempts(byte device)
{
    unsigned int attempts;
    EEPROM.get(deviceRegion(device) + EEPROM_PORT_BOOT_ATTEMPTS, attempts);
    return attempts;
}

void setBootAttempts(byte device, unsigned int attempts)
{
    EEPROM.put(deviceRegion(device) + EEPROM_PORT_BOOT_ATTEMPTS, attempts);
}

void incrementBootAttempts(byte device)
{
    setBootAttempts(device, getBootAttempts(device) + 1);
}

unsigned int getBootFailures(byte device)
{
    unsigned int failures;
    EEPROM.get(deviceRegion(device) + EEPROM_PORT_BOOT_FAILURES, failures);
    return failures;
}

void setBootFailures(byte device, unsigned int failures)
{
    EEPROM.put(deviceRegion(device) + EEPROM_PORT_BOOT_FAILURES, failures);
}

void incrementBootFailures(byte device)
{
    setBootFailures(device, getBootFailures(device) + 1);
}

byte getRelayState(byte port)
{
    return EEPROM.read(deviceRegion(port) + EEPROM_PORT_RELAY_JOURNAL);
}

void setRelayState(byte port, byte state)
{
    EEPROM.write(deviceRegion(port) + EEPROM_PORT_RELAY_JOURNAL, state);
}

int getFaultCurrent(byte port)
{
    // these are hardcoded for now to be "sensible" levels.
    if (port == 0)
        return 122;
    if (port == 1)
        return 122;
    if (port == 2)
        return 112;
    return 10000;
}

void setFaultCurrent(byte port, int current)
{
}

unsigned long getFaultTimeout(byte device)
{
    return (unsigned long)15000; // 15 seconds
}

unsigned long getHeartbeatTimeout(byte device)
{
    return (unsigned long)60000; // 60 seconds
}

unsigned long getUnmanagedChangeTime(byte device)
{
    return (unsigned long)28800000; // 8 hours
}

unsigned long getStopTimeout(byte device)
{
    return (unsigned long)60000; // 60 seconds
}

};

