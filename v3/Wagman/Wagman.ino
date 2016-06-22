#include <avr/wdt.h>
#include <EEPROM.h>
#include "Wagman.h"
#include "Record.h"
#include "Device.h"
#include "MCP79412RTC.h"

// TODO Add persistant event logging on using system hardware so we can trace what happened
//      during a device failure.
// TODO Add calibration interface for system / port sensors. (not so pressing)
// TODO Add self test / calibration mechanism. (Though we're not really using it right now.)
// TODO add communication layer!
// TODO look at Serial write buffer overflow lock up.
// TODO rearrange to better reflect state diagram.

#define SECONDS(x) ((unsigned long)(1000 * (x)))
#define MINUTES(x) ((unsigned long)(60000 * (x)))
#define HOURS(x) ((unsigned long)(3600000 * (x)))

bool logging = false;

static const byte DEVICE_NC = 0;
static const byte DEVICE_GN = 1;

Device devices[5];
unsigned long lastDeviceStartTime = 0;

typedef struct {
    const char *name;
    void (*func)(int, const char **);
} Command;

// NOTE Each time a command is caught, we can reset the node controller heartbeat as an addition "alive" indicator!

void commandOn(int argc, const char **args)
{
    for (int i = 1; i < argc; i++) {
        Wagman::setRelay(atoi(args[i]), true);
    }
}

void commandOff(int argc, const char **args)
{
    for (int i = 1; i < argc; i++) {
        Wagman::setRelay(atoi(args[i]), false);
    }
}

void commandStart(int argc, const char **args)
{
    for (int i = 1; i < argc; i++) {
        int index = atoi(args[i]);
        devices[index].start();
    }
}

void commandStop(int argc, const char **args)
{
    for (int i = 1; i < argc; i++) {
        int index = atoi(args[i]);
        devices[index].stop(false);
    }
}

void commandReset(int argc, const char **args)
{
}

// ...can do a full system status dump here too...
// include current, heartbeats, thermistor, etc...
// instead of separate commands...maybe?

void commandInfo(int argc, const char **args)
{
    byte id[8];

    RTC.idRead(id);
    
    Serial.print("id: ");

    for (int i = 0; i < 8; i++) {
        Serial.print(id[i] & 0x0F, HEX);
        Serial.print(id[i] >> 8, HEX);
    }

    Serial.println();
    
    Serial.print("temperature: ");
    Serial.println(Wagman::getTemperature());
    
    Serial.print("humidity: ");
    Serial.println(Wagman::getHumidity());
}

void commandDump(int argc, const char **args)
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

void commandDate(int argc, const char **args)
{
    tmElements_t tm;

    if (argc == 1) {
        RTC.read(tm);

        Serial.print(args[0]);
        Serial.print(": ");
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

        tm.Year = atoi(args[1]) - 1970;
        tm.Month = (argc >= 3) ? atoi(args[2]) : 0;
        tm.Day = (argc >= 4) ? atoi(args[3]) : 0;
        tm.Hour = (argc >= 5) ? atoi(args[4]) : 0;
        tm.Minute = (argc >= 6) ? atoi(args[5]) : 0;
        tm.Second = (argc >= 7) ? atoi(args[6]) : 0;
    
        RTC.set(makeTime(tm));
    }
}

void commandCurrent(int argc, const char **args)
{
    Serial.print(args[0]);
    Serial.print(": ");

    Serial.print(Wagman::getCurrent());
    Serial.print(" ");
    
    for (int i = 0; i < 5; i++) {
        Serial.print(Wagman::getCurrent(i));
        Serial.print(" ");
    }
    
    Serial.println();
}

void commandHeartbeat(int argc, const char **args)
{
    unsigned long currtime = millis();

    Serial.print(args[0]);
    Serial.print(": ");
    
    for (int i = 0; i < 5; i++) {
        Serial.print((currtime - devices[i].heartbeatTime) / 1000);
        Serial.print(" ");
    }
    
    Serial.println();
}

void commandThermistor(int argc, const char **args)
{
    Serial.print(args[0]);
    Serial.print(": ");
    
    for (int i = 0; i < 5; i++) {
        Serial.print(Wagman::getThermistor(i));
        Serial.print(" ");
    }
    
    Serial.println();
}

