#include "Device.h"
#include "Wagman.h"
#include "Record.h"
#include "Logger.h"

#define SECONDS(x) ((unsigned long)(1000 * (x)))
#define MINUTES(x) ((unsigned long)(60000 * (x)))
#define HOURS(x) ((unsigned long)(3600000 * (x)))

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

bool Device::started() const
{
    return state == STATE_STARTED;
}

bool Device::stopped() const
{
    return state == STATE_STOPPED;
}

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

void Device::start()
{
    switch (state)
    {
        case STATE_STARTED:
            Logger::begin(name);
            Logger::log("already started");
            Logger::end();
            break;
        case STATE_STOPPING:
            Logger::begin(name);
            Logger::log("busy stopping");
            Logger::end();
            break;
        case STATE_STOPPED:
            managed = Record::getBootFailures(port) < 30;
        
            int bootMedia = getBootMedia();
            
            Wagman::setBootMedia(bootSelector, bootMedia);

            Logger::begin(name);
            Logger::log("starting ");
            Logger::log((bootMedia == primaryMedia) ? "primary" : "secondary");
            Logger::end();
        
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
            Logger::begin(name);
            Logger::log("already stopped");
            Logger::end();
            break;
        case STATE_STOPPING:
            Logger::begin(name);
            Logger::log("already stopping");
            Logger::end();
            break;
        case STATE_STARTED:
            Logger::begin(name);
            Logger::log("stopping");
            Logger::end();

            changeState(STATE_STOPPING);
            break;
    }
}

void Device::kill()
{
    Logger::begin(name);
    Logger::log("killing");
    Logger::end();

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
    updateState();
}

void Device::updateHeartbeat()
{
    int heartbeat = Wagman::getHeartbeat(port);

    if (heartbeat != lastHeartbeat) {
        lastHeartbeat = heartbeat;
        heartbeatTime = millis();

        Logger::begin(name);
        Logger::log("heartbeat");
        Logger::end();
    }
}

void Device::updateFault()
{
    bool newAboveFault = Wagman::getCurrent(port) > Record::getFaultCurrent(port);

    if (aboveFault != newAboveFault) {
        faultModeStartTime = millis();
        aboveFault = newAboveFault;

        Logger::begin(name);
        Logger::log("fault");
        Logger::log(aboveFault ? "ok" : "warn");
        Logger::end();
    }
}

void Device::updateState()
{
    switch (state)
    {
        case STATE_STOPPED:
            if (aboveFault && faultModeTime() > SECONDS(15)) {
                Logger::begin(name);
                Logger::log("current detected");
                Logger::end();
                
                start();
            }
            break;
        case STATE_STARTED:
            if (heartbeatTimeout()) {
                Logger::begin(name);
                Logger::log("heartbeat timeout");
                Logger::end();
                
                stop();
            }
    
            if (!aboveFault && faultModeTime() > SECONDS(15)) {
                Logger::begin(name);
                Logger::log("fault timeout");
                Logger::end();
                
                stop();
            }
            break;
        case STATE_STOPPING:
            
            if (stopTimeout()) {
                Logger::begin(name);
                Logger::log("stop timeout");
                Logger::end();
                
                kill();
            }
            break;
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

