#include "Device.h"
#include "Wagman.h"
#include "Record.h"

#define SECONDS(x) ((unsigned long)(1000 * (x)))
#define MINUTES(x) ((unsigned long)(60000 * (x)))
#define HOURS(x) ((unsigned long)(3600000 * (x)))

enum {
    FAULT_ABOVE = 0,
    FAULT_BELOW,
};

static const unsigned long HEARTBEAT_TIMEOUT = MINUTES(2);
static const unsigned long STOPPING_TIMEOUT = MINUTES(2);

// fix case where no device is connected, but we keep trying to boot it.
// at some point should realize there's a problem and switch over to long
// delay booting.

extern bool logging;

const char *logPrefix = "log: ";

void Device::init()
{
    changeState(STATE_STOPPED);
}

bool Device::canStart() const
{
    if (state != STATE_STOPPED)
        return false;

    // node controller always allowed to start
    if (port == 0)
        return true;

    return Record::deviceEnabled(port) && !Record::relayFailed(port);
}

// canStop?

unsigned long Device::timeSinceHeartbeat() const
{
    return millis() - heartbeatTime;
}

unsigned long Device::faultModeTime() const
{
    return millis() - faultModeStartTime;
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

bool Device::started() const
{
    return state == STATE_STARTED;
}

bool Device::stopped() const
{
    return state == STATE_STOPPED;
}

void Device::start()
{
    switch (state)
    {
        case STATE_STARTED:
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" already started");
            }
            break;
        case STATE_STOPPING:
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" busy stopping");
            }
            break;
        case STATE_STOPPED:
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

            Record::setRelayBegin(port);
            Wagman::setRelay(port, true);
            Record::setRelayEnd(port);

            heartbeatTime = millis();
            faultTime = millis();

            changeState(STATE_STARTED);

            break;
    }
}

void Device::stop()
{
    switch (state)
    {
        case STATE_STOPPED:
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" already stopped");
            }
            break;
        case STATE_STOPPING:
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" already stopping");
            }
            break;
        case STATE_STARTED:
            if (logging) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" stopping");
            }

            changeState(STATE_STOPPING);
            break;
    }
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

    changeState(STATE_STOPPED);
}

void Device::update()
{
    updateHeartbeat();
    updateFault();

    switch (state)
    {
        case STATE_STOPPED:
            if (aboveFault && faultModeTime() > SECONDS(15)) {
                if (logging) {
                    Serial.print(logPrefix);
                    Serial.print(name);
                    Serial.println(" current detected");
                }
                
                start();
            }
            break;
        case STATE_STARTED:
            if (heartbeatTimeout()) {
                if (logging) {
                    Serial.print(logPrefix);
                    Serial.print(name);
                    Serial.println(" heartbeat timeout");
                }
                
                stop();
            }
    
            if (!aboveFault && faultModeTime() > SECONDS(15)) {
                if (logging) {
                    Serial.print(logPrefix);
                    Serial.print(name);
                    Serial.println(" fault timeout");
                }
                
                stop();
            }
            break;
        case STATE_STOPPING:
            if (stopTimeout()) {
                if (logging) {
                    Serial.print(logPrefix);
                    Serial.print(name);
                    Serial.println(" stop timeout");
                }
                
                kill();
            }
            break;
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

void Device::updateFault()
{
    bool newAboveFault = Wagman::getCurrent(port) > Record::getFaultCurrent(port);

    if (aboveFault != newAboveFault) {
        faultModeStartTime = millis();
        aboveFault = newAboveFault;

        if (logging) {
            if (aboveFault) {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" fault ok");
            } else {
                Serial.print(logPrefix);
                Serial.print(name);
                Serial.println(" fault warn");
            }
        }
    }
}

bool Device::heartbeatTimeout()
{
    return (millis() - heartbeatTime) > Record::getHeartbeatTimeout(port);
}

bool Device::stopTimeout()
{
    return (millis() - stateStartTime) > Record::getStopTimeout(port);
}

void Device::changeState(byte newState)
{
    state = newState;
    stateStartTime = millis();
}

