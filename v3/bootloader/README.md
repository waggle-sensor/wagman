## WagMan Bootloader:

The WagMan design is centered around an Atmel [ATmega32U4](http://www.atmel.com/Images/Atmel-7766-8-bit-AVR-ATmega16U4-32U4_Datasheet.pdf) processor, and uses a [modified version](https://github.com/waggle-sensor/WagMan/tree/master/v3/bootloader/src/caterina) of the [Caterina](https://github.com/arduino/Arduino/tree/master/hardware/arduino/avr/bootloaders/caterina) bootloader installed in the [Arduino Micro](https://www.arduino.cc/en/Main/ArduinoBoardMicro) class of boards. The bootloader modifications are aimed toward improving the reliability of the in-situ WagMan firmware upgrade process by passing bootloader stage GPIO operation requests from firmware space using the EEPROM storage. This bootloader makes the WagMan incompatible with the regular Arduino IDE's USB firmware upgrade process due to change in timing-delays to accommodate the additional features.

## WagMan Firmware Upgrade:

The WagMan firmware upgrade is initiated by the Node controller. The WagMan needs to be put in the boot-loader stage for the upgrade process, which can be done in two ways -
  1. Auto-reset: Initiated by the Node-controller opens and closes the serial port to the WagMan at 1200.
  2. Forced-reset: Initiated by the Node-controller pulses the reset-line of the WagMan using the SRE board.

Once reset to the bootloader state where the WagMan stays for a duration of time before jumping into program execution phase, the WagMan firmware is uploaded by the Node-controller through the USB connection using the avrdude application.

## WagMan Firmware Upgrade Risk:

During the upgrade process the Node-controller is kept alive by the latched power supply and WagMan has no firmware beyond a working bootloader (until the new firmware is completely committed to the memory). A power failure at this point leads to a WagMan restricted to only the bootloader state and Node-controller that is permanently turned off. With **no scope for recovery** without physical access to the node, we are essentially left with a dead node.

## WagMan Firmware Upgrade with Node-controller Power-ON in Boot-loader State:

To mitigate the above risk, the boot-loader is modified to receive instructions from a working firmware. A working firmware can write instructions (option to power ON Node-controller and choice of eMMC or SD card) into the EEPROM which are preserved across soft/hard resets and also power failures. These instructions are read by the boot-loader and appropriate actions are performed. The new firmware-upgrade process is as follows -
  1. The WagMan [firmware](../firmware/Record.cpp), on the direction from Node-controller, enables bootloader stage Node-controller power-ON, along with the choice of the appropriate boot-media. This is written to location 0x40 in the EEPROM.
  2. The Node-controller initiates the firmware-upgrade using the Auto-reset process (1) ( the Forced-reset (2) is used as the backup process).
  3. The bootloader reads the memory location 0x40 and sets the right relay settings to choose the appropriate boot-media for Node-controller and forces a power ON. If the Node-controller is already powered-ON, this process has no effect on it. Otherwise, the Node-controller is powered-ON with the required boot-media.
  4. After 5 seconds, the WagMan goes into the standard Arduino-bootloader phase, ready to receive the firmware upgrade over the USB link.
  5. The Node-controller flashes the WagMan with firmware and the WagMan proceeds to program execution phase onces the firmware upgrade is completed.
  6. On successful firmware upgrade, the WagMan firmware resets the bootloader stage options in the EEPROM on commands from the Node-controller.

## Caveats:

All risks in firmware-upgrade process are not alleviated by the bootloader improvements. If the power-outage during the firmware upgrade phase wrecks the boot-media of the Node-controller, the bootloader does not have the abilities to sense the situation and try alternate boot-media. The limited bootloader space prevents further expansion of the bootloader features.

## Compiling Bootloader:

The bootloader compilation process [requires several software packages](http://www.leonardomiliani.com/en/2013/accorciamo-i-tempi-del-bootloader-della-leonardomicroesplora/) including [LUFA](https://github.com/abcminiuser/lufa), and avr-gcc toolchain. The LUFA packages and the appropriate [MakeFile](./src/caterina/Makefile) used to compile the code  on an x86-64 Ubuntu machine have been included in the [LUFA-111009](./LUFA-111009) and [src](./src/caterina/) directories. The current binary bootloader can be found in the [bin](./bin) directory.

# Flashing the Bootloader in Linux:

The currently suggested bootloader flashing process involves the [installbl script](../installbl) and AVRISP mkII programmer. In order for the mkII programmer to work in Linux, a udev rule *60-avrisp.rules* may be needed in the */etc/udev/rules.d* directory with the following entry. 

```bash
SUBSYSTEM!="usb", ACTION!="add", GOTO="avrisp_end"
# Atmel Corp. AVRISP mkII
ATTR{idVendor}=="03eb", ATTR{idProduct}=="2104", MODE="660", GROUP="tty"
LABEL="avrisp_end"
```
