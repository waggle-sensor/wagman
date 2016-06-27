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

void Device::init()
{
    started = false;
    stopping = false;
}

bool Device::canStart() const
{
    if (started)
        return false;

    // node controller always allowed to start
    if (port == 0)
        return true;

    return Record::deviceEnabled(port) && !Record::relayFailed(port);
}

unsigned long Device::timeSinceHeartbeat() const
{
    return millis() - heartbeatTime;
}

int Device::getBootMedia() const
{
    if (managed) {
        if (Record::getBootFailures(port) % 4 == 3) {
            return secondaryMedia;
        } else {
            return primaryMedia;
        }
    } else {
        if (Record::getBootAttempts(port) % 2 == 1) {
            return secondaryMedia;
        } else {
            return primaryMedia;
        }
    }
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

    managed = Record::getBootFailures(port) < 30;

    int bootMedia = getBootMedia();
    
    Wagman::setBootMedia(bootSelector, bootMedia);

    if (logging) {
        if (bootMedia == primaryMedia) {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" booting primary");
        } else {
            Serial.print(logPrefix);
            Serial.print(name);
            Serial.println(" booting secondary");
        }
    }

    Record::incrementBootAttempts(port);

    // ensure relay set to on (notice that we leave it as-is if already on.)
    Record::setRelayBegin(port);
    Wagman::setRelay(port, true);
    Record::setRelayEnd(port);

    // initialize started device parameters.
    started = true;
    stopping = false;
    
    startTime = millis();
    stopTime = 0;
    heartbeatTime = millis();
    faultTime = millis();
}

void Device::stop()
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

    if (logging) {
        Serial.print(logPrefix);
        Serial.print(name);
        Serial.println(" stopping");
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
    delay(100);
    Wagman::setRelay(port, true); // set this in case the pin level is back to low?
    delay(100);
    Wagman::setRelay(port, false);
    delay(100);
    Record::setRelayEnd(port);
    delay(100);
    
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
    
        if (stopping && (millis() - stopTime > Record::getStopTimeout(port))) {
            kill();
        }
    } else {
        // looks like device is already started.
        if (Wagman::getCurrent(port) > Record::getFaultCurrent(port)) {
            start();
        }

        // do a similar check for heartbeat?
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
    if (Wagman::getCurrent(port) > Record::getFaultCurrent(port)) {
        faultDetected = false;
        faultTime = millis();
    } else if (!faultDetected) {
        faultDetected = true;
        Serial.print(logPrefix);
        Serial.print(name);
        Serial.println(" fault warning");
    }
}

void Device::checkStopConditions()
{
    if (managed) {
        if (millis() - heartbeatTime > Record::getHeartbeatTimeout(port)) {
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" heartbeat timeout!");
            }

            Record::incrementBootFailures(port);
            stop();
        } else if (millis() - faultTime > Record::getFaultTimeout(port)) {
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" fault timeout!");
            }
            
            Record::incrementBootFailures(port);
            kill();
        }
    } else {
        if (millis() - startTime > Record::getUnmanagedChangeTime(port)) {
            stop();
        }
    }
}

