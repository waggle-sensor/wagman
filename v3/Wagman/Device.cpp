#include "Device.h"
#include "Wagman.h"
#include "Record.h"
#include <avr/wdt.h>

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

void Device::start()
{
    if (started) {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" already started");
        }
        return; // consider having useful return info.
    }
    
    if (stopping) {
        if (logging) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" busy stopping");
        }
        return;
    }

    Record::incrementBootAttempts(port);
    
    if (Record::getBootFailures(port) % 4 == 3) {
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
            Serial.println(" booting secondary");
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
    managed = Record::getBootFailures(port) < 25; // this can be changed.
    startTime = millis();
    stopTime = 0;
    heartbeatTime = millis();
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
    Wagman::setRelay(port, true);
    Wagman::setRelay(port, false);
    Record::setRelayEnd(port);
    
    started = false;
    stopping = false;
}

void Device::update()
{    
    if (started) {

        updateHeartbeat();
        
        if (stopping) {
//            Serial.print(logPrefix);
//            Serial.print(name);
//            Serial.print(" stopping (");
//            Serial.print((STOPPING_TIMEOUT - (millis() - stopTime)) / 1000);
//            Serial.println(" seconds left)");

            if (logging) {
                Serial.print("wagman: stop ");
                Serial.println(name);
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

void Device::checkStopConditions()
{
    if (managed) {
        if (millis() - heartbeatTime > HEARTBEAT_TIMEOUT) {
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" heartbeat timeout!");
            }
            stop(true);
        } else if (Wagman::getCurrent(port) < 100) {
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" current lost! stopping!");
            }
            stop(true);
        }
    } else {
        if (millis() - heartbeatTime > HOURS(8)) {
            stop(false);
        }
    }
}

