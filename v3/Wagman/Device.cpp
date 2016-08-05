#include "Device.h"
#include "Wagman.h"
#include "Record.h"
#include "Logger.h"
#include "Error.h"

const byte PORT_NC = 0;
const byte PORT_GN = 1;
const byte PORT_CORESENSE = 2;

const unsigned long HEARTBEAT_TIMEOUT = (unsigned long)120000;
const unsigned long FAULT_TIMEOUT = (unsigned long)10000;
const unsigned long STOP_TIMEOUT = (unsigned long)60000;
const unsigned long DETECT_CURRENT_TIMEOUT = (unsigned long)10000;
const unsigned long STOP_MESSAGE_TIMEOUT = (unsigned long)10000;

void Device::init()
{
    shouldForceBootMedia = false;
    forceBootMedia = MEDIA_SD;

    currentLevel = CURRENT_LOW;

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

    return Record::getDeviceEnabled(port);
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

byte Device::start()
{
    if (state == STATE_STARTED) {
        Logger::begin(name);
        Logger::log("already started");
        Logger::end();
        return ERROR_ALREADY_DONE;
    }

    if (state == STATE_STOPPING) {
        Logger::begin(name);
        Logger::log("busy stopping");
        Logger::end();
        return ERROR_BUSY;
    }

    if (port != PORT_NC && !Record::getDeviceEnabled(port)) {
        Logger::begin(name);
        Logger::log("not enabled");
        Logger::end();
        return ERROR_DISABLED;
    }

    if (port != PORT_NC && (Record::getRelayState(port) == RELAY_TURNING_ON || Record::getRelayState(port) == RELAY_TURNING_OFF)) {
        Logger::begin(name);
        Logger::log("relay failed");
        Logger::end();
        return ERROR_DISABLED;
    }

    managed = Record::getBootFailures(port) < 30;

    /* note: depends on force boot media flag. don't change the order! */
    byte bootMedia = getBootMedia();
    
    /* override boot media only applies to next boot! */
    shouldForceBootMedia = false;
    
    Wagman::setBootMedia(bootSelector, bootMedia);

    Logger::begin(name);
    Logger::log("starting ");
    Logger::log((bootMedia == primaryMedia) ? "primary" : "secondary");
    Logger::end();

    Record::incrementBootAttempts(port);

    Record::setRelayState(port, RELAY_TURNING_ON);
    delay(10);
    Wagman::setRelay(port, true);
    delay(100);
    Record::setRelayState(port, RELAY_ON);
    delay(10);

    changeState(STATE_STARTED);

    return 0;
}

void Device::stop()
{
    if (state == STATE_STOPPED) {
        Logger::begin(name);
        Logger::log("already stopped");
        Logger::end();
        return;
    }

    if (state == STATE_STOPPING) {
        Logger::begin(name);
        Logger::log("busy stopping");
        Logger::end();
        return;
    }

    if (port == PORT_NC) {
        Logger::begin(name);
        Logger::log("cannot stop");
        Logger::end();
        return;
    }
    
    Logger::begin(name);
    Logger::log("stopping");
    Logger::end();

    shouldRestart = false;

    changeState(STATE_STOPPING);
}

void Device::restart()
{
    // simulated restart...but really should ask device to restart itself.
    if (state == STATE_STARTED) {
        stop();

        Logger::begin(name);
        Logger::log("will restart");
        Logger::end();
    }

    shouldRestart = true;
}

void Device::kill()
{
    Logger::begin(name);
    Logger::log("killing");
    Logger::end();

    Record::setRelayState(port, RELAY_TURNING_OFF);
    delay(10);
    Wagman::setRelay(port, false);
    delay(100);
    Record::setRelayState(port, RELAY_OFF);
    delay(10);

    changeState(STATE_STOPPED);
}

void Device::update()
{
    if (Record::getDeviceEnabled(port)) { // maybe cache all these device enables.
        updateHeartbeat();
        updateFault();
        updateState();
    }
}

void Device::updateHeartbeat()
{
    if (heartbeatCounters[port] >= 5) {
        heartbeatCounters[port] = 0;
        
        heartbeatTimer.reset();

        Logger::begin(name);
        Logger::log("heartbeat");
        Logger::end();
    }
}

void Device::updateFault()
{
    unsigned int current = Wagman::getCurrent(port);
    byte newCurrentLevel;

    // current sensor error
    if (current == 0xFFFF)
        return;

    if (current < 120) {
        newCurrentLevel = CURRENT_LOW;
    }  else if (current < 300) {
        newCurrentLevel = CURRENT_NORMAL;
    } else if (current < 850) {
        newCurrentLevel = CURRENT_STRESSED;
    } else {
        newCurrentLevel = CURRENT_HIGH;
    }

    if (newCurrentLevel != currentLevel) {
        currentLevelTimer.reset();
        currentLevel = newCurrentLevel;
    }
}

void Device::updateState()
{
    switch (state) {
        case STATE_STOPPED:
            updateStopped();
            break;
        case STATE_STARTED:
            updateStarted();
            break;
        case STATE_STOPPING:
            updateStopping();
            break;
    }
}

void Device::updateStopped()
{
    if (watchCurrent && currentLevel != CURRENT_LOW && currentLevelTimer.exceeds(10000)) {
        Logger::begin(name);
        Logger::log("current detected");
        Logger::end();

        changeState(STATE_STARTED);
    }
}

void Device::updateStarted()
{
    if (watchHeartbeat && heartbeatTimer.exceeds(HEARTBEAT_TIMEOUT)) {
        Logger::begin(name);
        Logger::log("heartbeat timeout");
        Logger::end();

        Record::incrementBootFailures(port);
        stop();
    }

    if (watchCurrent && currentLevelTimer.exceeds(10000)) {
        Logger::begin(name);

        switch (currentLevel) {
            case CURRENT_NORMAL:
                Logger::log("current normal");
                break;
            case CURRENT_LOW:
                Logger::log("current low");
                break;
            case CURRENT_HIGH:
                Logger::log("current high");
                break;
            case CURRENT_STRESSED:
                Logger::log("current stressed");
                break;
        }
        
        Logger::end();

        currentLevelTimer.reset();
    }
}

void Device::updateStopping()
{
    // periodically send a stop message to the device.
    if (stopMessageTimer.exceeds(STOP_MESSAGE_TIMEOUT)) {
        stopMessageTimer.reset();

        Logger::begin(name);
        Logger::log("stopping");
        Logger::end();
    }

    // device had sufficient time to shutdown, so kill it.
    if (stateTimer.exceeds(STOP_TIMEOUT)) {
        Logger::begin(name);
        Logger::log("stop timeout");
        Logger::end();
        
        kill();
    }
}

void Device::changeState(byte newState)
{
    // reset all timers
    stateTimer.reset();
    stopMessageTimer.reset();
    heartbeatTimer.reset();
    currentLevelTimer.reset();

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

