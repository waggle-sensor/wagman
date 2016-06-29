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
// TODO Observed in the logs that current checking is too aggresive. Use timeout as mentioned above.
// TODO may want to occasionally send a ping to wake?
// TODO keepalive 1 T/F

// TODO minimize int types (int -> byte / char when possible)
// TODO check if WDR shows up as NC serial dev change.

// TODO add variable LED to encode states.

// TODO watchout for keywords in naming

// TODO set up full port range.

static const int DEVICE_COUNT = 3;
static const int BUFFER_SIZE = 80;
static const int MAX_FIELDS = 8;

int deviceWantsStart = -1;
bool resetWagman = false;
bool logging = false;
int poweronCurrent[5];

static Timer startTimer;

static byte bootflags;

Device devices[5];
unsigned long lastDeviceStartTime = 0;

char buffer[BUFFER_SIZE];
static int bufferSize = 0;

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
void commandFailCount(int argc, const char **argv);
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
    { "fc", commandFailCount },
    { "help", commandHelp },
    { "log", commandLog },
    { NULL, NULL },
};

void commandPing(int argc, const char **argv)
{    
    for (int i = 1; i < argc; i++) {
        int index = atoi(argv[i]);

        if (Wagman::validPort(index)) {
            devices[index].sendExternalHeartbeat();
        }
    }

    Serial.println("pong");
}

void commandStart(int argc, const char **argv)
{
    int index = atoi(argv[1]);

    if (Wagman::validPort(index)) {
        deviceWantsStart = index;
    }
}

void commandStop(int argc, const char **argv)
{
    for (int i = 1; i < argc; i++) {
        int index = atoi(argv[i]);

        if (Wagman::validPort(index)) {
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

        if (Wagman::validPort(index)) {
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
    for (unsigned int i = 0; i < 1024; i++) {
        byte value = EEPROM.read(i);
        
        if ((value & 0xf0) == 0x00)
           Serial.print('0');
        Serial.print(value, HEX);
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
    
    for (int i = 0; i < DEVICE_COUNT; i++) {
        Serial.println(Wagman::getCurrent(i));
    }
}

void commandHeartbeat(int argc, const char **argv)
{
    for (int i = 0; i < DEVICE_COUNT; i++) {
        Serial.println(devices[i].timeSinceHeartbeat() / 1000);
    }
}

void commandFailCount(int argc, const char **argv)
{
    for (int i = 0; i < DEVICE_COUNT; i++) {
        Serial.println(Record::getBootFailures(i));
    }
}

void commandThermistor(int argc, const char **argv)
{
    for (int i = 0; i < DEVICE_COUNT; i++) {
        Serial.println(Wagman::getThermistor(i));
    }
}

void commandEnvironment(int argc, const char **argv)
{
    Serial.print("temperature=");
    Serial.println(Wagman::getTemperature());
    
    Serial.print("humidity=");
    Serial.println(Wagman::getHumidity());
}

void commandBootMedia(int argc, const char **argv)
{
    if (argc < 2)
        return;

    int index = atoi(argv[1]);

    if (!Wagman::validPort(index))
        return;

    // ...move these string into a lookup table...
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
        int bootMedia = devices[index].getBootMedia();

        if (bootMedia == MEDIA_SD) {
            Serial.println("sd");
        } else if (bootMedia == MEDIA_EMMC) {
            Serial.println("emmc");
        } else {
            Serial.println("invalid media");
        }
    }
}

void commandLog(int argc, const char **argv)
{
    logging = !logging;
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

void processCommands()
{
    Timer timer;

    timer.reset();

    while (Serial.available() > 0 && !timer.exceeds(3000)) {
        int c = Serial.read();

        buffer[bufferSize++] = c;

        if (bufferSize >= BUFFER_SIZE) { // exceeds buffer! dangerous!
            bufferSize = 0;
            
            // flush remainder of line.
            while (Serial.available() > 0 && !timer.exceeds(3000)) {
                if (Serial.read() == '\n')
                    break;
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
    wdt_disable();

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

    devices[1].name = "gn"; // move this into EEPROM?
    devices[1].port = 1;
    devices[1].bootSelector = 1;
    devices[1].primaryMedia = MEDIA_EMMC;
    devices[1].secondaryMedia = MEDIA_SD;

    devices[2].name = "coresense";
    devices[2].port = 2;
    
    devices[3].name = "na1";
    devices[3].port = 3;
    
    devices[4].name = "na2";
    devices[4].port = 4;
    
    Record::incrementBootCount();
    Record::setLastBootTime(Wagman::getTime());

    resetWagman = false; // used to reset Wagman in main loop

    // boot up after brown out.
    if (bootflags & _BV(BORF)) {
        // what would we do differently than a power-off reset flag.
    }

    // boot up after power on. this is a case where it'd be good to do a self test, current range check and device connected check.
    if (bootflags & _BV(PORF)) {
        // do some initial test / calibration here.

        for (int i = 0; i < DEVICE_COUNT; i++) {
            wdt_reset();
            poweronCurrent[i] = Wagman::getCurrent(i);
        }

        

        // perform remainder of sensor health checks BEFORE powering on the other devices if possible.

        // want ensure that devices are off and then set base line current values
    }

    // tripped by external reset? (do we do something similar to a watchdog reset here?)
    if ((bootflags & _BV(EXTRF)) || (bootflags & _BV(WDRF))) {
    }

    for (int i = 0; i < DEVICE_COUNT; i++) {
        devices[i].init();
    }

    // set the node controller as starting device
    deviceWantsStart = 0;
    startTimer.reset();

    // reset buffer size (just in case...)
    bufferSize = 0;
}

void startNextDevice()
{
    // if we've asked for a specific device, start that device.
    if (Wagman::validPort(deviceWantsStart)) {
        devices[deviceWantsStart].start();
        startTimer.reset();
        deviceWantsStart = -1;
        return;
    }

    if (startTimer.exceeds(30000)) {
        for (int i = 0; i < DEVICE_COUNT; i++) {
            if (!devices[i].started() && devices[i].canStart()) { // include !started in canStart() call.
                devices[i].start();
                startTimer.reset();
                return;
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
    for (int i = 0; i < 5; i++) {
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

    delay(100); // check if saves power
}

