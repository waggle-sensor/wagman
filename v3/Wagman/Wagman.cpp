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

static byte relayState[PORT_COUNT];

namespace Wagman
{

unsigned int getThermistor(int port)
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

void setLED(int led, bool on)
{
    if (!validLED(led))
        return;
    
    digitalWrite(LED_PINS[led], on ? HIGH : LOW);
}

bool getLED(int led)
{
    if (!validLED(led))
        return false;
        
    return digitalRead(LED_PINS[led]) == HIGH;
}

void toggleLED(int led)
{
    setLED(led, !getLED(led));
}

void init()
{
    Wire.begin();
    
    for (int i = 0; i < LED_COUNT; i++) {
        pinMode(LED_PINS[i], OUTPUT);
    }

    for (int i = 0; i < PORT_COUNT; i++) {
        pinMode(POWER_PINS[i], OUTPUT);
        pinMode(HEARTBEAT_PINS[i], INPUT);
        relayState[i] = RELAY_UNKNOWN;
    }

    for (int i = 0; i < BOOT_SELECTOR_COUNT; i++) {
        pinMode(BOOT_SELECTOR_PINS[i], OUTPUT);
    }

    pinMode(POWER_LATCH_0_PIN, OUTPUT);

    mcp3428_1.init(MCP342X::H, MCP342X::H);

    htu21d.begin();
}

void setRelay(int port, bool on)
{
    if (!validPort(port))
        return;

    if (port == 0) {
        digitalWrite(POWER_LATCH_0_PIN, LOW);
        delay(2);
        digitalWrite(POWER_PINS[port], on ? HIGH : LOW);
        delay(2);
        digitalWrite(POWER_LATCH_0_PIN, HIGH);
        delay(2);
        digitalWrite(POWER_LATCH_0_PIN, LOW);
        delay(2);
    } else {
        digitalWrite(POWER_PINS[port], on ? HIGH : LOW);
        delay(2);
    }

    relayState[port] = on ? RELAY_ON : RELAY_OFF;
}

int getRelay(int port)
{
    if (!validPort(port)) {
        return RELAY_UNKNOWN;
    }

    return relayState[port];
}

int getHeartbeat(int port)
{
    if (!validPort(port))
        return LOW;

    return digitalRead(HEARTBEAT_PINS[port]);
}

//
// Gets the current drawn by the entire system.
//
int getCurrent()
{
    return getAddressCurrent(SYSTEM_CURRENT_ADDRESS);
}

//
// Gets the current drawn by a particular port.
//
int getCurrent(int port)
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

int getBootMedia(int selector)
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

void setBootMedia(int selector, int media)
{
    if (!validBootSelector(selector))
        return;

    switch (digitalRead(BOOT_SELECTOR_PINS[selector])) {
        case MEDIA_SD:
            digitalWrite(BOOT_SELECTOR_PINS[selector], LOW);
            break;
        case MEDIA_EMMC:
            digitalWrite(BOOT_SELECTOR_PINS[selector], HIGH);
            break;
    }
}

void toggleBootMedia(int selector)
{
    if (getBootMedia(selector) == MEDIA_SD) {
        setBootMedia(selector, MEDIA_EMMC);
    } else {
        setBootMedia(selector, MEDIA_SD);
    }
}

bool validPort(int port)
{
    return 0 <= port && port < PORT_COUNT;
}

bool validLED(int led)
{
    return 0 <= led && led < LED_COUNT;
}

bool validBootSelector(int selector)
{
    return 0 <= selector && selector < BOOT_SELECTOR_COUNT;
}

int getAddressCurrent(int addr)
{
    static const int MILLIAMPS_PER_STEP = 16;

    byte csb, lsb;
    // Start I2C transaction with current sensor

    Wire.beginTransmission(addr);
    // Tell sensor we want to read "data" register
    Wire.write(0);
    // Sensor expects restart condition, so end I2C transaction (no stop bit)
    Wire.endTransmission(0);
    
    // Ask sensor for data
    Wire.requestFrom(addr, 3);

    while (Wire.available() < 3) {
    }

    Wire.read(); // ignore first byte
    csb = Wire.read() & 0x01;
    lsb = Wire.read();
    Wire.endTransmission(1);

    // Calculate milliamps from raw sensor data
    unsigned int milliamps = ((csb << 8) | lsb) * MILLIAMPS_PER_STEP;
    return milliamps;
}

time_t getTime()
{
    return RTC.get();
}

void setTime(int year, int month, int day, int hour, int minute, int second)
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
