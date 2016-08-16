#include <avr/wdt.h>
#include <EEPROM.h>
#include "Wagman.h"
#include "Record.h"
#include "Device.h"
#include "Logger.h"
#include "Error.h"
#include "MCP79412RTC.h"
#include "Timer.h"
#include "commands.h"
#include "verinfo.cpp"
#include "buildinfo.cpp"

void setupDevices();
void checkSensors();
void checkCurrentSensors();
void checkThermistors();
void printDate(const DateTime &dt);
unsigned long meanBootDelta(const Record::BootLog &bootLog, byte maxSamples);

static const byte DEVICE_COUNT = 5;
static const byte BUFFER_SIZE = 80;
static const byte MAX_ARGC = 8;

byte bootflags = 0;
bool shouldResetSystem = false;
bool logging = true;
byte deviceWantsStart = 255;

Device devices[DEVICE_COUNT];

static Timer startTimer;
static Timer statusTimer;

static char buffer[BUFFER_SIZE];
static byte bufferSize = 0;

time_t setupTime;

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
    { "up", commandUptime },
    { "enable", commandEnable },
    { "disable", commandDisable },
    { "watch", commandWatch },
    { "log", commandLog },
    { "eereset", commandResetEEPROM },
    { "boots", commandBoots },
    { "ver", commandVersion },
    { NULL, NULL },
};

void printDate(const DateTime &dt)
{
    Serial.print(dt.year);
    Serial.print(' ');
    Serial.print(dt.month);
    Serial.print(' ');
    Serial.print(dt.day);
    Serial.print(' ');
    Serial.print(dt.hour);
    Serial.print(' ');
    Serial.print(dt.minute);
    Serial.print(' ');
    Serial.print(dt.second);
}

byte commandPing(byte argc, const char **argv)
{
    if (argc != 2)
        return ERROR_INVALID_ARGC;

    byte port = atoi(argv[1]);

    if (!Wagman::validPort(port))
        return ERROR_INVALID_PORT;

    devices[port].sendExternalHeartbeat();

    return 0;
}

byte commandStart(byte argc, const char **argv)
{
    if (argc != 2)
        return ERROR_INVALID_ARGC;

    byte port = atoi(argv[1]);

    if (!Wagman::validPort(port))
        return ERROR_INVALID_PORT;

    deviceWantsStart = port;

    return 0;
}

byte commandStop(byte argc, const char **argv)
{
    if (argc != 2)
        return ERROR_INVALID_ARGC;

    byte port = atoi(argv[1]);

    if (!Wagman::validPort(port))
        return ERROR_INVALID_PORT;

    devices[port].stop();

    return 0;
}

byte commandKill(byte argc, const char **argv)
{
    if (argc != 2)
        return ERROR_INVALID_ARGC;

    byte port = atoi(argv[1]);

    if (!Wagman::validPort(port))
        return ERROR_INVALID_PORT;

    devices[port].kill();

    return 0;
}

byte commandReset(byte argc, const char **argv)
{
    shouldResetSystem = true;
    return 0;
}

byte commandID(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    byte id[8];

    RTC.idRead(id);

    for (byte i = 0; i < 8; i++) {
        Serial.print(id[i] & 0x0F, HEX);
        Serial.print(id[i] >> 4, HEX);
    }

    Serial.println();

    return 0;
}

byte commandEEDump(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
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

    return 0;
}

byte commandDate(byte argc, const char **argv)
{
    if (argc != 1 && argc != 7)
        return ERROR_INVALID_ARGC;

    if (argc == 1) {
        DateTime dt;
        Wagman::getDateTime(dt);
        printDate(dt);
        Serial.println();
    } else if (argc == 7) {
        DateTime dt;

        dt.year = atoi(argv[1]);
        dt.month = atoi(argv[2]);
        dt.day = atoi(argv[3]);
        dt.hour = atoi(argv[4]);
        dt.minute = atoi(argv[5]);
        dt.second = atoi(argv[6]);

        Wagman::setDateTime(dt);
    }

    return 0;
}

