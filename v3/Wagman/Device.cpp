#include "Device.h"
#include "Wagman.h"
#include "Record.h"

#define SECONDS(x) ((unsigned long)(1000 * (x)))
#define MINUTES(x) ((unsigned long)(60000 * (x)))
#define HOURS(x) ((unsigned long)(3600000 * (x)))

static const unsigned long HEARTBEAT_TIMEOUT = MINUTES(2);
static const unsigned long STOPPING_TIMEOUT = MINUTES(2);

// fix case where no device is connected, but we keep trying to boot it.
// at some point should realize there's a problem and switch over to long
// delay booting.

extern bool logging;

const char *logPrefix = "log: ";

Device::Device()
{
    started = false;
    stopping = false;
}

void Device::start()
{
    if (started) {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" already started");
        }
        return;
    }
    
    if (stopping) {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" busy stopping");
        }
        return;
    }

    // prevent guest nodes from booting on a relay fault
    if (Record::setRelayFailed(port) && port != 0) {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" relay tripped");
        }
    }

    Record::incrementBootAttempts(port);

    // can alter this strategy with something better if we need.
    if (Record::getBootFailures(port) % 2 == 1) {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" booting secondary");
        }
        Wagman::setBootMedia(bootSelector, secondaryMedia);
    } else {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" booting primary");
        }
        Wagman::setBootMedia(bootSelector, primaryMedia);
    }

    // ensure relay set to on (notice that we leave it as-is if already on.)
    Record::setRelayBegin(port);
    Wagman::setRelay(port, true);
    Record::setRelayEnd(port);

    // initialize started device parameters.
    started = true;
    stopping = false;

    // unmanaged mode is where we're supposing 
    managed = Record::getBootFailures(port) < 25;
    
    startTime = millis();
    stopTime = 0;
    heartbeatTime = millis();
    brownoutTime = millis();
}

void Device::stop(bool failure)
{
    if (!started) {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" not started");
        }
        return;
    }
    
    if (stopping) {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" already stopping");
        }
        return;
    }

    if (failure) {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" stopping (failure)");
        }
        Record::incrementBootFailures(port); // using device <-> mapping...
    } else {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" stopping");
        }
    }

    stopping = true;
    stopTime = millis();
}

void Device::kill()
{
    if (logging) {
        Serial.print(logPrefix);
        Serial.print(name);
        Serial.println(" killing!");
    }

    Record::setRelayBegin(port);
    Wagman::setRelay(port, true); // had this in case pin was low, but relay was closed...
    Wagman::setRelay(port, false);
    Record::setRelayEnd(port);
    
    started = false;
    stopping = false;
}

void Device::update()
{
    if (started) {
        checkHeartbeat();
        checkCurrent();
    
        if (!stopping) {
            checkStopConditions();
        }
    
        if (stopping && (millis() - stopTime > MINUTES(3))) {
            kill();
        }
    } else {
        // need to also detect if we think a device is not connected.
//        if (Wagman::getCurrent(port) > 110) { // store minimal autostart current in EEPROM
//            start();
//        }
    }
}

void Device::checkHeartbeat()
{
    int heartbeat = Wagman::getHeartbeat(port);

    if (heartbeat != lastHeartbeat) {
        lastHeartbeat = heartbeat;
        heartbeatTime = millis();

        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" heartbeat");
        }
    }
}

void Device::checkCurrent()
{
    if (Wagman::getCurrent(port) > Record::getBrownoutCurrent(port)) {
        brownoutTime = millis();
        brownoutWarnTime = millis(); // quick hack to log correctly.
    } else if (logging) {
        if (millis() - brownoutWarnTime > SECONDS(1)) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" possible brownout");
            brownoutWarnTime = millis();
        }
    }
}

void Device::checkStopConditions()
{
    unsigned long currentTime = millis();

    if (managed) {
        if (currentTime - heartbeatTime > SECONDS(120)) { // switch to use EEPROM
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" heartbeat timeout!");
            }
            stop(true);
        }

        if (currentTime - brownoutTime > SECONDS(5)) { // switch to use EEPROM
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" brownout! killing!");
            }
            Record::incrementBootFailures(port); // Record::incrementBootFailures(port);
            kill();
        }
    } else {
        if (currentTime - heartbeatTime > HOURS(8)) {
            stop(false);
        }
    }
}

