// This file is part of the Waggle Platform.Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.For more details on the Waggle project, visit:
// http://www.wa8.gl
/*

L [Relay] R
o o o o o o
| | | | | |
| | | | | +--- TX
| | | | +----- _V
| | | +------- RX
| | +--------- GND
| +----------- TH
+------------- TH

*/

#include "Wagman.h"

#include <Arduino.h>
#include <Wire.h>

#include <array>

#include "HTU21D.h"
#include "MCP342X.h"
#include "MCP79412RTC.h"
#include "Record.h"

static const byte PORT_COUNT = 5;
static const byte BOOT_SELECTOR_COUNT = 2;
std::array<byte, 9> LED_PINS = {12, 11, 2, 3, 5, 6, 7, 8, 9};

struct RelayConfig {
  int clk;
  int d;
};

const RelayConfig relayConfigs[PORT_COUNT] = {
    {.clk = 33, .d = 34}, {.clk = 35, .d = 36}, {.clk = 37, .d = 38},
    {.clk = 39, .d = 40}, {.clk = 45, .d = 46},
};

static const byte THERMISTOR_0_PIN = A0;
static const byte THERMISTOR_CHANNELS[] = {
    0,
    MCP342X::CHANNEL_0,
    MCP342X::CHANNEL_1,
    MCP342X::CHANNEL_2,
    MCP342X::CHANNEL_3,
};

const int voltagePins[5] = {A1, A3, A5, A7, A9};
const int thermistorPins[5] = {A0, A2, A4, A6, A8};
const int photoresistorPin = A10;

static const byte SYSTEM_CURRENT_ADDRESS = 0x60;

static const byte PORT_CURRENT_ADDRESS[PORT_COUNT] = {0x62, 0x68, 0x6A, 0x63,
                                                      0x6B};

static const byte BOOT_SELECTOR_PINS[BOOT_SELECTOR_COUNT] = {41, 47};

static HTU21D htu21d;
static MCP342X mcp3428[2];

static bool wireEnabled = true;

unsigned int heartbeatCounters[5] = {0, 0, 0, 0, 0};

namespace Wagman {

unsigned int getVoltage(int port) {
  if (!validPort(port)) {
    return 0;
  }

  return analogRead(voltagePins[port]);
}

unsigned int getThermistor(int port) {
  if (!validPort(port)) {
    return 0;
  }

  return analogRead(thermistorPins[port]);
}

bool getLight(unsigned int *raw) {
  *raw = analogRead(photoresistorPin);
  return true;
}

void setLEDs(int mode) {
  for (auto pin : LED_PINS) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, mode);
  }
}

void setLEDAnalog(byte led, int level) {
  if (!validLED(led)) {
    return;
  }

  if (level < 0) {
    level = 0;
  }

  if (level > 255) {
    level = 255;
  }

  pinMode(LED_PINS[led], OUTPUT);
  analogWrite(LED_PINS[led], level);
}

void setLED(byte led, bool on) {
  if (!validLED(led)) {
    return;
  }

  pinMode(LED_PINS[led], OUTPUT);
  digitalWrite(LED_PINS[led], on ? HIGH : LOW);
}

bool getLED(byte led) {
  if (!validLED(led)) return false;

  return digitalRead(LED_PINS[led]) == HIGH;
}

void toggleLED(byte led) { setLED(led, !getLED(led)); }

#define NC_AUTO_DISABLE 48

void init() {
  // TODO Confirm this is what we want. Maybe only set this once we're toggling
  // NC relay?
  pinMode(NC_AUTO_DISABLE, OUTPUT);
  digitalWrite(NC_AUTO_DISABLE, LOW);

  analogReadResolution(12);

  for (auto pin : LED_PINS) {
    pinMode(pin, OUTPUT);
  }

  for (int i = 0; i < PORT_COUNT; i++) {
    pinMode(relayConfigs[i].clk, OUTPUT);
    pinMode(relayConfigs[i].d, OUTPUT);
    heartbeatCounters[i] = 0;
  }

  for (int i = 0; i < BOOT_SELECTOR_COUNT; i++) {
    pinMode(BOOT_SELECTOR_PINS[i], OUTPUT);
  }

  wireEnabled = Record::getWireEnabled();

  mcp3428[0].init(MCP342X::H, MCP342X::H);
  mcp3428[1].init(MCP342X::L, MCP342X::H);
  delay(200);

  htu21d.begin();
  delay(200);

  noInterrupts();

#warning "Not running heartbeat timer. Expect regular timeouts."
  /* TODO Correct timer behavior.
  // schedule timer 1 to check on heartbeat signal. this is to ensure that
  // we never miss a heartbeat check.
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 = _BV(TOIE1); // only has interrupt enabled (no comparator or other
  features) TCCR1B = _BV(CS11)| _BV(CS10); // about every 1/4 second
  */

  interrupts();
}

void setLatchedRelay(int clk, int d, int mode) {
  pinMode(clk, OUTPUT);
  pinMode(d, OUTPUT);

  digitalWrite(clk, LOW);
  digitalWrite(d, mode);
  delay(100);
  digitalWrite(clk, HIGH);
  delay(100);
  digitalWrite(clk, LOW);
  digitalWrite(d, LOW);
  delay(100);
}

// need to change usage of LEDs slightly... want a dedicated "signal" LED
// want a relay LED.
// perhaps on a heartbeat, we can blink on of the power leds briefly, if its on
// (hypothetically, we shouldn't be able to not do this...)
// think in terms of a switch with blinking traffic lights.

void setRelay(int port, int mode) {
  if (!validPort(port)) {
    return;
  }

  setLatchedRelay(relayConfigs[port].clk, relayConfigs[port].d, mode);
}

