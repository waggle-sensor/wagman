#include <avr/wdt.h>
#include <EEPROM.h>
#include "Wagman.h"
#include "Record.h"
#include "Device.h"
#include "Logger.h"
#include "MCP79412RTC.h"

// TODO Speedup communications. Not bad now, but could be faster?
// TODO Add error detection to communications. Can still easily write a front end to talk using protocol.
// TODO Cleanup, really think about communication pathway. Does this allow us to reduce anything from the core.
// TODO Add persistant event logging on using system hardware so we can trace what happened
//      during a device failure.
// TODO look at Serial write buffer overflow lock up.
// TODO Rearrange to better reflect state diagram. (Or update the state diagram.)
// TODO Add a short "manage" message to tell Wagman we're bringing down a device.
// TODO Add a "die" command to tell wagman to stop...we're about to reset it.
// TODO Add temperature / humiditity readouts. Or `environment` command.
// TODO Add a delay to killing a device which consumes low power. We don't want to be too aggresive on this.
// For example, it may just be the case that a device is resetting.
// On the other hand, we should keep track of how frequently a device is resetting. We don't want it to get
// stuck in an endless reboot cycle.
// TODO Check the start flags to see if we can differentiate between different start conditions.
// This will possibly help us correctly decide on whether we start up with a self-test, a check
// for why we got locked up, etc.
// TODO Check that system current reading is correct.

// TODO minimize int types (int -> byte / char when possible)
// TODO check if WDR shows up as NC serial dev change.
// TODO add variable LED to encode states.

static const byte DEVICE_COUNT = 5;
static const byte BUFFER_SIZE = 80;
static const byte MAX_ARGC = 8;

byte bootflags;
bool resetWagman = false;
bool logging = true;
byte deviceWantsStart = 255;

Device devices[5];

static Timer startTimer;
static char buffer[BUFFER_SIZE];
static byte bufferSize = 0;

time_t setupTime;

bool isspace(char c);
bool isgraph(char c);

struct Command {
    const char *name;
    void (*func)(byte, const char **);
};

void commandStart(byte argc, const char **argv);
void commandStop(byte argc, const char **argv);
void commandKill(byte argc, const char **argv);
void commandReset(byte argc, const char **argv);
void commandPing(byte argc, const char **argv);
void commandID(byte argc, const char **argv);
void commandEEDump(byte argc, const char **argv);
void commandDate(byte argc, const char **argv);
void commandCurrent(byte argc, const char **argv);
void commandHeartbeat(byte argc, const char **argv);
void commandThermistor(byte argc, const char **argv);
void commandEnvironment(byte argc, const char **argv);
void commandBootMedia(byte argc, const char **argv);
void commandFailCount(byte argc, const char **argv);
void commandLog(byte argc, const char **argv);
void commandBootFlags(byte argc, const char **argv);
void commandUptime(byte argc, const char **argv);
void commandHelp(byte argc, const char **argv);

Command commands[] = {
    { "ping", commandPing },
    { "start", commandStart },
    { "stop", commandStop },
    { "stop!", commandKill },
    { "reset", commandReset },
    { "id", commandID },
    { "cu", commandCurrent },
    { "hb", commandHeartbeat },
    { "env", commandEnvironment},
    { "bs", commandBootMedia },
    { "th", commandThermistor },
    { "date", commandDate },
    { "eedump", commandEEDump },
    { "bf", commandBootFlags },
    { "fc", commandFailCount },
    { "uptime", commandUptime },
    { "up", commandUptime },
    { "help", commandHelp },
    { "log", commandLog },
    { NULL, NULL },
};

void commandPing(byte argc, const char **argv)
{    
    for (byte i = 1; i < argc; i++) {
        byte index = atoi(argv[i]);

        if (Wagman::validPort(index)) {
            devices[index].sendExternalHeartbeat();
        }
    }

    Serial.println("pong");
}

