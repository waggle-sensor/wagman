<!--
waggle_topic=wagman/wagman_v3,Wagman Version 3.0
-->


# WagMan Firmware Layout and Features

The WagMan V3.1 board is capable of managing upto 5 devices through power control, power monitoring,
heart-beat monitoring and device temperature monitoring. The figure below shows a WagMan with annotations
pointing out the various capabilities, sensors and connectors on board.

## Getting Started

The WagMan V3.1 device uses two unique pieces of code for accomplishing its tasks - bootloader and firmware. To use a WagMan
in a Waggle node, both the pieces of code must be flashed onto the board. It is recommended to use the precompiled binary
bootloader and firmware files found in this repo, following the steps below -

### 1. [Bootloader:](./bootloader/)

  The bootloader is installed first on the WagMan. The bootloader is not field upgradable.

  * Needs: [AVRISP mkII programmer](http://www.atmel.com/tools/avrispmkii.aspx), Linux Computer with [avrdude](http://www.nongnu.org/avrdude/).

  * Preperation:
    - For the mkII programmer to function properly in Linux, a udev rule 60-avrisp.rules may be needed in the /etc/udev/rules.d directory with the following entry.

    ```bash
    SUBSYSTEM!="usb", ACTION!="add", GOTO="avrisp_end"
    # Atmel Corp. AVRISP mkII
    ATTR{idVendor}=="03eb", ATTR{idProduct}=="2104", MODE="660", GROUP="tty"
    LABEL="avrisp_end"
    ```


  * Process:
    - Power on Wagman by connecting the 5V DC power as shown in figure below. Red is +5V and Black is ground. </br>
      <img src="./qa_testing/design_dev_tests/Wagman_Initialization/resources/power_connect.jpg" width="220">
    - The Yellow "ON" LED on the WagMan should light up. </br>
      <img src="./qa_testing/design_dev_tests/Wagman_Initialization/resources/coin_cell_battery_debug_LED.jpg" width="220">
    - Connect the Atmel AVRISP mkII to the Linux computer and connect it to J2 of Wagman as shown below. </br>
      <img src="./qa_testing/design_dev_tests/Wagman_Initialization/resources/avrisp_connect.jpg" width="220">
    - Install the bootloader using the command installbl
      ```bash
      ./installbl
      ```
    - Unplug the AVRISP from the WagMan on successful install of bootloader.

  ### 2. [Firmware:](./firmware/)

The firmware install follows the bootloader. Please leave only one WagMan (and no Arduino Micro class devices) plugged
into the programming computer when installing the firmware. The WagMan firware is field Upgradable.

* Needs: Micro-USB cable, Linux Computer with [avrdude](http://www.nongnu.org/avrdude/).

* Preperation:
  - Onetime setup: Place udev rule 75-waggle-arduino.rules in the /etc/udev/rules.d directory with the following entry.

  ```bash
  SUBSYSTEM!="usb", ACTION!="add", GOTO="add_rules_end"
  SUBSYSTEM=="tty", KERNEL=="ttyACM[0-9]*", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="0037", SYMLINK+="waggle_sysmon"
  SUBSYSTEM=="tty", KERNEL=="ttyACM[0-9]*", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="8037", SYMLINK+="waggle_sysmon"
  LABEL="add_rules_end"
  ```


* Process:
  - Power on Wagman by connecting the 5V DC power as described above.
  - Plug the Micro-USB cable into the Linux computer and connect it to Micro-USB port of the Wagman.
  - Install the firmware using the command installfw
  ```bash
  ./installfw
  ```


### 3. WagMan (Re)Initialize:

  Initialize the WagMan using the following command wminit-
  ```bash
  ./wminit
  ```


## WagMan Layout and Features

The WagMan V3.1 board is capable of managing upto 5 devices through power control, power monitoring, heart-beat monitoring and device temperature monitoring. The figure below shows a WagMan with annotations pointing out the various capabilities, sensors and connectors on board.


<img src="./resources/WagmanAnnotated.jpg" width="800">

## Onboard Sensors

The WagMan V3.1 board has a set of on-board sensors that include
  * HT21D - Digital Temperature and Humidity Sensor
  * HIH4030 - Analog Humidity Sensor
  * ACS764 Current Sensors, one sensing WagMan's utilization and five sensing utilizationg by other connected devices
  * Thermistors for sensing temperature of connected devices
  * An Opto-resistor to provide a measure of brightness in the vicinity of the WagMan.

## Time Keeping and Unique ID

In addition to the above sensors, the WagMan can keep time using a real-time clock chip - MCP79412,
which also provides the WagMan with a 6-byte unique ID.

## Power Distribution, Control and Metering



There are 5 ports on the WagMan numbered Port 1 to 5 with the following features
  * Each port has one 5V power output, one Thermistor input (2 pins) and one heart-beat input. An optional
  reset pin can be used to reset a device (by toggling the line) powered by an external source. The reset
  feature and 5V power cannot be used at the same time on a port.
  * The power output of a port can be turned On or Off by the WagMan.
  * The current consumed from each port can be sensed by the WagMan, accurate to 16mA.
  * The 5 output ports, Port 1 to Port 5, are uniquely configured on V3.1 board as follows
   * Port 1: Reserved for a C1+ based Node-Controller. The heart-beat line is designed to accomodate 3.3V logic.
   It is uniquely configured to maintain power state across Atmel 32U4 resets so that the WagMan can reprogrammed by the Node-Controller in the field.
   * Port 2: Reserved for a XU4 based Guest Module. The heart-beat line is designed to accomodate 1.8V logic.
   * Port 3,4,5: Can be connected to any device that takes 5V power, consumes less than 2.5A of current and can support 5V logic for heart-beat and reset lines.

## Odroid Specific Features

The WagMan is configured to be able to support boot media switch on two connected Odroid single board computers as follows
  * __C1+ Boot Switch__: This is a two pin connector that is bridged in the default state to choose uSD as the default boot
  media of the C1+ Node-Controller. The two pins can be disconnected by the Atmel 32U4 Processor to choose the eMMC boot media.
  * __XU4 Boot Switch__: This is a three pin connector where the center pin (PIN 2) can be bridged to either the right (PIN 1)
  or left (PIN 3) pins. The default state bridges PIN 1 and PIN 2, with PIN 3 isolated to choose uSD boot media on the connected XU4, and the
  Atmel 32U4 processor can bridge PIN 2 and PIN 3, isolating PIN 1 to choose eMMC boot media.

## Feedback LEDs
The WagMan board has 5 status LEDs with the following features
  * A On LED stays on if the board is receiving 5V DC power.
  * RX LED that blinks to signify input of data through the USB port to the board.
  * TX LED that blinks to signify output of data from the board through the USB port.
  * LEDs L and L1 - These can be brightness controlled (connected to PWM port) by the Atmel 32U4 to provide state information.

## Inputs and Outputs
 The WagMan board has several power and communication Inputs and Outputs as listed below

 * Power
   - Power Input - 5V, 10A max.
   - 5 Power Outputs - 5V, 2.5A max.

 * Communication
   - USB Interface - USB CDC communication port to connect to Node-Controller
   - ISP Interface - For loading Bootloader on Atmel 32U4 using AVRISP MkII programmer. Post boot-loader flashing phase, the SRE V1 board is connected to this port for enabling Atmel 32U4 reset by Node-Controller.

 * Health Status
   - Heart-beat inputs - five ports that can sense a heart-beat signal represented by toggle between GND and Logic high.

 * Sensors-
   - Thermistor inputs - five inputs to sense temperature

 * Control
   - Reset outputs - five lines for devices that can be reset by toggling the line. The lines operate at 5V logic and cannot be used simultaneously with power delivery from a port.
