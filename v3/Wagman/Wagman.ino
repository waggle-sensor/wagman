#include <avr/wdt.h>
#include "Wagman.h"
#include "Record.h"

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

class Device
{
    public:

        void start();
        void stop(bool failure=false);
        void kill();
        void update();

        const char *name;
        byte port;
        byte bootSelector;
        bool started;

    private:

        void updateHeartbeat();
        void checkStopConditions();

        bool stopping;
        bool managed;
        unsigned long startTime;
        unsigned long stopTime;
        unsigned long heartbeatTime;
        int heartbeatMode;
};

static const unsigned long HEARTBEAT_TIMEOUT = MINUTES(2);
static const unsigned long STOPPING_TIMEOUT = MINUTES(2);

static const byte DEVICE_NC = 0;
static const byte DEVICE_GN = 1;

Device devices[5];
unsigned long lastDeviceStartTime = 0;

void healthCheck()
{
    // we can certainly use the eeprom to input what we believe is the right range
    // of sensor values. (this can also be in program space...)
    
    // we can detect if a device is within that operating range...but what will we
    // do if it's not? ignore the value as untrustworthy? i don't believe we can
    // do much correction with this information.
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

    // guest node
    devices[1].name = "gn";
    devices[1].port = 1;
    devices[1].bootSelector = 1;
    
    Record::incrementBootCount();
    Record::setLastBootTime(Wagman::getTime());

    Serial.print('?');
    Serial.println("system starting");
}

bool hasHeartbeat(int device)
{
    unsigned long startTime = millis();
    int startHeartbeat = Wagman::getHeartbeat(device);

    wdt_reset();

    while (true) {
        if (millis() - startTime > 2000) {
            wdt_reset();
            return false;
        }
        if (startHeartbeat != Wagman::getHeartbeat(device)) {
            wdt_reset();
            return true;
        }
    }
}

int getNextBootDevice()
{
    for (int i = 0; i < 5; i++) {
        // disabled
        if (!Record::getDeviceEnabled(i))
            continue;

        // already started
        if (devices[i].started)
            continue;

        if (i == DEVICE_NC || !Record::setRelayFailed(i))
            return i;
    }

    return -1;
}

void bootNextReadyDevice()
{
    int index = getNextBootDevice();

    if (index != -1) {
        devices[index].start();
    }
}

// fix case where no device is connected, but we keep trying to boot it.
// at some point should realize there's a problem and switch over to long
// delay booting.

void Device::start()
{
    if (started) {
        Serial.print('?');
        Serial.print(name);
        Serial.println(" already started");
        return; // consider having useful return info.
    }
    
    if (stopping) {
        Serial.print('?');
        Serial.print(name);
        Serial.println(" busy stopping");
        return;
    }
    
    // keep correct boot media stored somewhere as it may vary by device!
    if (Record::getBootFailures(port) % 4 == 3) {
        Serial.print('?');
        Serial.print(name);
        Serial.println(" booting secondary");
        Wagman::setBootMedia(bootSelector, MEDIA_EMMC);
    } else {
        Serial.print('?');
        Serial.print(name);
        Serial.println(" booting secondary");
        Wagman::setBootMedia(bootSelector, MEDIA_SD);
    }

    // ensure relay set to on (notice that we leave it as-is if already on.)
    Record::setRelayBegin(port);
    Wagman::setRelay(port, true);
    Record::setRelayEnd(port);

    // initialize started device parameters.
    started = true;
    stopping = false;
    managed = Record::getBootFailures(port) < 25; // this can be changed.
    startTime = millis();
    stopTime = 0;
    heartbeatTime = millis();
}

void Device::stop(bool failure)
{
    if (!started) {
        Serial.print('?');
        Serial.print(name);
        Serial.println(" not started");
        return;
    }
    
    if (stopping) {
        Serial.print('?');
        Serial.print(name);
        Serial.println(" already stopping");
        return;
    }

    if (failure) {
        Serial.print('?');
        Serial.print(name);
        Serial.println(" stopping (failure)");
        Record::incrementBootFailures(port); // using device <-> mapping...
    } else {
        Serial.print('?');
        Serial.print(name);
        Serial.println(" stopping");
    }

    stopping = true;
    stopTime = millis();
}

