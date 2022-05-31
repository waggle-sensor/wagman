// ANL:waggle-license
// This file is part of the Waggle Platform.  Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.  For more details on the Waggle project, visit:
//          http://www.wa8.gl
// ANL:waggle-license
#include <Arduino.h>
#line 1
#line 1 "/media/rajesh/Bharadwaja/github/waggle/wagman/manager/nc_power_on/nc_power_on.ino"
#include <Wire.h>
#include "./libs/HTU21D/HTU21D.h"
#include "./libs/MCP79412RTC/MCP79412RTC.h"    //http://github.com/JChristensen/MCP79412RTC
#include "./libs/Time/Time.h"
#include <avr/wdt.h>

// light sensor
#define light_sensor A5

// Thermistors
#define Thermistor1 A0
#define Thermistor2 A1
#define Thermistor3 A2
#define Thermistor4 A3
#define Thermistor5 A4

// Power ports
#define POW1 4
#define POW_LATCH1 0

#define POW2 6
#define POW3 8
#define POW4 10
#define POW5 12

// HTU21D
HTU21D myHumidity;

//RTC

//Hearbeat and Reset lines.

#define HBT1 5
#define HBT2 7
#define HBT3 9
#define HBT4 11
#define HBT5 13

// Current Sensors
#define CURRENT_SYS 0x60
#define CURRENT_POW1 0x62
#define CURRENT_POW2 0x63
#define CURRENT_POW3 0x68
#define CURRENT_POW4 0x6A
#define CURRENT_POW5 0x6B
// Resolution of current sensors (with 8A range) (mA)
#define MILLIAMPS_PER_STEP 16

#line 49 "/media/rajesh/Bharadwaja/github/waggle/wagman/manager/nc_power_on/nc_power_on.ino"
void setup();
#line 75 "/media/rajesh/Bharadwaja/github/waggle/wagman/manager/nc_power_on/nc_power_on.ino"
void turnON_POW1();
#line 88 "/media/rajesh/Bharadwaja/github/waggle/wagman/manager/nc_power_on/nc_power_on.ino"
void turnOFF_POW1();
#line 102 "/media/rajesh/Bharadwaja/github/waggle/wagman/manager/nc_power_on/nc_power_on.ino"
void loop();
#line 49
void setup()
{
    delay(5000);
    Serial.begin(115200);
//     myHumidity.begin();
    pinMode(POW1, OUTPUT);
    pinMode(POW_LATCH1, OUTPUT);
    pinMode(POW2, OUTPUT);
    pinMode(POW3, OUTPUT);
    pinMode(POW4, OUTPUT);
    pinMode(POW5, OUTPUT);

    pinMode(HBT1, INPUT);
    pinMode(HBT2, INPUT);
    pinMode(HBT3, INPUT);
    pinMode(HBT4, INPUT);
    pinMode(HBT5, INPUT);

    //RTC.set(1450418360);

    turnON_POW1 ();


}


void turnON_POW1 () {
    digitalWrite(POW_LATCH1, LOW);
    //Setting state to ON
    digitalWrite(POW1, HIGH);
    delay(1);
    // giving raising clock edge
    digitalWrite(POW_LATCH1, HIGH);
    delay(1);
    // lowering clock edge
    digitalWrite(POW_LATCH1, LOW);
    delay(1);
}

void turnOFF_POW1 () {
    digitalWrite(POW_LATCH1, LOW);
    //Setting state to ON
    digitalWrite(POW1, LOW);
    delay(1);
    // giving raising clock edge
    digitalWrite(POW_LATCH1, HIGH);
    delay(1);
    // lowering clock edge
    digitalWrite(POW_LATCH1, LOW);
    delay(5);

}

void loop()
{
    while(1);
}


