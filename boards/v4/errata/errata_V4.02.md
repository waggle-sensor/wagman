## Wagman 4.0 Power up and Testing 

The Wagman was powered ON using 5V bench supply and connected to a test 
laptop over USB. 

## What works?
    * Atmel Powers ON and is recognized by the test-laptop. 
    * Auto power - ON of NC in 1 min
    * Latching of power state for all the other 4 ports
    * Wagman-reset
    * Wagman-erase
    * Atmel will reset when the Voltage drops below 4.65V.
    * NC can reset Wagman Atmel 
    * Sensors on Board:
        - HTU21D
        - Thermistors
        - Light-sensor
        - Current Sensors
        - Output Voltage sensing 
    * NC-boot prevention
    * SD card access 
    * Console Port
        - Communication 
        - Erase
        - Reset Wagman
    * RTC
    * EEPROM
    * Wagman-EERESET switch
    * Debug-LEDs
        

## Corrections Needed: 

1. `TESTED:` On U25, U40 and U37 --- Bring AR to 5V instead of GND. This removes power on reset of timer. 
This was testesd on board after modification at Surya. Fix - required, enables low-power operation also. PCB correction. 

2. `TESTED:` On U25 and U40 - Change C3 and C43 to 3nF. Reduces reset time to 10 seconds. Tested after modification at 
Surya. Change BOM. 

3. `TESTED:` On U37 - Change C1 to 150pF. Reduces erase to 450 ms. Tested at Surya. Change BOM. 

4. `TESTED:` Replace PC1, PC5 and PC2 to 330uF. Reduces the effect of voltage fluctuation when C1+ boots. This has been tested after replacement at Surya. Change BOM.

5. `UNTESTED:` Relace PC3 with 47uF. Reduces the effect of voltage fluctuation when C1+ boots. Risk and mitigation: 47uF is too high on 
             the low side of the 3.3V converter. But it can be easily replaced. Change BOM.

6. `UNTESTED:` On U20, pull 1D high through a 1M resistor, and pull to ground with 10uF cap. Risk and mitigation: 1M resistor prevents the 
D input of U20 from getting to 5V. We can replace it with a smaller value or 0ohm value, and get the current functionality. Side effect: If the 
device is unplugged and plugged back in under 60 seconds, the C1+ will be started immidiately. PCB correction.

7. `TESTED:` On U22, break spurious connecton to A8 and B8, which crossed S1RX and S2TX. This connection was cut at 
Argonne and tested to function properly. PCB correction.

8. `TESTED:` Break N$27 connection between R33/U18.1 and R60/U21.10. PCB correction.