void Device::kill()
{
    Serial.print('?');
    Serial.print(name);
    Serial.println(" killing power!");

    Record::setRelayBegin(port);
    Wagman::setRelay(port, true);
    Wagman::setRelay(port, false);
    Record::setRelayEnd(port);
    
    started = false;
    stopping = false;
}

void Device::update()
{
    wdt_reset();
    
    if (started) {
        updateHeartbeat();
        
        if (stopping) {
            Serial.print('?');
            Serial.print(name);
            Serial.print(" stopping (");
            Serial.print((STOPPING_TIMEOUT - (millis() - stopTime)) / 1000);
            Serial.println(" seconds left)");
    
            if (port == 0) {
                Serial.println("!stop nc");
            } else { // for now, assume other nodes are guest nodes.
                Serial.println("!stop gn");
            }
    
            if (millis() - stopTime > STOPPING_TIMEOUT) {
                kill();
            }
        } else {
            checkStopConditions();
        }
    }
}

void Device::updateHeartbeat()
{
    if (hasHeartbeat(port)) {
        heartbeatTime = millis();
        Serial.print('?');
        Serial.print(name);
        Serial.println(" has heartbeat");
    } else {
        Serial.print('?');
        Serial.print(name);
        Serial.print(" no heartbeat (");
        Serial.print((HEARTBEAT_TIMEOUT - (millis() - heartbeatTime)) / 1000);
        Serial.println(" seconds left)");
    }
}

void Device::checkStopConditions()
{
    if (managed) {
        if (millis() - heartbeatTime > HEARTBEAT_TIMEOUT) {
            Serial.print('?');
            Serial.print(name);
            Serial.println(" heartbeat timeout!");
            stop(true);
        } else if (Wagman::getCurrent(port) < 100) {
            Serial.print('?');
            Serial.print(name);
            Serial.println(" current lost! stopping!");
            stop(true);
        }
    } else {
        if (millis() - heartbeatTime > HOURS(8)) {
            stop(false);
        }
    }
}

void commandStart(const char *msg)
{
    int index = msg[0] - '0';
    devices[index].start();
}

void commandStop(const char *msg)
{
    int index = msg[0] - '0';
    devices[index].stop();
}

void commandKill(const char *msg)
{
    int index = msg[0] - '0';
    devices[index].kill();
}

struct {
    byte command;
    void (*handler)(const char *msg);
} commandTable[] = {
    {'s', commandStart},
    {'x', commandStop},
    {'k', commandKill},
    {0, NULL},
};

static char inputBuffer[20];
static byte inputLength = 0;
static bool inputStarted = false;

void dispatchCommand()
{
    for (int i = 0; commandTable[i].command != 0; i++) {
        if (commandTable[i].command == inputBuffer[0]) {
            commandTable[i].handler(inputBuffer + 1);
            break;
        }
    }
}

void processCommands()
{
    wdt_reset();
    
    while (Serial.available() > 0) {
        int inChar = Serial.read();

        if (inputStarted) {
            inputBuffer[inputLength++] = inChar;

            if (inChar == '\n' || inChar == '\r') {
                inputBuffer[inputLength] = '\0';
                inputStarted = false;
                inputLength = 0;
                dispatchCommand();
            } else if (inputLength == 20) {
                // not a valid command string! reset buffer!
                inputStarted = false;
                inputLength = 0;
            }
        } else if (inChar == '!') {
            inputStarted = true;
            inputLength = 0;
        }
    }
}

void loop()
{
    processCommands();

    wdt_reset();

//    if (millis() - lastDeviceStartTime > SECONDS(15)) {
//        bootNextReadyDevice();
//        lastDeviceStartTime = millis();
//    }
    
    for (int i = 0; i < 5; i++) {
        devices[i].update();
    }
}