void commandStart(__attribute__ ((unused)) byte argc, const char **argv)
{
    byte index = atoi(argv[1]);

    if (Wagman::validPort(index)) {
        deviceWantsStart = index;
    }
}

void commandStop(byte argc, const char **argv)
{
    for (byte i = 1; i < argc; i++) {
        byte index = atoi(argv[i]);

        if (Wagman::validPort(index)) {
            devices[index].stop();
        } else {
            Serial.println("invalid device");
        }

        delay(500);
    }
}

void commandKill(byte argc, const char **argv)
{
    for (byte i = 1; i < argc; i++) {
        byte index = atoi(argv[i]);

        if (Wagman::validPort(index)) {
            devices[index].kill();
        } else {
            Serial.println("invalid device");
        }
    }
}

void commandReset(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    resetWagman = true;
}

void commandID(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    byte id[8];

    RTC.idRead(id);

    for (byte i = 0; i < 8; i++) {
        Serial.print(id[i] & 0x0F, HEX);
        Serial.print(id[i] >> 8, HEX);
    }

    Serial.println();
}

void commandEEDump(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    for (unsigned int i = 0; i < 1024; i++) {
        byte value = EEPROM.read(i);
        
        Serial.print((value >> 4) & 0x0F, HEX);
        Serial.print(value & 0x0F, HEX);
        Serial.print(' ');

        if (i % 4 == 3)
            Serial.print(' ');
        
        if (i % 32 == 31)
            Serial.println();
    }
}

void commandDate(byte argc, const char **argv)
{
    tmElements_t tm;

    if (argc == 1) {
        RTC.read(tm);
        Serial.print(tm.Year + 1970);
        Serial.print('-');
        Serial.print(tm.Month);
        Serial.print('-');
        Serial.print(tm.Day);
        Serial.print(' ');
        Serial.print(tm.Hour);
        Serial.print(':');
        Serial.print(tm.Minute);
        Serial.print(':');
        Serial.print(tm.Second);
        Serial.println();
    } else {
        tmElements_t tm;

        tm.Year = atoi(argv[1]) - 1970;
        tm.Month = (argc >= 3) ? atoi(argv[2]) : 0;
        tm.Day = (argc >= 4) ? atoi(argv[3]) : 0;
        tm.Hour = (argc >= 5) ? atoi(argv[4]) : 0;
        tm.Minute = (argc >= 6) ? atoi(argv[5]) : 0;
        tm.Second = (argc >= 7) ? atoi(argv[6]) : 0;
    
        RTC.set(makeTime(tm));
    }
}

void commandCurrent(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    Serial.println(Wagman::getCurrent());
    
    for (byte i = 0; i < DEVICE_COUNT; i++) {
        Serial.println(Wagman::getCurrent(i));
    }
}

void commandHeartbeat(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    for (byte i = 0; i < DEVICE_COUNT; i++) {
        Serial.print(devices[i].timeSinceHeartbeat());

        if (devices[i].watchHeartbeat) {
            Serial.print(" *");
        }

        Serial.println();
    }
}

void commandFailCount(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    for (byte i = 0; i < DEVICE_COUNT; i++) {
        Serial.println(Record::getBootFailures(i));
    }
}

void commandThermistor(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    for (byte i = 0; i < DEVICE_COUNT; i++) {
        Serial.println(Wagman::getThermistor(i));
    }
}

void commandEnvironment(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    Serial.print("temperature=");
    Serial.println(Wagman::getTemperature());
    
    Serial.print("humidity=");
    Serial.println(Wagman::getHumidity());
}

