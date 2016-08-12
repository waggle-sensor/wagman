#include <Arduino.h>
#include <Wire.h>
#include "HTU21D.h"
#include "MCP342X.h"
#include "MCP79412RTC.h"
#include "Wagman.h"

static const byte LED_COUNT = 2;
static const byte PORT_COUNT = 5;
static const byte BOOT_SELECTOR_COUNT = 2;
static const byte LED_PINS[LED_COUNT] = {13, 9};
static const byte POWER_PINS[PORT_COUNT] = {4, 6, 8, 10, 12};
static const byte POWER_LATCH_0_PIN = 0;
static const byte HEARTBEAT_PINS[PORT_COUNT] = {5, 7, A4, 11, A5};
static const byte THERMISTOR_0_PIN = A0;
static const byte THERMISTOR_CHANNELS[] = {
    0,
    MCP342X::CHANNEL_0,
    MCP342X::CHANNEL_1,
    MCP342X::CHANNEL_2,
    MCP342X::CHANNEL_3,
};

static const byte SYSTEM_CURRENT_ADDRESS = 0x60;

static const byte PORT_CURRENT_ADDRESS[PORT_COUNT] = {0x62, 0x68, 0x6A, 0x63, 0x6B};

static const byte BOOT_SELECTOR_PINS[BOOT_SELECTOR_COUNT] = {1, A3};

static HTU21D htu21d;
static MCP342X mcp3428_1;

static volatile byte heartbeatState[5] = {LOW, LOW, LOW, LOW, LOW};
volatile byte heartbeatCounters[5] = {0, 0, 0, 0, 0};

ISR(TIMER1_OVF_vect) {
    for (byte i = 0; i < 5; i++) {
        byte newState = digitalRead(HEARTBEAT_PINS[i]);
        bool triggered = heartbeatState[i] != newState;
        heartbeatState[i] = newState;

        if (triggered) {
            heartbeatCounters[i]++;
        }
    }
}

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
    for (byte i = 0; i < LED_COUNT; i++) {
        pinMode(LED_PINS[i], OUTPUT);
    }

    for (byte i = 0; i < PORT_COUNT; i++) {
        pinMode(POWER_PINS[i], OUTPUT);
        pinMode(HEARTBEAT_PINS[i], INPUT);
        heartbeatCounters[i] = 0;
    }

    for (byte i = 0; i < BOOT_SELECTOR_COUNT; i++) {
        pinMode(BOOT_SELECTOR_PINS[i], OUTPUT);
    }

    pinMode(POWER_LATCH_0_PIN, OUTPUT);

    Wire.begin();
    delay(200);

    mcp3428_1.init(MCP342X::H, MCP342X::H);
    delay(200);

    htu21d.begin();
    delay(200);

    noInterrupts();

    // schedule timer 1 to check on heartbeat signal. this is to ensure that
    // we never miss a heartbeat check.
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = _BV(TOIE1); // only has interrupt enabled (no comparator or other features)
    TCCR1B = _BV(CS11)| _BV(CS10); // about every 1/4 second

    interrupts();
}

void setRelay(byte port, bool on)
{
    if (!validPort(port))
        return;

    if (port == 0) {
        digitalWrite(POWER_LATCH_0_PIN, LOW);
        delay(50);
        digitalWrite(POWER_PINS[port], on ? HIGH : LOW);
        delay(50);
        digitalWrite(POWER_LATCH_0_PIN, HIGH);
        delay(50);
        digitalWrite(POWER_LATCH_0_PIN, LOW);
        delay(50);
    } else {
        digitalWrite(POWER_PINS[port], on ? HIGH : LOW);
        delay(50);
    }
}

byte getRelay(byte port)
{
    if (!validPort(port)) {
        return RELAY_OFF; // probably should indicate error state instead...
    }

    return RELAY_OFF;
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
        if (Wire.requestFrom(addr, 3) != 3)
            continue;

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

