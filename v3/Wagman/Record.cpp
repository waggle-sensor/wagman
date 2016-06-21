#include <EEPROM.h>
#include "Record.h"

static const unsigned long MAGIC = 0xADA1ADA1;

static const unsigned int DEVICE_COUNT = 5;

static const unsigned int
    EEPROM_MAGIC = 0,
    EEPROM_HARDWARE_VERSION = 4,
    EEPROM_FIRMWARE_VERSION = 6,
    EEPROM_BOOT_COUNT = 8,
    EEPROM_LAST_BOOT_TIME = 12;

static const unsigned int EEPROM_DEVICE_ENABLED[DEVICE_COUNT] = {520, 584, 808, 808, 808};
static const unsigned int EEPROM_BOOT_ATTEMPTS[DEVICE_COUNT] = {521, 585, 808, 808, 808};
static const unsigned int EEPROM_BOOT_FAILURES[DEVICE_COUNT] = {523, 587, 808, 808, 808};

static const unsigned int EEPROM_RELAY_JOURNALS[DEVICE_COUNT] = {800, 801, 802, 803, 804};

namespace Record
{

void init()
{
    unsigned long magic;
    EEPROM.get(EEPROM_MAGIC, magic);

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
            setBootAttempts(i, 0);
            setBootFailures(i, 0);
            EEPROM.write(EEPROM_RELAY_JOURNALS[i], 0);
        }

        // default setup is just node controller and single guest node.
        setDeviceEnabled(0, true);
        setDeviceEnabled(1, true);
        setDeviceEnabled(2, false);
        setDeviceEnabled(3, false);
        setDeviceEnabled(4, false);
        
        EEPROM.put(EEPROM_MAGIC, MAGIC);
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
    EEPROM.write(EEPROM_DEVICE_ENABLED[device], enabled);
}

bool getDeviceEnabled(int device)
{
    return EEPROM.read(EEPROM_DEVICE_ENABLED[device]);
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

bool setRelayFailed(int port)
{
    return EEPROM.read(EEPROM_RELAY_JOURNALS[port]) == JOURNAL_ATTEMPT;
}

};

