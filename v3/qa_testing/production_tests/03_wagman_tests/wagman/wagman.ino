#include "config.cpp"

#include <Wire.h>
#include <avr/wdt.h>

#include "./libs/MCP342X/MCP342X.h"
#include "./libs/HTU21D/HTU21D.h"
#include "./libs/MCP79412RTC/MCP79412RTC.h"    //http://github.com/JChristensen/MCP79412RTC
#include "./libs/Time/Time.h"


MCP342X mcp3428_1;
unsigned char WagID[8];
float htu21d_humd, htu21d_temp;
HTU21D myHumidity;
unsigned long time_this;
int loop_count = 0;
int question_no = 1;
int temp;

int HIH_final, HIH_init, Light_final, Light_init;
float HTU_hum_final, HTU_hum_init, HTU_temp_final, HTU_temp_init;



void setup()
{
    delay(1000);
    set_up_pinmodes();
    delay(1000);
    power_off_all();
    Serial.begin(115200);
    Wire.begin();
    delay(1000);
    mcp3428_1.init(MCP342X::H, MCP342X::H);
    myHumidity.begin();
}




void loop()

{
    Serial.println("------Wagman Environmental Sensors Test------");
    Serial.println("");
    WagID_print ();
    
    Serial.print("I. Wagman RTC Test: ");
    time_this = RTC.get();
    delay(1000);
    if ((RTC.get() - time_this) > 0)
    {
        Serial.println("PASS");
    }

    else
    {
        Serial.println("FAIL");
    }
    
    thermistor_check();
   
    
    HIH_init = analogRead(PIN_HIH4030_Sensor);
    Light_init = analogRead(PIN_Light_Sensor);
    HTU_hum_init = myHumidity.readHumidity();
    HTU_temp_init = myHumidity.readTemperature();
    
        
    Serial.println("\n");
    
    Serial.println("Please place your fingers in the three sensors now...");
    delay(7000);
    Serial.print("Counting down...");
    for (loop_count = 5; loop_count > 0; loop_count--)
    {
        Serial.print(loop_count-1);
        Serial.print(" ");
        delay(990);
    }
    delay(500);
    Serial.println("\n");
    
    
    HIH_final = analogRead(PIN_HIH4030_Sensor);
    Light_final = analogRead(PIN_Light_Sensor);
    HTU_hum_final = myHumidity.readHumidity();
    HTU_temp_final = myHumidity.readTemperature();
    
//     Serial.println(HIH_final);
//     Serial.println(HIH_init);
//     
//     Serial.println(HTU_hum_final);
//     Serial.println(HTU_hum_init);
//     
//     Serial.println(HTU_temp_final);
//     Serial.println(HTU_temp_init);
    
    if (HIH_final > (HIH_init - 5))
    {
         Serial.println("III. HIH4030: PASS");
    }
    else
    {
         Serial.println("III. HIH4030: FAIL");
    }
        
        
    if (Light_final < Light_init)
    {
         Serial.println("IV. LIGHT_SENSOR: PASS");
    }
    else
    {
         Serial.println("IV. LIGHT_SENSOR: FAIL");
    }
    
    if ((HTU_hum_final > (HTU_hum_init - 1.5)) && (HTU_temp_final > (HTU_temp_init) + 0.5))
    {
        Serial.println("V. HTU21D: PASS");
    }
    else
    {
         Serial.println("V. HTU21D: FAIL");
         
         
    }
    
    
    Serial.println("\n");
    Serial.println("Please turn right side up, and wait to hear the relays click...");
    delay(5000);
    Serial.println("\n");
    
    Serial.println("VI. Relay Test:");
    boot_pow_check();
    Serial.println("");
    
    
    Serial.println("VII. Wagman Boot Selector Tests. Please use a multimeter with the beeper enabled for the following tests.");
    Serial.println("");
    Serial.println("");
    Serial.println("------End of board test------");
    while(1)
    {
        boot_gm_usd();
        boot_nc_usd();
        delay(2000);
        boot_gm_emmc();
        boot_nc_emmc();
        delay(2000);

    }
    
}