void commandBootMedia(byte argc, const char **argv)
{
    if (argc < 2)
        return;

    byte index = atoi(argv[1]);

    if (!Wagman::validPort(index))
        return;

    if (argc == 3) {
        if (strcmp(argv[2], "sd") == 0) {
            devices[index].shouldForceBootMedia = true;
            devices[index].forceBootMedia = MEDIA_SD;
            Serial.println("set sd");
        } else if (strcmp(argv[2], "emmc") == 0) {
            devices[index].shouldForceBootMedia = true;
            devices[index].forceBootMedia = MEDIA_EMMC;
            Serial.println("set emmc");
        } else {
            Serial.println("invalid media");
        }
    } else if (argc == 2) {
        byte bootMedia = devices[index].getBootMedia();

        if (bootMedia == MEDIA_SD) {
            Serial.println("sd");
        } else if (bootMedia == MEDIA_EMMC) {
            Serial.println("emmc");
        } else {
            Serial.println("invalid media");
        }
    }
}

void commandLog(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    logging = !logging;
    Serial.println(logging ? "on" : "off");
}

void commandBootFlags(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    if (bootflags & _BV(WDRF))
        Serial.println("WDRF");
    if (bootflags & _BV(BORF))
        Serial.println("BORF");
    if (bootflags & _BV(EXTRF))
        Serial.println("EXTRF");
    if (bootflags & _BV(PORF))
        Serial.println("PORF");
}

void commandUptime(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    Serial.println(Wagman::getTime() - setupTime);
}

void commandHelp(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    for (byte i = 0; commands[i].name != NULL; i++) {
        Serial.println(commands[i].name);
    }
}

void executeCommand(const char *sid, byte argc, const char **argv)
{
    void (*func)(byte, const char **) = NULL;

    // search for command and execute if found
    for (byte i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, argv[0]) == 0) {
            func = commands[i].func;
            break;
        }
    }

    // marks the beginning of a response packet.
    Serial.print("<<<- sid=");
    Serial.print(sid);
    Serial.print(' ');
    Serial.println(argv[0]);

    if (func != NULL) {
        func(argc, argv);
    } else {
        Serial.println("command not found");
    }

    // marks the end of a response packet.
    Serial.println("->>>");
}

// For ASCII Encoded
bool isspace(char c) {
    return c == ' ' || c == '\t';
}

// For ASCII Encoded
bool isgraph(char c) {
    return '!' <= c && c <= '~';
}

void processCommand()
{
    byte argc = 0;
    const char *argv[MAX_ARGC];
    
    char *s = buffer;

    while (argc < MAX_ARGC) {

        // skip whitespace
        while (isspace(*s)) {
            s++;
        }

        // check if next character is an argument
        if (isgraph(*s)) {

            // mark start of argument.
            argv[argc++] = s;

            // scan remainder of argument.
            while (isgraph(*s)) {
                s++;
            }
        }

        // end of buffer?
        if (*s == '\0') {
            break;
        }

        // mark end of argument.
        *s++ = '\0';
    }

    if (argc > 0) {
        wdt_reset();

        if (argv[0][0] == '@') {
            executeCommand(argv[0] + 1, argc - 1, argv + 1);
        } else {
            executeCommand("0", argc, argv);
        }
    }
}

void processCommands()
{
    byte dataread = 0; // using this instead of a timer to reduce delay in processing byte stream

    while (Serial.available() > 0 && dataread < 240) {
        char c = Serial.read();

        buffer[bufferSize++] = c;
        dataread++;

        if (bufferSize >= BUFFER_SIZE) { // exceeds buffer! dangerous!
            bufferSize = 0;
            
            // flush remainder of line.
            while (Serial.available() > 0 && dataread < 240) {
                if (Serial.read() == '\n')
                    break;
                dataread++;
            }
        } else if (c == '\n') {
            buffer[bufferSize] = '\0';
            bufferSize = 0;
            processCommand();
        }
    }
}

