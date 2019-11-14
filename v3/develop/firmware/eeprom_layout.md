<!--
waggle_topic=/wagman/wagman_v3/EEPROM_Layout, Wagman V3.0 EEPROM Reference
-->

# Wagman EEPROM Reference

## Header Region

* `offset = 0`
* `length = 128`

```
0 magic uint32
4 hw ver [2]byte
6 fw ver [2]byte
8 wagman boot count uint32
12 last boot time uint32
32 wire bus enabled byte
64 nc bootloader mode byte
```


# Wagman Region

* `offset = 128`
* `length = 128`

```
0 last boot time uint32
4 htu21d enabled byte
5 htu21d range [2]int16

9 hih4030 enabled byte
10 hih4030 range [2,2]

14 hih4030 enabled byte
15 hih4030 range [2,2]
```

## Device Region

* `offset = 256 + 128 * port`
* `length = 128`

```
0 device enabled byte
1 device managed byte
2 device default boot media

3 device last boot time uint32
7 device boot attempts uint32
11 device boot failures uint32

15 thermistor enabled byte
16 thermistor range [2]uint16

20 current sensor enabled byte
21 current sensor range [2]uint16
25 current sensor fault level uint16

21 current sensor low level uint16
23 current sensor normal level uint16
25 current sensor stressed level uint16
27 current sensor high level uint16

29 current fault timeout uint32
33 current heartbeat timeout uint32

37 relay enabled byte
38 relay journal byte
64 boot log (Apparently I did implement a boot log for each device... with room to support backing off from killing the device to often. I didn't even remember implementing this until now.)
```

## Boot Logs

Boot logs are persisted in EEPROM as a ring buffer of up to 8 uint32 entries.

```
                 count
                   v
---]           [-------- ...
[ tn | _ | _ | t1 | t2 | ...
                ^
              start
```

### Memory Layout

```
start byte
count byte
values [8]uint32
```
