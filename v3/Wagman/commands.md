<!--
waggle_topic=Waggle/Node/Wagman
-->

# Wagman Commands

## Change Device Timeout Behavior

Change the timeout behavior for the heartbeat and current. Currently, only heartbeat
timeouts are supported.

```sh
# disable watching the coresense heartbeat
$ wagman-client watch 2 hb f

# enable watching the coresense heartbeat
$ wagman-client watch 2 hb t
```
## Disable Device

Disables a device from being powered on automatically. Note that this doesn't
immediately stop a device.

```sh
# disable the coresense
$ wagman-client disable 2
```
## Enable Device

Enables a device to be powered on automatically. Note that this doesn't
immediately start the device.

```sh
# enable the coresense
$ wagman-client enable 2
```
## Get / Set Date

Gets / sets the RTC date.

```sh
# gets the date
$ wagman-client date

# sets the date
$ wagman-client date 2016 03 15 13 00 00
```
## Get / Set Device Boot Media

Gets / sets the next boot media for a device. The possible boot media are `sd`
and `emmc`.

```sh
# gets the selected boot media for the node controller.
$ wagman-client bs 0

# set the selected boot media for the guest node to sd
$ wagman-client bs 1 sd

# set the selected boot media for the guest node to emmc
$ wagman-client bs 1 emmc
```
## Get Boot Flags



```sh
$ wagman-client bf
```
## Get Current Values



```sh
$ wagman-client cu
```
## Get Environment Sensor Values

Gets the onboard temperature and humidity sensor values.

```sh
$ wagman-client env
```
## Get Fail Counts

Gets the number of device failures for each device. Currently, this only includes
heartbeat timeouts.
Device0
Device1
...
Device4

```sh
$ wagman-client fc
```
## Get Heartbeats



```sh
$ wagman-client hb
```
## Get RTC

Gets the milliseconds since epoch from the RTC.

```sh
$ wagman-client rtc
```
## Get Thermistor Values



```sh
$ wagman-client th
```
## Get Uptime

Gets the system uptime in seconds.

```sh
$ wagman-client up
```
## Get Wagman Boot Count

Gets the number of times the system has booted.

```sh
$ wagman-client boots
```
## Get Wagman Bootloader Flag

Gets the bootloader stage boot / media flag for the node controller. The output
indicates whether the node controller will be started in the bootloader stage
and if so, with which boot media.

```sh
$ wagman-client blf
```
## Get Wagman ID

Gets the onboard Wagman ID.

```sh
$ wagman-client id
```
## Get Wagman Version

Gets the hardware / firmware versions of the system.

```sh
$ wagman-client ver
```
## Reset EEPROM

Requests that the system resets its persistent EEPROM on the *next* reboot.

```sh
# request a eeprom reset
$ wagman-client eereset
# reset the wagman
$ wagman-client reset
```
## Reset Wagman

Resets the Wagman with an optional delay.

```sh
# resets the wagman immediately
$ wagman-client reset

# resets the wagman after 90 seconds
$ wagman-client reset 90
```
## Send Ping

Sends an "external" heartbeat signal for a device.

```sh
# resets heartbeat timeout on device 1
$ wagman-client ping 1
```
## Start Device

Starts a device.

```sh
# start the node controller
$ wagman-client start 0

# start the guest node
$ wagman-client start 1
```
## Stop Device

Stops a device with an optional delay.

```sh
# stop guest node after 30 seconds
$ wagman-client stop 1 30

# stop guest node immediately
$ wagman-client stop 1 0
```
