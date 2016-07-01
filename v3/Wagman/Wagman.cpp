#include <Arduino.h>
#include <Wire.h>
#include "HTU21D.h"
#include "MCP342X.h"
#include "MCP79412RTC.h"
#include "Wagman.h"

static const byte LED_COUNT = 2;
static const byte PORT_COUNT = 5;
static const byte BOOT_SELECTOR_COUNT = 2;

static const byte LED_PINS[LED_COUNT] = {
    13, // L
    9,  // L1
};

static const byte POWER_PINS[PORT_COUNT] = {
    4,
    6,
    8,
    10,
    12,
};

static const byte POWER_LATCH_0_PIN = 0;

static const byte HEARTBEAT_PINS[PORT_COUNT] = {
    5,
    7, 
    A4,
    11,
    A5,
};

static const byte THERMISTOR_0_PIN = A0;

static const byte THERMISTOR_CHANNELS[] = {
    0,
    MCP342X::CHANNEL_0,
    MCP342X::CHANNEL_1,
    MCP342X::CHANNEL_2,
    MCP342X::CHANNEL_3,
};

static const byte SYSTEM_CURRENT_ADDRESS = 0x60;

static const byte PORT_CURRENT_ADDRESS[PORT_COUNT] = {
    0x62,
    0x68,
    0x6A,
    0x63,
    0x6B,
};

static const byte BOOT_SELECTOR_PINS[BOOT_SELECTOR_COUNT] = {
    1,
    A3,
};

static HTU21D htu21d;
static MCP342X mcp3428_1;

static byte relayState[PORT_COUNT]; // don't really need this.

namespace Wagman
{

unsigned int getThermistor(byte port)
{
    if (!validPort(port))
        return 0;
    
    if (port == 0) {
        return analogRead(THERMISTOR_0_PIN);
    } else {
        mcp3428_1.selectChannel(THERMISTOR_CHANNELS[port], MCP342X::GAIN_1);
        return mcp3428_1.readADC() >> 5;
    }
}

void setLED(byte led, bool on)
{
    if (!validLED(led))
        return;
    
    digitalWrite(LED_PINS[led], on ? HIGH : LOW);
}

bool getLED(byte led)
{
    if (!validLED(led))
        return false;
        
    return digitalRead(LED_PINS[led]) == HIGH;
}

void toggleLED(byte led)
{
    setLED(led, !getLED(led));
}

void init()
{
    Wire.begin();
    delay(1000);
    
    for (byte i = 0; i < LED_COUNT; i++) {
        pinMode(LED_PINS[i], OUTPUT);
    }

    for (byte i = 0; i < PORT_COUNT; i++) {
        pinMode(POWER_PINS[i], OUTPUT);
        pinMode(HEARTBEAT_PINS[i], INPUT);
        relayState[i] = RELAY_UNKNOWN;
    }

    for (byte i = 0; i < BOOT_SELECTOR_COUNT; i++) {
        pinMode(BOOT_SELECTOR_PINS[i], OUTPUT);
    }

    pinMode(POWER_LATCH_0_PIN, OUTPUT);
    
    delay(500);

    mcp3428_1.init(MCP342X::H, MCP342X::H);
    delay(500);

    htu21d.begin();
    delay(500);

    // check if RTC is ticking and if not keep track of this.
}

void setRelay(byte port, bool on)
{
    if (!validPort(port))
        return;

    if (port == 0) {
        digitalWrite(POWER_LATCH_0_PIN, LOW);
        delay(10);
        digitalWrite(POWER_PINS[port], on ? HIGH : LOW);
        delay(10);
        digitalWrite(POWER_LATCH_0_PIN, HIGH);
        delay(10);
        digitalWrite(POWER_LATCH_0_PIN, LOW);
        delay(10);
    } else {
        digitalWrite(POWER_PINS[port], on ? HIGH : LOW);
        delay(10);
    }

    relayState[port] = on ? RELAY_ON : RELAY_OFF; // possible to replace 
}

byte getRelay(byte port)
{
    if (!validPort(port)) {
        return RELAY_UNKNOWN;
    }

    return relayState[port];
}

byte getHeartbeat(byte port)
{
    if (!validPort(port))
        return LOW;

    return digitalRead(HEARTBEAT_PINS[port]);
}

//
// Gets the current drawn by the entire system.
//
unsigned int getCurrent()
{
    return getAddressCurrent(SYSTEM_CURRENT_ADDRESS);
}

//
// Gets the current drawn by a particular port.
//
unsigned int getCurrent(byte port)
{
    if (!validPort(port))
        return 0;

    return getAddressCurrent(PORT_CURRENT_ADDRESS[port]);
}

float getHumidity()
{
    return htu21d.readHumidity();
}

float getTemperature()
{
    return htu21d.readTemperature();
}

byte getBootMedia(byte selector)
{
    if (!validBootSelector(selector))
        return MEDIA_INVALID;

    switch (digitalRead(BOOT_SELECTOR_PINS[selector])) {
        case LOW:
            return MEDIA_SD;
        case HIGH:
            return MEDIA_EMMC;
        default:
            return MEDIA_INVALID;
    }
}

void setBootMedia(byte selector, byte media)
{
    if (!validBootSelector(selector))
        return;

    switch (media) {
        case MEDIA_SD:
            digitalWrite(BOOT_SELECTOR_PINS[selector], LOW);
            break;
        case MEDIA_EMMC:
            digitalWrite(BOOT_SELECTOR_PINS[selector], HIGH);
            break;
    }
}

void toggleBootMedia(byte selector)
{
    if (getBootMedia(selector) == MEDIA_SD) {
        setBootMedia(selector, MEDIA_EMMC);
    } else {
        setBootMedia(selector, MEDIA_SD);
    }
}

bool validPort(byte port)
{
    return port < PORT_COUNT;
}

bool validLED(byte led)
{
    return led < LED_COUNT;
}

bool validBootSelector(byte selector)
{
    return selector < BOOT_SELECTOR_COUNT;
}

unsigned int getAddressCurrent(byte addr)
{
    static const unsigned int MILLIAMPS_PER_STEP = 16;
    byte csb, lsb;
    byte attempts;
    byte timeout;

    for (attempts = 0; attempts < 10; attempts++) {

        /* request data from sensor */
        Wire.beginTransmission(addr);
        delay(5);
        Wire.write(0);
        delay(5);

        /* retry on error */
        if (Wire.endTransmission(0) != 0)
            continue;

        delay(5);

        /* read data from sensor */
        Wire.requestFrom(addr, 3);
        delay(5);

        for (timeout = 0; timeout < 100 && Wire.available() < 3; timeout++) {
            delay(5);
        }

        /* retry on error */
        if (timeout >= 100)
            continue;
        
        Wire.read();
        csb = Wire.read() & 0x01;
        lsb = Wire.read();

        /* retry on error */
        if (Wire.endTransmission(1) != 0)
            continue;

        delay(5);

        /* return milliamps from raw sensor data. */
        return ((csb << 8) | lsb) * MILLIAMPS_PER_STEP;
    }

    return 0xFFFF; /* return error value */
}

void getTime(time_t &time)
{
    time = RTC.get();
}

void setTime(byte year, byte month, byte day, byte hour, byte minute, byte second)
{
    tmElements_t tm;
    
    tm.Year = year - 1970;
    tm.Month = month;
    tm.Day = day;
    tm.Hour = hour;
    tm.Minute = minute;
    tm.Second = second;

    RTC.set(makeTime(tm));
}

};
