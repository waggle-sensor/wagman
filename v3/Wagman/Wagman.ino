#include <avr/wdt.h>
#include <EEPROM.h>
#include "Wagman.h"
#include "Record.h"
#include "Device.h"
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
// TODO Observed in the logs that current checking is too aggresive. Use timeout as mentioned above.
// TODO may want to occasionally send a ping to wake?
// TODO keepalive 1 T/F

#define SECONDS(x) ((unsigned long)(1000 * (x)))
#define MINUTES(x) ((unsigned long)(60000 * (x)))
#define HOURS(x) ((unsigned long)(3600000 * (x)))

bool resetWagman = false;
bool logging = false;
int poweronCurrent[5];

static const byte DEVICE_NC = 0;
static const byte DEVICE_GN = 1;

static unsigned long lastStartTime = 0;

static byte bootflags;

Device devices[5];
unsigned long lastDeviceStartTime = 0;

struct Command {
    const char *name;
    void (*func)(int, const char **);
};

void commandStart(int argc, const char **argv);
void commandStop(int argc, const char **argv);
void commandKill(int argc, const char **argv);
void commandReset(int argc, const char **argv);
void commandPing(int argc, const char **argv);
void commandID(int argc, const char **argv);
void commandEEDump(int argc, const char **argv);
void commandDate(int argc, const char **argv);
void commandCurrent(int argc, const char **argv);
void commandHeartbeat(int argc, const char **argv);
void commandThermistor(int argc, const char **argv);
void commandEnvironment(int argc, const char **argv);
void commandBootMedia(int argc, const char **argv);
void commandLog(int argc, const char **argv);
void commandBootFlags(int argc, const char **argv);
void commandHelp(int argc, const char **argv);

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
    { "help", commandHelp },
    { "log", commandLog },
    { NULL, NULL },
};

void commandPing(int argc, const char **argv)
{
    Serial.println("pong");
}

void commandStart(int argc, const char **argv)
{
    for (int i = 1; i < argc; i++) {
        int index = atoi(argv[i]);

        if (index == 0) {
            Serial.println("what?");
        } else if (0 < index && index < 5) {
            devices[index].start();
        } else {
            Serial.println("invalid device");
        }

        delay(500);
    }
}

void commandStop(int argc, const char **argv)
{
    for (int i = 1; i < argc; i++) {
        int index = atoi(argv[i]);

        if (index == 0) {
            Serial.println("no...");
        } else if (0 < index && index < 5) {
            devices[index].stop();
        } else {
            Serial.println("invalid device");
        }

        delay(500);
    }
}

void commandKill(int argc, const char **argv)
{
    for (int i = 1; i < argc; i++) {
        int index = atoi(argv[i]);

        if (index == 0) {
            Serial.println("no...");
        } else if (0 < index && index < 5) {
            devices[index].kill();
        } else {
            Serial.println("invalid device");
        }
    }
}

void commandReset(int argc, const char **argv)
{
    resetWagman = true;
}

void commandID(int argc, const char **argv)
{
    byte id[8];

    RTC.idRead(id);

    for (int i = 0; i < 8; i++) {
        Serial.print(id[i] & 0x0F, HEX);
        Serial.print(id[i] >> 8, HEX);
    }

    Serial.println();
}

void commandEEDump(int argc, const char **argv)
{
    for (int i = 0; i < 1024; i++) {
        byte value = EEPROM.read(i);
        
        Serial.print(value & 0x0F, HEX);
        Serial.print(value >> 8, HEX);
        Serial.print(' ');

        if (i % 4 == 3)
            Serial.print(' ');
        
        if (i % 32 == 31)
            Serial.println();
    }
}

void commandDate(int argc, const char **argv)
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

void commandCurrent(int argc, const char **argv)
{
    Serial.println(Wagman::getCurrent());
    
    for (int i = 0; i < 5; i++) {
        Serial.print(Wagman::getCurrent(i));
        Serial.print(" PO=");
        Serial.println(poweronCurrent[i]);
    }
}

void commandHeartbeat(int argc, const char **argv)
{
    for (int i = 0; i < 5; i++) {
        Serial.println(devices[i].timeSinceHeartbeat());
    }
}

void commandThermistor(int argc, const char **argv)
{
    for (int i = 0; i < 5; i++) {
        Serial.println(Wagman::getThermistor(i));
    }
}

void commandEnvironment(int argc, const char **argv)
{
    Serial.print("temperature: ");
    Serial.println(Wagman::getTemperature());
    
    Serial.print("humidity: ");
    Serial.println(Wagman::getHumidity());
}

void commandBootMedia(int argc, const char **argv)
{
    int media;
    
    if (argc >= 2) {
        if (argc == 3) {
            Wagman::setBootMedia(atoi(argv[1]), atoi(argv[2]));
        }
        
        Serial.println(Wagman::getBootMedia(atoi(argv[1])));
    }
}

