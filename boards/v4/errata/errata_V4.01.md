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
        
## Issues - 

### Wagman self-power cycle circuit :

This does not work out of the box due to bugs. After making the necessary 
changes by cutting and splicing circuits, the full functionality was 
verified to work. The current board is designed with a bypass for this portion of 
the functionality, and the 3 boards for further testing have been enabled with 
the by-pass. 

        * CAT872 (SWS003) - Current wiring limits usage and needs to be 
          corrected. The device should also be powered on using 5V bus instead of 
          3V bus. 
        * MC14541 (U25) - Reset Output needs to be inverted. Auto-reset should 
          be enabled for deterministic behavior. 
        * NL17SZ06 (U27) - Chip has I/O pins flipped, needs to be corrected. 
        
### Latching Circuit for Relays :
Latches are triggered automatically to ON position sometimes when the 
Atmel powers ON and assumes a high-impedance state on its PINs. This can be 
corrected using a weak pulldown on the CLK gates of the D-flipflops, and 
was tested on the boards practically and also looking at the clock lines on 
a logic probe. 

    
## To be Tested - 

    * 

## Required Fixes - 

    1. Remove Diode D5. Removes reverse polarity protection, but removes the 
      0.2V voltage drop. 
    2. 10K weak pulldown on all CLK lines into D-latches (U18,U19 and U20).
    3. 10uF Cap from PIN 2 to ground of U20 to prevent data input from raising 
      to logic before the first pulse from U26. 
    4. CAT872 (SWS003) - Connect MR1 and MR2 to Atmel, one to arm the 
      power-cycle circuit and the other to request a 13 second power-OFF of the Atmel. 
      Power device using 5V and not 3V. 
    5. MC14541 (U25) - AR to GND and Q Sel to VCC. 
    6. NL17SZ06 (U27) - Exchange input and output. 
    7. Remove R56 and R55. 
    8. ACS722LLCTR-05AB-T should be ACS722LLCTR-10AU-T. We do not need +/- 5A swing. 
    9. Replace power input part -  277-1721-ND instead of 277-1779-ND 
    
## Other-fixes and things to think about -

### NC-Reset-Wagman: 
The current implementation can allow the NC to indefinitely keep the 
Wagman-Atmel in a reset state with just one active operation. However, 
this both not ideal and also exposes us to the vulnerability of the system being 
wedged if the NC hangs after starting the single active-operation. The Wagman 
self-power cycle control chain (that has been corrected and tested) if 
replicated for Wagman-reset can prevent such a situation, as it requires a 
complete re-trigger process for every reset, 
needing continuous active operations to hold the Wagman in reset-state. 

### NC's ability to Power-cycle Wagman:

Currently the NC has the ability to power-cycle the Wagman in addition to being able 
to perform a Wagman-reset. In the next version of the board - 
 * Choose one of the two modes
 * Keep both with the option of choosing one of the two with a switch at Wagman 
   install time. 

    