byte commandCurrent(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    Serial.print(Wagman::getCurrent());

    for (byte i = 0; i < DEVICE_COUNT; i++) {
        Serial.print(' ');
        Serial.print(Wagman::getCurrent(i));
    }

    Serial.println();

    return 0;
}

byte commandHeartbeat(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    for (byte i = 0; i < DEVICE_COUNT; i++) {
        Serial.println(devices[i].timeSinceHeartbeat());
    }

    return 0;
}

byte commandFailCount(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    for (byte i = 0; i < DEVICE_COUNT; i++) {
        Serial.println(Record::getBootFailures(i));
    }

    return 0;
}

byte commandThermistor(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    for (byte i = 0; i < DEVICE_COUNT; i++) {
        Serial.println(Wagman::getThermistor(i));
    }

    return 0;
}

byte commandEnvironment(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    Serial.print("temperature ");
    Serial.println(Wagman::getTemperature());

    Serial.print("humidity ");
    Serial.println(Wagman::getHumidity());

    return 0;
}

byte commandBootMedia(byte argc, const char **argv)
{
    if (argc != 2 && argc != 3)
        return ERROR_INVALID_ARGC;

    byte index = atoi(argv[1]);

    if (!Wagman::validPort(index))
        return ERROR_INVALID_PORT;

    if (argc == 3) {
        if (strcmp(argv[2], "sd") == 0) {
            devices[index].setNextBootMedia(MEDIA_SD);
            Serial.println("set sd");
        } else if (strcmp(argv[2], "emmc") == 0) {
            devices[index].setNextBootMedia(MEDIA_EMMC);
            Serial.println("set emmc");
        } else {
            Serial.println("invalid media");
        }
    } else if (argc == 2) {
        byte bootMedia = devices[index].getNextBootMedia();

        if (bootMedia == MEDIA_SD) {
            Serial.println("sd");
        } else if (bootMedia == MEDIA_EMMC) {
            Serial.println("emmc");
        } else {
            Serial.println("invalid media");
        }
    }

    return 0;
}

byte commandLog(byte argc, const char **argv)
{
    if (argc == 2) {
        if (strcmp(argv[1], "on") == 0) {
            logging = true;
        } else if (strcmp(argv[1], "off") == 0) {
            logging = false;
        }
    }

    return 0;
}

byte commandBootFlags(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    if (bootflags & _BV(WDRF))
        Serial.println("WDRF");
    if (bootflags & _BV(BORF))
        Serial.println("BORF");
    if (bootflags & _BV(EXTRF))
        Serial.println("EXTRF");
    if (bootflags & _BV(PORF))
        Serial.println("PORF");
    return 0;
}

byte commandUptime(__attribute__ ((unused)) byte argc, __attribute__ ((unused)) const char **argv)
{
    time_t time;
    Wagman::getTime(time);
    Serial.println(time - setupTime);
    return 0;
}

byte commandEnable(byte argc, const char **argv)
{
    if (argc != 2)
        return ERROR_INVALID_ARGC;

    byte port = atoi(argv[1]);

    if (!Wagman::validPort(port))
        return ERROR_INVALID_PORT;

    devices[port].enable();

    return 0;
}

byte commandDisable(byte argc, const char **argv)
{
    if (argc != 2)
        return ERROR_INVALID_ARGC;

    byte port = atoi(argv[1]);

    if (!Wagman::validPort(port))
        return ERROR_INVALID_PORT;

    devices[port].disable();

    return 0;
}

byte commandWatch(byte argc, const char **argv)
{
//    if (argc != 4)
//        return ERROR_USAGE;

    if (argc == 4) {
        byte index = atoi(argv[1]);
        if (Wagman::validPort(index)) {
            bool mode = strcmp(argv[3], "t") == 0;

            if (strcmp(argv[2], "hb") == 0) {
                devices[index].watchHeartbeat = mode;
            } else if (strcmp(argv[2], "cu")) {
                devices[index].watchCurrent = mode;
            }
        }
    }

    return 0;
}

