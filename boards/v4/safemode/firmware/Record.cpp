// ANL:waggle-license
// This file is part of the Waggle Platform.  Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.  For more details on the Waggle project, visit:
//          http://www.wa8.gl
// ANL:waggle-license
#include "EEPROM.h"
#include "Record.h"
#include "Wagman.h"
#include "commands.h"

#warning "Using mocked out RAM EEPROM."
// MockEEPROM<4096> EEPROM;
ExternalEEPROM EEPROM;

static const unsigned long MAGIC = 0xADA1ADA1;

static const byte DEVICE_COUNT = 5;

// Config EEPROM Spec

static const byte BOOT_LOG_CAPACITY = 4;

static const int
    EEPROM_MAGIC_ADDR = 0,
    EEPROM_HARDWARE_VERSION = 4,
    EEPROM_FIRMWARE_VERSION = 6,
    EEPROM_BOOT_COUNT = 8,
    EEPROM_LAST_BOOT_TIME = 12,
    EEPROM_WIRE_ENABLED = 32,
    EEPROM_BOOTLOADER_NODE_CONTROLLER = 0x40;

static const int WAGMAN_REGION_START = 128;
static const int WAGMAN_LAST_BOOT_TIME = 0;
static const int WAGMAN_HTU21D_HEALTH = 4;
static const int WAGMAN_HTU21D_RANGE = 5;
static const int WAGMAN_HIH4030_HEALTH = 9;
static const int WAGMAN_HIH4030_RANGE = 10;
static const int WAGMAN_CURRENT_HEALTH = 14;
static const int WAGMAN_CURRENT_RANGE = 15;

// Device EEPROM Spec

static const int EEPROM_PORT_REGIONS_START = 256;
static const int EEPROM_PORT_REGIONS_SIZE = 128;
static const int
    EEPROM_PORT_ENABLED = 0,
    EEPROM_PORT_MANAGED = 1,

    EEPROM_PORT_BOOT_SELECT = 2,

    EEPROM_PORT_LAST_BOOT_TIME = 3,
    EEPROM_PORT_BOOT_ATTEMPTS = 7,
    EEPROM_PORT_BOOT_FAILURES = 11,

    EEPROM_PORT_THERMISTOR_HEALTH = 15,
    EEPROM_PORT_THERMISTOR_RANGE = 16,

    EEPROM_PORT_CURRENT_HEALTH = 20,
    EEPROM_PORT_CURRENT_RANGE = 21,
    EEPROM_PORT_CURRENT_FAULT_LEVEL = 25,

    EEPROM_PORT_CURRENT_LEVEL_LOW = 21,
    EEPROM_PORT_CURRENT_LEVEL_NORMAL = 23,
    EEPROM_PORT_CURRENT_LEVEL_STRESSED = 25,
    EEPROM_PORT_CURRENT_LEVEL_HIGH = 27,

    EEPROM_PORT_CURRENT_FAULT_TIMEOUT = 29,
    EEPROM_PORT_CURRENT_HEARTBEAT_TIMEOUT = 33,

    EEPROM_PORT_RELAY_HEALTH = 37,
    EEPROM_PORT_RELAY_JOURNAL = 38,
    EEPROM_PORT_BOOT_LOG = 64;