//
// Gets the current drawn by the entire system.
//
unsigned int getCurrent() {
  mcp3428[0].selectChannel(MCP342X::CHANNEL_0, MCP342X::GAIN_1);
  // return mcp3428[0].readADC() >> 5;
  return mcp3428[0].readADC();
}

struct CurrentADCConfig {
  int device;
  int channel;
  int gain;
};

CurrentADCConfig currentADCConfigs[] = {
    {0, MCP342X::CHANNEL_3, MCP342X::GAIN_1},
    {0, MCP342X::CHANNEL_1, MCP342X::GAIN_1},
    {0, MCP342X::CHANNEL_2, MCP342X::GAIN_1},
    {1, MCP342X::CHANNEL_0, MCP342X::GAIN_1},
    {1, MCP342X::CHANNEL_3, MCP342X::GAIN_1},
};

//
// Gets the current drawn by a particular port.
//
unsigned int getCurrent(byte port) {
  if (!validPort(port)) {
    return 0;
  }

  int device = currentADCConfigs[port].device;
  int channel = currentADCConfigs[port].channel;
  int gain = currentADCConfigs[port].gain;

  mcp3428[device].selectChannel(channel, gain);
  return mcp3428[device].readADC();
}

bool getHumidity(unsigned int *raw, float *hrf) {
  if (!getWireEnabled()) {
    return false;
  }

  return htu21d.readHumidity(raw, hrf);
}

bool getTemperature(unsigned int *raw, float *hrf) {
  if (!getWireEnabled()) {
    return false;
  }

  return htu21d.readTemperature(raw, hrf);
}

byte getBootMedia(byte selector) {
  if (!validBootSelector(selector)) return MEDIA_INVALID;

  switch (digitalRead(BOOT_SELECTOR_PINS[selector])) {
    case LOW:
      return MEDIA_SD;
    case HIGH:
      return MEDIA_EMMC;
    default:
      return MEDIA_INVALID;
  }
}

void setBootMedia(byte selector, byte media) {
  if (!validBootSelector(selector)) return;

  switch (media) {
    case MEDIA_SD:
      digitalWrite(BOOT_SELECTOR_PINS[selector], LOW);
      break;
    case MEDIA_EMMC:
      digitalWrite(BOOT_SELECTOR_PINS[selector], HIGH);
      break;
  }
}

void toggleBootMedia(byte selector) {
  if (getBootMedia(selector) == MEDIA_SD) {
    setBootMedia(selector, MEDIA_EMMC);
  } else {
    setBootMedia(selector, MEDIA_SD);
  }
}

bool validPort(byte port) { return port < PORT_COUNT; }

bool validLED(byte led) { return led < LED_PINS.size(); }

bool validBootSelector(byte selector) { return selector < BOOT_SELECTOR_COUNT; }

unsigned int getAddressCurrent(byte addr) {
  static const unsigned int MILLIAMPS_PER_STEP = 16;
  byte csb, lsb;
  byte attempts;
  byte timeout;

  if (!getWireEnabled()) {
    return 0;
  }

  for (attempts = 0; attempts < 10; attempts++) {
    /* request data from sensor */
    Wire.beginTransmission(addr);
    delay(5);
    Wire.write(0);
    delay(5);

    /* retry on error */
    if (Wire.endTransmission(0) != 0) continue;

    delay(5);

    /* read data from sensor */
    if (Wire.requestFrom(addr, 3) != 3) continue;

    delay(5);

    for (timeout = 0; timeout < 100 && Wire.available() < 3; timeout++) {
      delay(5);
    }

    /* retry on error */
    if (timeout >= 100) continue;

    Wire.read();
    csb = Wire.read() & 0x01;
    lsb = Wire.read();

    /* retry on error */
    if (Wire.endTransmission(1) != 0) continue;

    delay(5);

    /* return milliamps from raw sensor data. */
    return ((csb << 8) | lsb) * MILLIAMPS_PER_STEP;
  }

  return 0; /* return error value */
}

void getTime(time_t &time) {
  if (getWireEnabled()) {
    time = Wagman::Clock.get();
  } else {
    time = 0;
  }
}

void setTime(const time_t &time) {
  if (getWireEnabled()) {
    Wagman::Clock.set(time);
  }
}

void getDateTime(DateTime &dt) {
  if (getWireEnabled()) {
    tmElements_t tm;

    Wagman::Clock.read(tm);

    dt.year = tm.Year + 1970;
    dt.month = tm.Month;
    dt.day = tm.Day;
    dt.hour = tm.Hour;
    dt.minute = tm.Minute;
    dt.second = tm.Second;
  } else {
    dt.year = 0;
    dt.month = 0;
    dt.day = 0;
    dt.hour = 0;
    dt.minute = 0;
    dt.second = 0;
  }
}

void setDateTime(const DateTime &dt) {
  if (getWireEnabled()) {
    tmElements_t tm;

    tm.Year = dt.year - 1970;
    tm.Month = dt.month;
    tm.Day = dt.day;
    tm.Hour = dt.hour;
    tm.Minute = dt.minute;
    tm.Second = dt.second;

    Wagman::Clock.set(makeTime(tm));
  }
}

void setWireEnabled(bool enabled) {
  Record::setWireEnabled(enabled);
  wireEnabled = enabled;
}

bool getWireEnabled() { return wireEnabled; }

void getID(byte id[8]) {
  if (getWireEnabled()) {
    Wagman::Clock.idRead(id);
  } else {
    for (byte i = 0; i < 8; i++) {
      id[i] = 0xFF;
    }
  }
}

};  // namespace Wagman
