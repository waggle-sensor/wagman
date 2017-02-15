# Command: Get / Set Date

## Description

Gets / sets the RTC date.

## Examples

```sh
# gets the date
$ wagman-client date

# sets the date
$ wagman-client date 2016 03 15 13 00 00
```
# Command: Get RTC

## Description

Gets the milliseconds since epoch from the RTC.

## Examples

```sh
$ wagman-client rtc
```
# Command: Get Wagman ID

## Description

Gets the onboard Wagman ID.

## Examples

```sh
$ wagman-client id
```
# Command: Reset Wagman

## Description

Resets the Wagman with an optional delay.

## Examples

```sh
# resets the wagman immediately
$ wagman-client reset

# resets the wagman after 90 seconds
$ wagman-client reset 90
```
# Command: Send Ping

## Description

Sends an "external" heartbeat signal for a device.

## Examples

```sh
# resets heartbeat timeout on device 1
$ wagman-client ping 1
```
# Command: Start Device

## Description

Starts a device.

## Examples

```sh
# start the node controller
$ wagman-client start 0

# start the guest node
$ wagman-client start 1
```
# Command: Stop Device

## Description

Stops a device with an optional delay.

## Examples

```sh
# stop guest node after 30 seconds
$ wagman-client stop 1 30

# stop guest node immediately
$ wagman-client stop 1 0
```
