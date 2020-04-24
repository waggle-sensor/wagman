<!--
waggle_topic=wagman/introduction, Wagman: The Waggle Manager
-->

# WagMan: The Waggle Manager

<img src="./v3/resources/WagmanAnnotated.jpg" width="800">

The WagMan is a custom-designed embedded microcontroller board that manages power, monitors the operation of all system components, and
resolves many classes of faults. It uses a reliable Atmel ATMEGA32U4-MU with industrial operational range (-40 °C to 85 °C). The microcontroller is commonly used in Arduino-class boards.

The Waggle Manager (WagMan) board is our approach to providing enhanced resilience while also supporting Linux-based compute boards with extremely complex, and usually brittle, software stacks. In effect, the Linux boards become devices that can be managed.  Instead of focusing on making the full-featured embedded OS resilient, Waggle borrows a design pattern from NASA deep space probes and relies on the simplified and environmentally hardened WagMan to provide a safe mode that enables recovery and reprogramming of the compute boards.

WagMan employs internal sensor inputs that are out of band with respect to the Linux boards, including CPU temperature, power draw, and humidity of the enclosure. A simple digital signal between the Linux system and WagMan provides a software-driven heartbeat monitor. Sealed relays allow WagMan to completely manage electrical power to subsystems (including the Linux single-board computers), permitting sequential
power-up of subsystems to avoid power spikes or to electrically isolate and disconnect faulty boards.

WagMan can also manage boot image selection on the ODROID single-board computers. Each of the ODROIDs can boot from either an eMMC card (primary) or a microSD card (secondary), which are both installed. The selection is done via a board-level logic line that is routed to Wagman.  By using the heartbeat line and boot selector, WagMan becomes capable of sophisticated recovery schemes. For example, when the
heartbeat is lost, WagMan can try to reboot the node.  After three failed attempts, WagMan can select the backup (secondary) boot image. The backup boot image can then determine how to rebuild or reinstall the image on the primary boot media. This approach allows for a “safe mode” that can reformat the media and reinstall a known configuration.

## Firmware

The latest release of the firmware targetting new v4 Wagman boards can be found [here](https://github.com/waggle-sensor/wagman/tree/master/v4/new/firmware).

For archival purposes, the firmware for older v3 Wagman boards is kept [here](https://github.com/waggle-sensor/wagman/tree/master/v3/firmware).

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
