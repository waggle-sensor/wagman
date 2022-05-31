<!--
waggle_topic=wagman/introduction, Wagman: The Waggle Manager
-->

# Wagman: The Waggle Manager

![name of the image]<img src="./v3/resources/WagmanAnnotated.jpg" width="800">

The Wagman (Wagman V4 illustrated above) is a custom-designed embedded microcontroller board that manages power, monitors the operation of all system components, and
resolves many classes of faults. Versions 1-4 of Wagman used Atmel MCUs with industrial operational range (-40 °C to 85 °C). The microcontroller is commonly used in Arduino-class boards. Wagman V5 uses no programmable MCUs and is entirely implemented in hardware with no customizable firmware.

The Waggle Manager (Wagman) board is our approach to providing enhanced resilience while also supporting Linux-based compute boards with extremely complex, and usually brittle, software stacks. In effect, the Linux boards become devices that can be managed.  Instead of focusing on making the full-featured embedded OS resilient, Waggle borrows a design pattern from NASA deep space probes and relies on the simplified and environmentally hardened Wagman to provide a safe mode that enables recovery and reprogramming of the compute boards.

Wagman employs internal sensor inputs that are out of band with respect to the Linux boards, including CPU temperature, power draw, and humidity of the enclosure. A simple digital signal between the Linux system and Wagman provides a software-driven heartbeat monitor. Sealed relays allow WagMan to completely manage electrical power to subsystems (including the Linux single-board computers), permitting sequential
power-up of subsystems to avoid power spikes or to electrically isolate and disconnect faulty boards.

Wagman can also manage boot image selection on the ODROID single-board computers. Each of the ODROIDs can boot from either an eMMC card (primary) or a microSD card (secondary), which are both installed. The selection is done via a board-level logic line that is routed to Wagman.  By using the heartbeat line and boot selector, WagMan becomes capable of sophisticated recovery schemes. For example, when the
heartbeat is lost, WagMan can try to reboot the node.  After three failed attempts, WagMan can select the backup (secondary) boot image. The backup boot image can then determine how to rebuild or reinstall the image on the primary boot media. This approach allows for a “safe mode” that can reformat the media and reinstall a known configuration.

## Firmware

Wagman v5 does not incorporate any programmable components and does not hence have any firmware. For archival purposes, the firmware targetting v4 Wagman boards can be found [here](https://github.com/waggle-sensor/wagman/tree/master/boards/v4) and the firmware for older v3 Wagman boards is kept [here](https://github.com/waggle-sensor/wagman/tree/master/boards/v3).