byte commandBoots(byte argc, const char **argv)
{
    unsigned long count;
    Record::getBootCount(count);
    Serial.println(count);
    return 0;
}

byte commandVersion(byte argc, const char **argv)
{
    Serial.print("hw ");
    Serial.print(WAGMAN_HW_VER_MAJ);
    Serial.print('.');
    Serial.println(WAGMAN_HW_VER_MAJ);

    Serial.print("ker ");
    Serial.print(WAGMAN_KERNEL_MAJ);
    Serial.print('.');
    Serial.print(WAGMAN_KERNEL_MIN);
    Serial.print('.');
    Serial.println(WAGMAN_KERNEL_SUB);

    Serial.print("time ");
    Serial.println(BUILD_TIME);

    Serial.print("git ");
    Serial.println(BUILD_GIT);

    return 0;
}

void executeCommand(const char *sid, byte argc, const char **argv)
{
    byte (*func)(byte, const char **) = NULL;

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
        Serial.println("invalid command");
    }

    // marks the end of a response packet.
    Serial.println("->>>");
}

byte commandResetEEPROM(byte argc, const char **argv)
{
    Record::clearMagic();
    return 0;
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
        // this sid case was suggested by wolfgang to help identify the source of certain messages.
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

void deviceKilled(Device &device)
{
    if (meanBootDelta(Record::bootLogs[device.port], 3) < 240) {
        device.setStartDelay(300000);
        Logger::begin("backoff");
        Logger::log("backing off of ");
        Logger::log(device.name);
        Logger::end();
    }
}

void setup()
{
    bootflags = MCUSR;
    MCUSR = 0;
    wdt_disable();
    delay(4000);
    wdt_enable(WDTO_8S);

    wdt_reset();
    Serial.begin(57600);

    // show init light sequence
    for (byte i = 0; i < 8; i++) {
        Wagman::setLED(0, true);
        delay(100);
        Wagman::setLED(1, true);
        delay(100);
        Wagman::setLED(0, false);
        delay(100);
        Wagman::setLED(1, false);
        delay(100);
    }

    wdt_reset();

    if (!Record::initialized()) {
        Record::init();
        Wagman::init();

        for (byte i = 0; i < 16; i++) {
            Wagman::setLED(0, true);
            delay(20);
            Wagman::setLED(1, true);
            delay(20);
            Wagman::setLED(0, false);
            delay(20);
            Wagman::setLED(1, false);
            delay(20);
        }
    } else {
        Wagman::init();
    }

    Wagman::getTime(setupTime);
    Record::setLastBootTime(setupTime);
    Record::incrementBootCount();

    if (bootflags & _BV(PORF) || bootflags & _BV(BORF)) {
        checkSensors();
    }

    setupDevices();

    Wagman::setWireEnabled(false);

    deviceWantsStart = 0;
    shouldResetSystem = false;
    bufferSize = 0;
    startTimer.reset();
    statusTimer.reset();
}

void setupDevices()
{
    devices[0].name = "nc";
    devices[0].port = 0;
    devices[0].bootSelector = 0;
    devices[0].primaryMedia = MEDIA_SD;
    devices[0].secondaryMedia = MEDIA_EMMC;
    devices[0].watchHeartbeat = true;
    devices[0].watchCurrent = true;

    devices[1].name = "gn";
    devices[1].port = 1;
    devices[1].bootSelector = 1;
    devices[1].primaryMedia = MEDIA_SD;
    devices[1].secondaryMedia = MEDIA_EMMC;
    devices[1].watchHeartbeat = true;
    devices[1].watchCurrent = true;

    devices[2].name = "cs";
    devices[2].port = 2;
    devices[2].primaryMedia = MEDIA_SD;
    devices[2].secondaryMedia = MEDIA_EMMC;
    devices[2].watchHeartbeat = false;
    devices[2].watchCurrent = false;

    devices[3].name = "x1";
    devices[3].port = 3;
    devices[3].watchHeartbeat = false;
    devices[3].watchCurrent = false;

    devices[4].name = "x2";
    devices[4].port = 4;
    devices[4].watchHeartbeat = false;
    devices[4].watchCurrent = false;

    for (byte i = 0; i < DEVICE_COUNT; i++) {
        devices[i].init();
    }

    // Check for any incomplete relays which may have killed the system.
    // An assumption here is that if one of these devices killed the system
    // when starting, then there should only be one incomplete relay state.
    for (byte i = 0; i < DEVICE_COUNT; i++) {
        byte state = Record::getRelayState(i);

        if (state == RELAY_TURNING_ON || state == RELAY_TURNING_OFF) {
            Record::setRelayState(i, RELAY_OFF);

            // Plausible that power off was caused by toggling this relay.
            if (bootflags & _BV(PORF) || bootflags & _BV(BORF)) {
                devices[i].disable();
            } else {
                devices[i].kill();
            }
        }
    }
}

void checkSensors()
{
    checkCurrentSensors();
    checkThermistors();
}

void checkCurrentSensors()
{
    for (byte i = 0; i < DEVICE_COUNT; i++) {
        byte attempt;

        if (Record::getPortCurrentSensorHealth(i) >= 5) {
            Logger::begin("post");
            Logger::log("current sensor ");
            Logger::log(i);
            Logger::log(" too many faults");
            Logger::end();
            continue;
        }

        for (attempt = 0; attempt < 3; attempt++) {
            Record::setPortCurrentSensorHealth(i, Record::getPortCurrentSensorHealth(i) + 1);
            delay(20);
            unsigned int value = Wagman::getCurrent(i);
            delay(50);
            Record::setPortCurrentSensorHealth(i, Record::getPortCurrentSensorHealth(i) - 1);
            delay(20);

            if (value <= 5000)
                break;
        }

        if (attempt == 3) {
            Logger::begin("post");
            Logger::log("current sensor ");
            Logger::log(i);
            Logger::log(" out of range.");
            Logger::end();
        }
    }
}

void checkThermistors()
{
    for (byte i = 0; i < DEVICE_COUNT; i++) {
        byte attempt;

        if (Record::getThermistorSensorHealth(i) >= 5) {
            Logger::begin("post");
            Logger::log("thermistor ");
            Logger::log(i);
            Logger::log(" too many faults");
            Logger::end();
            continue;
        }

        for (attempt = 0; attempt < 3; attempt++) {
            Record::setThermistorSensorHealth(i, Record::getThermistorSensorHealth(i) + 1);
            delay(20);
            unsigned int value = Wagman::getThermistor(i);
            delay(50);
            Record::setThermistorSensorHealth(i, Record::getThermistorSensorHealth(i) - 1);
            delay(20);

            if (value <= 10000)
                break;
        }

        if (attempt == 3) {
            Logger::begin("post");
            Logger::log("thermistor ");
            Logger::log(i);
            Logger::log(" out of range.");
            Logger::end();
        }
    }
}

unsigned long meanBootDelta(const Record::BootLog &bootLog, byte maxSamples)
{
    byte count = min(maxSamples, bootLog.getCount());
    unsigned long total = 0;

    for (byte i = 1; i < count; i++) {
        total += bootLog.getEntry(i) - bootLog.getEntry(i-1);
    }

    return total / count;
}

void showBootLog(const Record::BootLog &bootLog)
{
    Logger::begin("bootlog");
    for (byte i = 0; i < bootLog.getCount(); i++) {
        Logger::log(" ");
        Logger::log(bootLog.getEntry(i));
    }
    Logger::end();

    if (bootLog.getCount() > 1) {
        Logger::begin("bootdt");
        for (byte i = 1; i < min(4, bootLog.getCount()); i++) {
            unsigned long bootdt = bootLog.getEntry(i) - bootLog.getEntry(i-1);
            Logger::log(" ");
            Logger::log(bootdt);
        }

        if (meanBootDelta(bootLog, 3) < 200) {
            Logger::log(" !");
        }

        Logger::end();
    }
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
        for (byte i = 0; i < DEVICE_COUNT; i++) {
            if (devices[i].canStart()) { // include !started in canStart() call.
                startTimer.reset();
                devices[i].start();
                showBootLog(Record::bootLogs[i]);
                break;
            }
        }
    }
}