namespace Record
{

BootLog bootLogs[5] = {
    BootLog(256 + 0 * 128 + 64),
    BootLog(256 + 1 * 128 + 64),
    BootLog(256 + 2 * 128 + 64),
    BootLog(256 + 3 * 128 + 64),
    BootLog(256 + 4 * 128 + 64),
};

int deviceRegion(byte device)
{
    return EEPROM_PORT_REGIONS_START + device * EEPROM_PORT_REGIONS_SIZE;
}

bool initialized()
{
    unsigned long magic;
    EEPROM.get(EEPROM_MAGIC_ADDR, magic);
    #ifdef CLEANSLATE
    return false;
    #else
    return magic == MAGIC;
    #endif
}

void init()
{
    Version version;

    Record::setBootCount(0);
    Record::setLastBootTime(0);

    version.major = 3;
    version.minor = 1;
    Record::setHardwareVersion(version);

    version.major = 1;
    version.minor = 0;
    Record::setFirmwareVersion(version);

    setWireEnabled(true);

    for (byte i = 0; i < DEVICE_COUNT; i++) {
        setBootAttempts(i, 0);
        setBootFailures(i, 0);
        setRelayState(i, RELAY_OFF);

        setPortCurrentSensorHealth(i, 0);
        setThermistorSensorHealth(i, 0);

        bootLogs[i].init();
    }

    // default setup is just node controller and single guest node.
    setDeviceEnabled(0, true);
    setDeviceEnabled(1, true);
    setDeviceEnabled(2, true);
    setDeviceEnabled(3, false);
    setDeviceEnabled(4, false);

    setBootloaderNodeController(0);

    EEPROM.put(EEPROM_MAGIC_ADDR, MAGIC);
}

void clearMagic() {
    EEPROM.put(EEPROM_MAGIC_ADDR, (unsigned long)0);
}

bool getWireEnabled()
{
    return EEPROM.read(EEPROM_WIRE_ENABLED);
}

void setWireEnabled(bool enabled)
{
    EEPROM.write(EEPROM_WIRE_ENABLED, enabled);
}

byte getBootloaderNodeController()
{
    return EEPROM.read(EEPROM_BOOTLOADER_NODE_CONTROLLER);
}

void setBootloaderNodeController(byte mode)
{
    // in case some wacky thing happens, set nc to safe mode.
    if (mode > 2) {
        mode = 1;
    }

    EEPROM.write(EEPROM_BOOTLOADER_NODE_CONTROLLER, mode);
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

byte getPortCurrentSensorHealth(byte port)
{
    return EEPROM.read(deviceRegion(port) + EEPROM_PORT_CURRENT_HEALTH);
}

void setPortCurrentSensorHealth(byte port, byte health)
{
    EEPROM.write(deviceRegion(port) + EEPROM_PORT_CURRENT_HEALTH, health);
}

byte getThermistorSensorHealth(byte port)
{
    return EEPROM.read(deviceRegion(port) + EEPROM_PORT_THERMISTOR_HEALTH);
}

void setThermistorSensorHealth(byte port, byte health)
{
    EEPROM.write(deviceRegion(port) + EEPROM_PORT_THERMISTOR_HEALTH, health);
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
    return 15000L; // 15 seconds
}

unsigned long getHeartbeatTimeout(byte device)
{
    return 60000L; // 60 seconds
}

unsigned long getUnmanagedChangeTime(byte device)
{
    return 28800000L; // 8 hours
}

unsigned long getStopTimeout(byte device)
{
    return 60000L; // 60 seconds
}

BootLog::BootLog(unsigned int addr)
{
    address = addr;
}

void BootLog::init()
{
    setStart(0);
    setCount(0);
}

void BootLog::addEntry(time_t time)
{
    byte start = getStart();
    byte count = getCount();
    byte index = (start + count) % capacity;

    EEPROM.put(address + 2 + sizeof(time_t) * index, time);

    // if there's no more space, overwrite the oldest entry
    if (count == capacity) {
        setStart(start + 1);
    } else {
        setCount(count + 1);
    }
}

time_t BootLog::getEntry(byte i) const
{
    byte start = getStart();
    byte count = getCount();

    if (i >= count) {
        return 0;
    }

    byte index = (start + i) % capacity;

    time_t time;
    EEPROM.get(address + 2 + sizeof(time_t) * index, time);
    return time;
}

byte BootLog::getStart() const
{
    return EEPROM.read(address + 0);
}

void BootLog::setStart(byte start)
{
    EEPROM.write(address + 0, start);
}

byte BootLog::getCount() const
{
    return EEPROM.read(address + 1);
}

void BootLog::setCount(byte count)
{
    EEPROM.write(address + 1, count);
}

byte BootLog::getCapacity() const
{
    return capacity;
}

};
