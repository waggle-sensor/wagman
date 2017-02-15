# Commands List
## Get / Set Date

Gets / sets the RTC date.

```sh
# gets the date
$ wagman-client date

# sets the date
$ wagman-client date 2016 03 15 13 00 00
```
## Get RTC

Gets the milliseconds since epoch from the RTC.

```sh
$ wagman-client rtc
```
## Get Wagman ID

Gets the onboard Wagman ID.

```sh
$ wagman-client id
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