void commandBootMedia(int argc, const char **args)
{
    if (argc == 2) {
        Serial.print("bm: ");
        Serial.println(Wagman::getBootMedia(atoi(args[1])));
    } else if (argc == 3) {
        Wagman::setBootMedia(atoi(args[1]), atoi(args[2]));
        Serial.print("bm: ");
        Serial.println(Wagman::getBootMedia(atoi(args[1])));
    }
}

void commandLog(int argc, const char **args)
{
    logging = !logging;

    if (logging) {
        Serial.println("logging: on");
    } else {
        Serial.println("logging: off");
    }
}

Command commands[] = {
    { "on", commandOn },
    { "off", commandOff },
    { "start", commandStart },
    { "stop", commandStop },
    { "reset", commandReset },
    { "info", commandInfo },
    { "eedump", commandDump },
    { "date", commandDate },
    { "log", commandLog },
    { "cu", commandCurrent },
    { "hb", commandHeartbeat },
    { "bm", commandBootMedia },
    { "therm", commandThermistor },
    { NULL, NULL },
};

// notice...don't really need a ping / pong message on either side. can just check if communication times out.

const int BUFFER_SIZE = 120;
const int MAX_FIELDS = 12;
char buffer[BUFFER_SIZE];

void processCommand()
{
    const char *fields[MAX_FIELDS];
    int numfields = 0;
    char *s = buffer;

    // set all fields to empty string
    for (int i = 0; i < MAX_FIELDS; i++)
        fields[i] = NULL;

    while (numfields < MAX_FIELDS) {

        // skip whitespace
        while (isspace(*s))
            s++;

        // mark an argument
        if (isalnum(*s)) {
            fields[numfields++] = s;
            while (isalnum(*s))
                s++;
        }

        // end of buffer?
        if (*s == '\0')
            break;

        // split buffer at argument
        *s++ = '\0';
    }

    // device is talking to us coherently, so it's alive
//    devices[0].heartbeatTime = millis();

    // empty command
    if (numfields == 0)
        return;

    // look up command in table
    for (Command *c = commands; c->name; c++) {
        if (strcmp(c->name, fields[0]) == 0) {
            c->func(numfields, fields);
            return;
        }
    }

    Serial.print(fields[0]);
    Serial.println(": command not found");
}

void setup()
{
    wdt_enable(WDTO_4S);
   
    wdt_reset();
    Serial.begin(115200);
    delay(1000);

    wdt_reset();
    Wagman::init();
    delay(1000);

    wdt_reset();
    Record::init();
    delay(1000);

    // node controller
    devices[0].name = "nc";
    devices[0].port = 0;
    devices[0].bootSelector = 0;
    devices[0].primaryMedia = MEDIA_SD;
    devices[0].secondaryMedia = MEDIA_EMMC;

    // guest node
    devices[1].name = "gn";
    devices[1].port = 1;
    devices[1].bootSelector = 1;
    devices[1].primaryMedia = MEDIA_EMMC;
    devices[1].secondaryMedia = MEDIA_SD;
    
    Record::incrementBootCount();
    Record::setLastBootTime(Wagman::getTime());
}

void processCommands()
{
    static int bufferSize = 0;

    while (Serial.available() > 0) {
        int c = Serial.read();

        buffer[bufferSize++] = c;

        if (c == '\n') {
            buffer[bufferSize] = '\0';
            processCommand();
            bufferSize = 0;
        }
    }
}

//int getNextBootDevice()
//{
//    for (int i = 0; i < 5; i++) {
//        // disabled
//        if (!Record::getDeviceEnabled(i))
//            continue;
//
//        // already started
//        if (devices[i].started)
//            continue;
//
//        if (i == DEVICE_NC || !Record::setRelayFailed(i))
//            return i;
//    }
//
//    return -1;
//}
//
//void bootNextReadyDevice()
//{
//    int index = getNextBootDevice();
//
//    if (index != -1) {
//        devices[index].start();
//    }
//}

void loop()
{
    wdt_reset();
    processCommands();

    for (int i = 0; i < 5; i++) {
        wdt_reset();
        devices[i].update();
    }

    if (!devices[0].started) {
        wdt_reset();
        devices[0].start(); // ok..now work more precisely on refining these conditions.
    }
}

// may even want a traceback from watchdog???
// can try keeping some information in memory which can be used to debug lockup.

