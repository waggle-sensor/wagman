#include <avr/wdt.h>
#include "Wagman.h"
#include "Record.h"

// TODO Add persistant event logging on using system hardware so we can trace what happened
//      during a device failure.
// TODO Add calibration interface for system / port sensors. (not so pressing)
// TODO Add self test / calibration mechanism. (Though we're not really using it right now.)
// TODO add communication layer!
// TODO look at Serial write buffer overflow lock up.

#define SECONDS(x) ((unsigned long)(1000 * (x)))
#define MINUTES(x) ((unsigned long)(60000 * (x)))
#define HOURS(x) ((unsigned long)(3600000 * (x)))

struct Device
{
    bool started;
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
        if (Record::getDeviceEnabled(i) && !devices[i].started && (i == DEVICE_NC || !Record::setRelayFailed(i)))
            return i;
    }

    return -1;
}

void bootDevice(int device)
{            
    // keep correct boot media stored somewhere as it may vary by device!
    if (Record::getBootFailures(device) % 4 == 3) {
        Serial.print('?');
        Serial.print(device);
        Serial.println(" booting secondary");
        Wagman::setBootMedia(device, MEDIA_EMMC);
    } else {
        Serial.print('?');
        Serial.print(device);
        Serial.println(" booting secondary");
        Wagman::setBootMedia(device, MEDIA_SD);
    }

    // ensure relay set to on (notice that we leave it as-is if already on.)
    Record::setRelayBegin(device);
    Wagman::setRelay(device, true);
    Record::setRelayEnd(device);

    // initialize started device parameters.
    devices[device].started = true;
    devices[device].stopping = false;
    devices[device].managed = Record::getBootFailures(device) < 25; // this can be changed.
    devices[device].startTime = millis();
    devices[device].stopTime = 0;
    devices[device].heartbeatTime = millis();
}

void bootNextReadyDevice()
{
    int device = getNextBootDevice();

    if (device != -1) {
        bootDevice(device);
    }
}

// fix case where no device is connected, but we keep trying to boot it.
// at some point should realize there's a problem and switch over to long
// delay booting.

void updateDevice(int device)
{
    wdt_reset();
    
    if (!devices[device].started) // should we do anything special here?
        return;

    // if we're not already stopping, check stopping criteria.
    if (devices[device].stopping) {

        if (device == DEVICE_NC) {
            Serial.println("!stop nc");
        } else { // for now, assume other nodes are guest nodes.
            Serial.println("!stop gn");
        }

        Serial.print('?');
        Serial.print(device);
        Serial.print(" stop time remaining ");
        Serial.println(STOPPING_TIMEOUT - (millis() - devices[device].stopTime));

        if (millis() - devices[device].stopTime > STOPPING_TIMEOUT) { // any other conditions to add? this is the most conservative.
            Serial.print('?');
            Serial.print(device);
            Serial.println(" killing power!");
        
            Record::setRelayBegin(device);
            Wagman::setRelay(device, true);
            Wagman::setRelay(device, false);
            Record::setRelayEnd(device);
            
            devices[device].started = false;
        }
    } else {
        // update most recent heartbeat time
        if (hasHeartbeat(device)) {
            devices[device].heartbeatTime = millis();
            Serial.print('?');
            Serial.print(device);
            Serial.println(" has heartbeat");
        } else {
            Serial.print('?');
            Serial.print(device);
            Serial.print(" no heartbeat (");
            Serial.print(HEARTBEAT_TIMEOUT - (millis() - devices[device].heartbeatTime));
            Serial.println(" left)");
        }

        // check if stop condtions are met.
        if (devices[device].managed) {
            if (millis() - devices[device].heartbeatTime > HEARTBEAT_TIMEOUT) {
                Serial.print('?');
                Serial.print(device);
                Serial.println(" heartbeat timeout! stopping...");
                devices[device].stopping = true;
            }
            
            if (Wagman::getCurrent(device) < 100) {
                Serial.print('?');
                Serial.print(device);
                Serial.println(" current lost! stopping!");
                devices[device].stopping = true;
            }
        } else {
            if (millis() - devices[device].heartbeatTime > HOURS(8)) // oscasionally switch disk / unless heartbeat exists...but don't be aggressive.
                devices[device].stopping = true;
        }

        // check if device ready to begin stopping
        if (devices[device].stopping) {
            Serial.print('?');
            Serial.print(device);
            Serial.println(" beginning stop");
            Record::incrementBootFailures(device);
            devices[device].stopTime = millis();
        }
    }
}

void loop()
{
    wdt_reset();

    if (millis() - lastDeviceStartTime > SECONDS(15)) {
        bootNextReadyDevice();
        lastDeviceStartTime = millis();
    }

    for (int i = 0; i < 5; i++) {
        updateDevice(i);
    }

    Wagman::toggleLED(0);
}