void loop()
{
    // ensure that the watchdog is always enabled
    wdt_enable(WDTO_8S);
    wdt_reset();

    startNextDevice();

    for (byte i = 0; i < DEVICE_COUNT; i++) {
        devices[i].update();
    }

    wdt_reset();
    processCommands();

    if (statusTimer.exceeds(30000)) {
        statusTimer.reset();
        wdt_reset();
        logStatus();
    }

    if (shouldResetSystem) {
        resetSystem();
    }

    Wagman::toggleLED(0);

    delay(200);
}

void resetSystem()
{
    // ensure that watchdog is set!
    wdt_enable(WDTO_8S);
    wdt_reset();

    // watchdog will reset in a few seconds.
    for (;;) {
        for (byte i = 0; i < 5; i++) {
            Wagman::setLED(0, true);
            Wagman::setLED(1, true);
            delay(80);
            Wagman::setLED(0, false);
            Wagman::setLED(1, false);
            delay(80);
        }

        Wagman::setLED(0, true);
        Wagman::setLED(1, true);
        delay(200);
        Wagman::setLED(0, false);
        Wagman::setLED(1, false);
        delay(200);
    }
}

void logStatus()
{
    byte id[8];

    if (Wagman::getWireEnabled()) {
        RTC.idRead(id);
    } else {
        for (byte i = 0; i < 8; i++) {
            id[i] = 0xFF;
        }
    }

    Logger::begin("id");

    for (byte i = 0; i < 8; i++) {
        Logger::logHex(id[i] & 0x0F);
        Logger::logHex(id[i] >> 4);
    }

    Logger::end();

    delay(50);

    DateTime dt;
    Wagman::getDateTime(dt);

    Logger::begin("date");
    printDate(dt);
    Serial.println();

    delay(50);

    Logger::begin("cu");

    Logger::log(Wagman::getCurrent());

    for (byte i = 0; i < 5; i++) {
        Logger::log(' ');
        Logger::log(Wagman::getCurrent(i));
    }

    Logger::end();

    delay(50);

    Logger::begin("th");

    for (byte i = 0; i < 5; i++) {
        Logger::log(Wagman::getThermistor(i));
        Logger::log(' ');
    }

    Logger::end();

    delay(50);

    Logger::begin("env");
    Logger::log(Wagman::getTemperature());
    Logger::log(' ');
    Logger::log(Wagman::getHumidity());
    Logger::end();

    delay(50);

    Logger::begin("fails");

    for (byte i = 0; i < 5; i++) {
        Logger::log(Record::getBootFailures(i));
        Logger::log(' ');
    }

    Logger::end();

    delay(50);

    Logger::begin("enabled");

    for (byte i = 0; i < 5; i++) {
        Logger::log(Record::getDeviceEnabled(i));
        Logger::log(' ');
    }

    Logger::end();

    delay(50);

    Logger::begin("media");

    for (byte i = 0; i < 2; i++) {
        byte bootMedia = devices[i].getNextBootMedia();

        if (bootMedia == MEDIA_SD) {
            Logger::log("sd ");
        } else if (bootMedia == MEDIA_EMMC) {
            Logger::log("emmc ");
        } else {
            Logger::log("? ");
        }
    }

    Logger::end();
}
