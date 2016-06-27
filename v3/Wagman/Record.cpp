#include <EEPROM.h>
#include "Record.h"

static const unsigned long MAGIC = 0xADA1ADA1;

static const unsigned int DEVICE_COUNT = 5;

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
    EEPROM_PORT_BOOT_FAILURES = 11;

// sync up with memory layout.
// some of these can be replace by a simple fixed size offset
// for now...using this.

static const unsigned int EEPROM_DEVICE_ENABLED[DEVICE_COUNT] = {520, 584, 1000, 808, 1000};
static const unsigned int EEPROM_BOOT_ATTEMPTS[DEVICE_COUNT] = {521, 585, 1000, 808, 1000};
static const unsigned int EEPROM_BOOT_FAILURES[DEVICE_COUNT] = {523, 587, 1000, 808, 1000};

static const unsigned int EEPROM_RELAY_JOURNALS[DEVICE_COUNT] = {800, 801, 1000, 1000, 1000};

namespace Record
{

unsigned int deviceRegion(int device)
{
    return EEPROM_PORT_REGIONS_START + device * EEPROM_PORT_REGIONS_SIZE;
}

void init()
{
    unsigned long magic;

    EEPROM.get(EEPROM_MAGIC_ADDR, magic);

    if (magic != MAGIC) {
        Record::setBootCount(0);
        Record::setLastBootTime(0);

        Version version;
    
        version.major = 3;
        version.minor = 1;
        Record::setHardwareVersion(version);
    
        version.major = 1;
        version.minor = 0;
        Record::setFirmwareVersion(version);

        for (unsigned int i = 0; i < DEVICE_COUNT; i++) {
            setDeviceEnabled(i, false);
            setBootAttempts(i, 0);
            setBootFailures(i, 0);
            EEPROM.write(EEPROM_RELAY_JOURNALS[i], JOURNAL_UNKNOWN);
        }

        // default setup is just node controller and single guest node.
        setDeviceEnabled(0, true);
        setDeviceEnabled(1, true);
        
        EEPROM.put(EEPROM_MAGIC_ADDR, MAGIC);
    }
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

unsigned long getBootCount()
{
    unsigned long count;
    EEPROM.get(EEPROM_BOOT_COUNT, count);
    return count;
}

void setBootCount(unsigned long count)
{
    EEPROM.put(EEPROM_BOOT_COUNT, count);
}

void incrementBootCount()
{
    setBootCount(getBootCount() + 1);
}

void setDeviceEnabled(int device, bool enabled)
{
    EEPROM.write(deviceRegion(device) + EEPROM_PORT_ENABLED, enabled);
}

bool deviceEnabled(int device)
{
    if (0 <= device && device < 5) {
        return EEPROM.read(deviceRegion(device) + EEPROM_PORT_ENABLED);
    } else {
        return false;
    }
}

unsigned long getLastBootTime()
{
    unsigned long time;
    EEPROM.get(EEPROM_LAST_BOOT_TIME, time);
    return time;
}

void setLastBootTime(unsigned long time)
{
    EEPROM.put(EEPROM_LAST_BOOT_TIME, time);
}

unsigned long getLastBootTime(int device)
{
    unsigned long time;
    EEPROM.get(deviceRegion(device) + EEPROM_PORT_LAST_BOOT_TIME, time);
    return time;
}

void setLastBootTime(int device, unsigned long time)
{
    EEPROM.put(deviceRegion(device) + EEPROM_PORT_LAST_BOOT_TIME, time);
}

unsigned int getBootAttempts(int device)
{
    unsigned int attempts;
    EEPROM.get(EEPROM_BOOT_ATTEMPTS[device], attempts);
    return attempts;
}

void setBootAttempts(int device, unsigned int attempts)
{
    EEPROM.put(EEPROM_BOOT_ATTEMPTS[device], attempts);
}

void incrementBootAttempts(int device)
{
    setBootAttempts(device, getBootAttempts(device) + 1);
}

unsigned int getBootFailures(int device)
{
    unsigned int failures;
    EEPROM.get(EEPROM_BOOT_FAILURES[device], failures);
    return failures;
}

void setBootFailures(int device, unsigned int failures)
{
    EEPROM.put(EEPROM_BOOT_FAILURES[device], failures);
}

void incrementBootFailures(int device)
{
    setBootFailures(device, getBootFailures(device) + 1);
}

void setRelayBegin(int port)
{
    EEPROM.write(EEPROM_RELAY_JOURNALS[port], JOURNAL_ATTEMPT);
}

void setRelayEnd(int port)
{
    EEPROM.write(EEPROM_RELAY_JOURNALS[port], JOURNAL_SUCCESS);
}

bool relayFailed(int port)
{
    return EEPROM.read(EEPROM_RELAY_JOURNALS[port]) == JOURNAL_ATTEMPT;
}

int getFaultCurrent(int port)
{
    if (port == 0)
        return 120; // we also need a strategy for autodetecting reasonable ranges over time and then sticking to those.

    if (port == 1)
        return 140;

    return 10000;
}

void setFaultCurrent(int port, int current)
{
}

unsigned long getFaultTimeout(int device)
{
    return 15 * 1000; // 15 seconds
}

unsigned long getHeartbeatTimeout(int device)
{
    return 120 * 1000; // 120 seconds
}

unsigned long getUnmanagedChangeTime(int device)
{
    return 8 * 60 * 60 * 1000; // 8 hours
}

unsigned long getStopTimeout(int device)
{
    return 60 * 1000; // 60 seconds
}

};