void commandLog(int argc, const char **argv)
{
    logging = !logging;

    Serial.print("logging: ");
    Serial.println(logging ? "on" : "off");
}

void commandBootFlags(int argc, const char **argv)
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

void commandHelp(int argc, const char **argv)
{
    for (Command *c = commands; c->name; c++) {
        Serial.println(c->name);
    }
}

const int BUFFER_SIZE = 80;
const int MAX_FIELDS = 8;
char buffer[BUFFER_SIZE];

void executeCommand(int argc, const char **argv)
{
    void (*func)(int, const char **) = NULL;

    // search for command and execute if found
    for (Command *c = commands; c->name; c++) {
        if (strcmp(c->name, argv[0]) == 0) {
            func = c->func;
            break;
        }
    }

    // marks the beginning of a response packet.
    Serial.print("<<<- 0 ");
    Serial.println(argv[0]);

    if (func != NULL) {
        func(argc, argv);
    } else  {
        Serial.println("command not found");
    }

    // marks the end of a response packet.
    Serial.println("->>>");
}

void processCommand()
{
    const char *fields[MAX_FIELDS];
    int numfields = 0;
    char *s = buffer;

    wdt_reset();

    while (numfields < MAX_FIELDS) {

        // skip whitespace
        while (isspace(*s)) {
            s++;
        }

        // check if next character is an argument
        if (isgraph(*s)) {

            // mark start of argument.
            fields[numfields++] = s;

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

    if (numfields > 0) {
        wdt_reset();
        executeCommand(numfields, fields);
    }
}

// ...may want to add some kind of assert failure log...

//void assert(bool condition, const char *msg)
//{
//    // for unexplained, unhandlable conditions, we can do an assertion failure.
//}

void setup()
{
    // save and clear boot flags
    bootflags = MCUSR;
    MCUSR = 0;

    wdt_enable(WDTO_8S);

    wdt_reset();
    Wagman::init();

    // blink sequence indicating that we got through the first step.
    for (int i = 0; i < 5; i++) {
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
    delay(2000);

    wdt_reset();
    Serial.begin(115200);
    delay(2000);

    // node controller
    devices[0].name = "nc"; // move this into EEPROM
    devices[0].port = 0;
    devices[0].bootSelector = 0;
    devices[0].primaryMedia = MEDIA_SD;
    devices[0].secondaryMedia = MEDIA_EMMC;

    // guest node
    devices[1].name = "gn"; // move this into EEPROM
    devices[1].port = 1;
    devices[1].bootSelector = 1;
    devices[1].primaryMedia = MEDIA_EMMC;
    devices[1].secondaryMedia = MEDIA_SD;

    devices[2].name = "";
    devices[3].name = "";
    devices[4].name = "";
    
    Record::incrementBootCount();
    Record::setLastBootTime(Wagman::getTime());

    resetWagman = false; // used to reset Wagman in main loop

    // initially used to track known current range...but this should be moved into EEPROM
    for (int i = 0; i < 5; i++) {
        poweronCurrent[i] = 0;
    }

    // boot up after brown out.
    if (bootflags & _BV(BORF)) {
        // what would we do differently than a power-off reset flag.
    }

    // boot up after power on. this is a case where it'd be good to do a self test, current range check and device connected check.
    if (bootflags & _BV(PORF)) {
        // do some initial test / calibration here.

        for (int i = 0; i < 5; i++) {
            wdt_reset();
            poweronCurrent[i] = Wagman::getCurrent(i);
        }

        // perform remainder of sensor health checks BEFORE powering on the other devices if possible.
    }

    // tripped by external reset? (do we do something similar to a watchdog reset here?)
    if ((bootflags & _BV(EXTRF)) || (bootflags & _BV(WDRF))) {
        // it looks like we were reset
    }

    for (int i = 0; i < 5; i++) {
        devices[i].init();
    }

    // always start node controller immediately, for now.
    devices[0].start();
    lastStartTime = millis();
}

static int bufferSize = 0;

void processCommands()
{
    while (Serial.available() > 0) {
        int c = Serial.read();

        buffer[bufferSize++] = c;

        if (bufferSize == BUFFER_SIZE) { // exceeds buffer! dangerous!
            bufferSize = 0;
        } else if (c == '\n') {
            buffer[bufferSize] = '\0';
            bufferSize = 0;
            processCommand();
        }
    }
}

void loop()
{
    unsigned long currentTime = millis();
    
    // ensure that the watchdog is always enabled (in case some anomaly disables it)
    wdt_enable(WDTO_8S);

    // start next unstarted device
    if (currentTime - lastStartTime > MINUTES(3)) {
        for (int i = 0; i < 5; i++) {
            wdt_reset();
            if (devices[i].canStart()) {
                devices[i].start();
                lastStartTime = currentTime;
                break;
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        wdt_reset();
        devices[i].update();
    }

    wdt_reset();
    processCommands();

    if (resetWagman) {
        // mark resetting journal flag so wagman knows it was told to reset.
        
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
}

