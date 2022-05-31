# Wagman V4 Development Notes

## LEDs

LED pinout ordered from `L13` to `L11` is:

```
12, 11, 2, 3, 5, 6, 7, 8, 9
```

## EEPROM

System uses [24LC256](https://www.microchip.com/wwwproducts/en/24LC256) EEPROM chip.

## Watchdog

By default, the Arduino DUE's core disables the watchdog on setup, as seen in
`packages/arduino/hardware/sam/1.6.11/cores/arduino/watchdog.cpp`.

```
extern "C"
void _watchdogDefaultSetup (void)
{
	WDT_Disable (WDT);
}
void watchdogSetup (void) __attribute__ ((weak, alias("_watchdogDefaultSetup")));
```

Unfortunately, the DUE's watchdog configuration is write-once per chip reset, so
you **must** to override the `watchdogSetup` function in your code to use the watchdog.

A minimal example is:

```
void watchdogSetup() {
    watchdogEnable(8000); // 8s watchdog
}

void setup() {
    // ...do setup...
}

void loop() {
    watchdogReset();

    // ...do some stuff...
}
```