void setup()
{
    // save and clear boot flags
    bootflags = MCUSR;
    MCUSR = 0;
    
    wdt_disable(); // do we really want this?

    delay(2000);
    
    wdt_enable(WDTO_8S);
    wdt_reset();
    
    Wagman::init();

    // blink sequence indicating that we got through the first step.
    for (byte i = 0; i < 5; i++) {
        wdt_reset();
        Wagman::setLED(0, true);
        delay(200);
        Wagman::setLED(0, false);
        delay(200);

        wdt_reset();
        Wagman::setLED(0, true);
        delay(200);
        Wagman::setLED(0, false);
        delay(400);
    }

    wdt_reset();
    Record::init();

    wdt_reset();
    Serial.begin(57600);

    devices[0].name = "nc"; // move this into EEPROM? / rename to something other than name?
    devices[0].port = 0;
    devices[0].bootSelector = 0;
    devices[0].primaryMedia = MEDIA_SD;
    devices[0].secondaryMedia = MEDIA_EMMC;
    devices[0].watchHeartbeat = true;
    devices[0].watchCurrent = true;

    devices[1].name = "gn"; // move this into EEPROM?
    devices[1].port = 1;
    devices[1].bootSelector = 1;
    devices[1].primaryMedia = MEDIA_EMMC;
    devices[1].secondaryMedia = MEDIA_SD;
    devices[1].watchHeartbeat = true;
    devices[1].watchCurrent = true;

    devices[2].name = "coresense";
    devices[2].port = 2;
    devices[2].watchHeartbeat = false;
    devices[2].watchCurrent = false;
    
    devices[3].name = "unused1";
    devices[3].port = 3;
    devices[3].watchHeartbeat = false;
    devices[3].watchCurrent = false;
    
    devices[4].name = "unused2";
    devices[4].port = 4;
    devices[4].watchHeartbeat = false;
    devices[4].watchCurrent = false;

    setupTime = Wagman::getTime(); // used for uptime
    Record::setLastBootTime(setupTime);
    Record::incrementBootCount();

    // boot up after brown out.
    if (bootflags & _BV(BORF)) {
        // what would we do differently than a power-off reset flag.
    }

    // boot up after power on. this is a case where it'd be good to do a self test, current range check and device connected check.
    if (bootflags & _BV(PORF)) {
    }

    // tripped by external reset? (do we do something similar to a watchdog reset here?)
    if ((bootflags & _BV(EXTRF)) || (bootflags & _BV(WDRF))) {
    }

    for (byte i = 0; i < DEVICE_COUNT; i++) {
        devices[i].init();
    }

    // set the node controller as starting device
    deviceWantsStart = 0;
    startTimer.reset();

    // reinitialize globals just in case...
    resetWagman = false; // used to reset Wagman in main loop
    bufferSize = 0;
}

void startNextDevice()
{
    // if we've asked for a specific device, start that device.
    if (Wagman::validPort(deviceWantsStart)) {
        startTimer.reset();
        devices[deviceWantsStart].start();
        deviceWantsStart = 255;
        return;
    }

    if (startTimer.exceeds(30000)) {
        startTimer.reset();

        for (byte i = 0; i < DEVICE_COUNT; i++) {
            if (devices[i].canStart()) { // include !started in canStart() call.
                devices[i].start();
                break;
            }
        }
    }
}

void loop()
{
    // ensure that the watchdog is always enabled (in case some anomaly disables it)
    wdt_enable(WDTO_8S);

    wdt_reset();
    startNextDevice();

    wdt_reset();
    for (byte i = 0; i < 3; i++) {
        devices[i].update();
    }

    wdt_reset();
    processCommands();

    wdt_reset();

    if (resetWagman) {
        // 1. store "about to reset" marker
        // 2. tell other devices they're about to lose power
        // 3. 
        
        // watchdog will reset in a few seconds.
        for (;;) {
            Wagman::setLED(1, true);
            delay(200);
            Wagman::setLED(1, false);
            delay(200);
    
            Wagman::setLED(1, true);
            delay(200);
            Wagman::setLED(1, false);
            delay(400);
        }
    }

    wdt_reset();

    delay(100); // check if saves power
}

