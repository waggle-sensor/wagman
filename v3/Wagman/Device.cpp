#include "Device.h"
#include "Wagman.h"
#include "Record.h"
#include "Logger.h"

const unsigned long HEARTBEAT_TIMEOUT = (unsigned long)80000;
const unsigned long FAULT_TIMEOUT = (unsigned long)2500;
const unsigned long STOP_TIMEOUT = (unsigned long)60000;
const unsigned long DETECT_CURRENT_TIMEOUT = (unsigned long)10000;
const unsigned long STOP_MESSAGE_TIMEOUT = (unsigned long)5000;

void Device::init()
{
    shouldForceBootMedia = false;
    forceBootMedia = MEDIA_SD;

    changeState(STATE_STOPPED);
}

bool Device::canStart() const
{
    if (state != STATE_STOPPED) {
        return false;
    }

    // node controller always allowed to start
    if (port == 0) {
        return true;
    }

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
    return heartbeatTimer.elapsed();
}

unsigned long Device::lastHeartbeatTime() const
{
    return heartbeatTimer.elapsed();
}

byte Device::getBootMedia() const
{
    if (shouldForceBootMedia) {
        return forceBootMedia;
    }

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
    if (!canStart()) {
        Logger::begin(name);
        Logger::log("cannot start"); // maybe give reason...
        Logger::end();
        return;
    }

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

            // note: depends on force boot media flag. don't change the order!
            byte bootMedia = getBootMedia();
            
            // override boot media only applies to next boot!
            shouldForceBootMedia = false;
            
            Wagman::setBootMedia(bootSelector, bootMedia);

            Logger::begin(name);
            Logger::log("starting ");
            Logger::log((bootMedia == primaryMedia) ? "primary" : "secondary");
            Logger::end();
        
            Record::incrementBootAttempts(port);

            Record::setRelayBegin(port);
            Wagman::setRelay(port, true);
            Record::setRelayEnd(port);

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
//    Wagman::setRelay(port, true); // check this behavior
//    delay(1000);
    Wagman::setRelay(port, false);
    delay(500);
    Record::setRelayEnd(port);
    delay(100);

    changeState(STATE_STOPPED);
}

void Device::update()
{
    if (Record::deviceEnabled(port)) { // maybe cache all these device enables.
        updateHeartbeat();
        updateFault();
        updateState();
    }
}

void Device::updateHeartbeat()
{
    byte heartbeat = Wagman::getHeartbeat(port);

    if (heartbeat != lastHeartbeat) {
        lastHeartbeat = heartbeat;
        heartbeatTimer.reset();

        Logger::begin(name);
        Logger::log("heartbeat");
        Logger::end();
    }
}

void Device::updateFault()
{
    bool newAboveFault = Wagman::getCurrent(port) > Record::getFaultCurrent(port);

    if (aboveFault != newAboveFault) {
        aboveFault = newAboveFault;
        steadyCurrentTimer.reset();

        Logger::begin(name);
        Logger::log("current ");
        Logger::log(aboveFault ? "normal" : "low");
        Logger::end();
    }
}

void Device::updateState()
{
    switch (state)
    {
        case STATE_STOPPED:
            if (aboveFault && steadyCurrentTimer.exceeds(DETECT_CURRENT_TIMEOUT)) { // 15 seconds
                Logger::begin(name);
                Logger::log("current detected");
                Logger::end();
                
                start();
            }
            break;
        case STATE_STARTED:
            if (watchHeartbeat && heartbeatTimer.exceeds(HEARTBEAT_TIMEOUT)) { // 120 seconds
                Logger::begin(name);
                Logger::log("heartbeat timeout");
                Logger::end();

                Record::incrementBootFailures(port);
                stop();
            }

            if (!aboveFault && steadyCurrentTimer.exceeds(FAULT_TIMEOUT)) { // 15 seconds
                Logger::begin(name);
                Logger::log("fault timeout");
                Logger::end();

                Record::incrementBootFailures(port);
                kill();
            }
            break;
        case STATE_STOPPING:
            if (stopMessageTimer.exceeds(STOP_MESSAGE_TIMEOUT)) { // every 5 seconds, send stop message to device
                stopMessageTimer.reset();

                Logger::begin(name);
                Logger::log("wants stop");
                Logger::end();
            }

            if (stateTimer.exceeds(STOP_TIMEOUT)) {
                Logger::begin(name);
                Logger::log("stop timeout");
                Logger::end();
                
                kill();
            }

            break;
    }
}

void Device::changeState(byte newState)
{    
    stateTimer.reset();
    // these should be moved into their respective states. but for now, let's not forget anything.
    stopMessageTimer.reset();
    heartbeatTimer.reset();
    steadyCurrentTimer.reset();

    state = newState;
}

void Device::sendExternalHeartbeat()
{
    heartbeatTimer.reset();

    Logger::begin(name);
    Logger::log("external heartbeat");
    Logger::end();
}


void Timer::reset()
{
    start = millis();
}

unsigned long Timer::elapsed() const
{
    return millis() - start;
}

bool Timer::exceeds(unsigned long time) const
{
    return (millis() - start) > time;
}

