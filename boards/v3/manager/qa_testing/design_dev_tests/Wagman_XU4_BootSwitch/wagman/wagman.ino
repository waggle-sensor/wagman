#include "config.cpp"

#include <Wire.h>
#include <avr/wdt.h>

#include "./libs/MCP79412RTC/MCP79412RTC.h"    //http://github.com/JChristensen/MCP79412RTC
#include "./libs/Time/Time.h"


unsigned char WagID[8];
int question_no = 1;

void setup()
{
    delay(1000);
    set_up_pinmodes();
    delay(1000);
    Serial.begin(115200);
    Wire.begin();
    delay(1000);
}




void loop()

{
    Serial.println("------Wagman XU4 Boot Selector Test------");
    WagID_print ();
    delay(1000);
    xu4_boot_selector_test();
    Serial.println(" ");
    Serial.println("------End of board test------");

    while(1)
    {
        boot_gm_usd();
        delay(1000);
        boot_gm_emmc();
        delay(1000);
    }
}


