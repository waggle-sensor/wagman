<!--
waggle_topic=/wagman/wagman_v4/introduction, Wagman V4.0 Features
-->

# Wagman V4.0 Features and Capabilities

## Features

The WagMan V4 board is capable of managing up to 5 devices through power control,
power monitoring, heart-beat monitoring and device temperature monitoring. The
Wagman V4 version is a successor to the [Wagman V3](https://github.com/waggle-sensor/wagman/blob/develop/v3/README.md#wagman-layout-and-features) implemented in 2015-16 and was designed based on the features listed in the [RFC.](https://github.com/waggle-sensor/development/blob/master/WagMan_4.0_Discussion.md) Salient features of the new Wagman V4 include -

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
