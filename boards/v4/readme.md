<!--
waggle_topic=/wagman/wagman_v4/introduction, Wagman V4.0 Features
-->

# Wagman V4.0

<img src="https://raw.githubusercontent.com/waggle-sensor/wagman/master/boards/v4/resources/Wagman-v4.jpg" width="450"></br>

The Wagman [V4 board](https://raw.githubusercontent.com/waggle-sensor/wagman/master/boards/v4/resources/Wagman-v4.jpg) is capable of managing up to 5 devices through power control, power monitoring, heart-beat monitoring and device temperature monitoring. Built around more capable [SAM3X8E ARM Cortex-M3 CPU](https://www.microchip.com/en-us/product/ATSAM3X8E), the Wagman V4 version is a successor to the [Wagman V3](https://github.com/waggle-sensor/wagman/tree/master/boards/v3). It is designed to work with Odroid [C1+](https://www.hardkernel.com/shop/odroid-c1/) and [XU4](https://www.hardkernel.com/shop/odroid-xu4-special-price/) SBCs, providing special circuits for mnaging boot-media, and also powering devices across a wider power range. The board is built with a deadman trigger and MOSFET/Relay based power control. 


## Design Decisions

Implemented in 2015-16 and was designed to with the goal of incorporating features below divided into **H**)igh, **M**)edium, and **L**)ow.

**H**) Countdown timer on NC, so it will turn on without further involvement of the WagMan unless it is STOPPED from turning on before the timer expires

**H**) Replace all analog signaling that is subject to transients and slight voltage bias problems with reliable digital signaling (including SRE-Fix).  In some cases, signal could be serial line, in others two GPIO pins with appropriate logic/latch to trigger digital opto-isolators.

**H**) Discuss with Intel and other circuit board designers how PCB layout could be modified to manage heavy current draws, for example, turning on 40W Edge Processor.  Is different wiring requires to avoid loads traveling on PCB traces?  

**H**) Explore WagMan modifications to support 40W Intel NUC.  What are the tradeoffs to go even larger?  60W

**H**) Understand if brownouts are caused by PCB routing or insufficiently large source PS capacitors. Work to stabilize power supply, or provide better isolation

**H**) Bigger Flash memory / more powerful CPU (logs and program code)

**H**) Remove Schottky Diode for reverse polarity protection, add polarized connectors for wiring harness and simplified assembly connection headers.

**H**) Add support for 12V Edge Processor SBCs.  

**H**) Redesign headers/connectors so manufacturing is easy, with single harness to connect.  

**M**) Additional I/O pins that can be configured for out-of-band monitoring serial line, or connection to Particle.io Electron, etc.

**M**) Explore temporary countdown latch for relays, so Wagman can reboot without needing to power down all devices

**L**) Add 3 pushbuttons for GPIO pins. They can be used for testing, resetting to factory defaults, etc.  

**L**) Do we ever have situations where WagMan must be removed from power to recover operation?  How will this be solved?  Do we need a failsafe?

**L**) Add current and voltage diagnostic sensors to WagMan.

**L**) Add LEDs. Decide what LEDs might be modified/changed for debugging/status.  Which LEDs will be programmable (can be turned on an off via firmware) and which are triggered directly by logic levels (flickering heartbeat, power, etc.)


## Salient Features 

* *Built around more capable Atmel SAM3X8E ARM Cortex-M3 CPU*
* *On board 32K EEPROM*
* *On board uSD storage slot*
* *On board RTC*
* *Supervisory circuits to safely reset Wagman during power glitches*
* *SAM3X8E able to trigger self-power-cycle*
* *Ability to power-down SAM3X8E and associated circuits completely from Node Controller and reboot*
* *Ability to soft-reset SAM3X8E from Node Controller*
* *Improved power distribution*
* *UART Console and Programming Port for debugging and out-of-band wireless interaction*
* *Support for devices with multiple input voltages and logic-levels*
* *Voltage and Current sensing on Wagman's main power supply and individual power lines to all connected devices*
* *Thermal monitoring of all 5 connected devices*
* *On board temperature, humidity and light sensor for local environmental monitoring*
* *Full UART heartbeat capability on all 5 managed channels*
* *Node Control dead-man trigger*
* *Latched relays for uninterrupted power to devices during Wagman reboot*
* *10 LEDS for visual feedback*
* *GPIO port with 5 I/O lines*

## Toolchain Setup

The new Wagman requires a SAM board entry:

```txt
wagman_v2.name=Wagman v2
wagman_v2.vid.0=0x2341
wagman_v2.pid.0=0x003f
wagman_v2.vid.1=0x2A03
wagman_v2.pid.1=0x003f
wagman_v2.upload.tool=bossac
wagman_v2.upload.protocol=sam-ba
wagman_v2.upload.maximum_size=524288
wagman_v2.upload.use_1200bps_touch=true
wagman_v2.upload.wait_for_upload_port=true
wagman_v2.upload.native_usb=true
wagman_v2.build.mcu=cortex-m3
wagman_v2.build.f_cpu=84000000L
wagman_v2.build.usb_manufacturer="Waggle Team"
wagman_v2.build.usb_product="Wagman v2"
wagman_v2.build.board=SAM_DUE
wagman_v2.build.core=arduino
wagman_v2.build.extra_flags=-D__SAM3X8E__ -mthumb {build.usb_flags}
wagman_v2.build.ldscript=linker_scripts/gcc/flash.ld
wagman_v2.build.variant=arduino_due_x
wagman_v2.build.variant_system_lib=libsam_sam3x8e_gcc_rel.a
wagman_v2.build.vid=0x2341
wagman_v2.build.pid=0x003f
```

## Code

The bleeding edge code base is located [here](https://github.com/waggle-sensor/wagman/tree/master/v4/develop/firmware).
