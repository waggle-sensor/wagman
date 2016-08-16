#include "Device.h"
#include "Wagman.h"
#include "Record.h"
#include "Logger.h"
#include "Error.h"

const byte PORT_NC = 0;
const byte PORT_GN = 1;
const byte PORT_CORESENSE = 2;

const unsigned long HEARTBEAT_TIMEOUT = (unsigned long)300000;
const unsigned long FAULT_TIMEOUT = (unsigned long)10000;
const unsigned long STOP_TIMEOUT = (unsigned long)60000;
const unsigned long DETECT_CURRENT_TIMEOUT = (unsigned long)10000;
const unsigned long STOP_MESSAGE_TIMEOUT = (unsigned long)10000;

void Device::init()
{
    shouldForceBootMedia = false;
    forceBootMedia = MEDIA_SD;

    currentLevel = CURRENT_LOW;

    startDelay = 0;

    if (Record::getDeviceEnabled(port)) {
        changeState(STATE_STOPPED);
    } else {
        changeState(STATE_DISABLED);
    }
}

bool Device::canStart() const
{
    return state == STATE_STOPPED && stateTimer.exceeds(startDelay);
}

unsigned long Device::timeSinceHeartbeat() const
{
    return heartbeatTimer.elapsed();
}

unsigned long Device::lastHeartbeatTime() const
{
    return heartbeatTimer.elapsed();
}

byte Device::getNextBootMedia() const
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

void Device::setNextBootMedia(byte media)
{
    shouldForceBootMedia = true;
    forceBootMedia = media;
}

byte Device::start()
{
    if (state != STATE_STOPPED)
        return ERROR_INVALID_ACTION;

    managed = Record::getBootFailures(port) < 30;

    /* note: depends on force boot media flag. don't change the order! */
    byte bootMedia = getNextBootMedia();

    /* override boot media only applies to next boot! */
    shouldForceBootMedia = false;

    Wagman::setBootMedia(bootSelector, bootMedia);

    Logger::begin(name);
    Logger::log("starting ");
    Logger::log((bootMedia == primaryMedia) ? "primary" : "secondary");
    Logger::end();

    Record::incrementBootAttempts(port);

    time_t bootTime;
    Wagman::getTime(bootTime);
    Record::bootLogs[port].addEntry(bootTime);

    Record::setRelayState(port, RELAY_TURNING_ON);
    delay(10);
    Wagman::setRelay(port, true);
    delay(500);
    Record::setRelayState(port, RELAY_ON);
    delay(10);

    changeState(STATE_STARTED);

    return 0;
}

byte Device::stop()
{
    if (state != STATE_STARTED)
        return ERROR_INVALID_ACTION;

    Logger::begin(name);
    Logger::log("stopping");
    Logger::end();

    changeState(STATE_STOPPING);

    return 0;
}

byte Device::kill()
{
    if (state == STATE_DISABLED)
        return ERROR_INVALID_ACTION;

    Logger::begin(name);
    Logger::log("killing");
    Logger::end();

    Record::setRelayState(port, RELAY_TURNING_OFF);
    delay(10);
    Wagman::setRelay(port, false);
    delay(500);
    Record::setRelayState(port, RELAY_OFF);
    delay(10);

    changeState(STATE_STOPPED);
    return 0;
}

byte Device::enable()
{
    Record::setDeviceEnabled(port, true);
    changeState(STATE_STOPPED);
    return 0;
}

byte Device::disable()
{
    if (port != 0)
        Record::setDeviceEnabled(port, false);

    kill();

    if (port != 0)
        changeState(STATE_DISABLED);

    return 0;
}

void Device::update()
{
    updateHeartbeat();
    updateFault();
    updateState();
}

void Device::updateHeartbeat()
{
    // ...this is probably all we'd like exposed from the outside...not this combination...
    if (heartbeatCounters[port] >= 8) {
        heartbeatCounters[port] = 0;
        onHeartbeat();
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
        case STATE_DISABLED:
            updateDisabled();
            break;
        case STATE_STOPPED:
            updateStopped();
            break;
        case STATE_STARTING:
            updateStarting();
            break;
        case STATE_STARTED:
            updateStarted();
            break;
        case STATE_STOPPING:
            updateStopping();
            break;
    }
}

void Device::updateDisabled()
{
    // never allow node controller to remain in this state for more than a minute.
    if (port == 0 && stateTimer.exceeds(60000)) {
        enable();
    }
}

void Device::updateStopped()
{
}

void Device::updateStarting()
{
    if (stateTimer.exceeds(300000)) {
        Logger::begin(name);
        Logger::log("starting timeout");
        Logger::end();

        Record::incrementBootFailures(port);
        kill();
    }
}

void Device::updateStarted()
{
    if (managed) {
        updateStartedManaged();
    } else {
        updateStartedUnmanaged();
    }
}

void Device::updateStartedManaged()
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

void Device::updateStartedUnmanaged()
{
    if (stateTimer.exceeds(14400000)) {
        Logger::begin(name);
        Logger::log("unmanaged switch");
        Logger::end();

        setNextBootMedia((getNextBootMedia() == MEDIA_SD) ? MEDIA_EMMC : MEDIA_SD);
        stop();
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
    onHeartbeat();
}

unsigned long Device::getStartDelay() const
{
    return startDelay;
}

void Device::setStartDelay(unsigned long t)
{
    startDelay = t;
}


void Device::onHeartbeat()
{
    heartbeatTimer.reset();

    Logger::begin(name);
    Logger::log("heartbeat");
    Logger::end();

    switch (state) {
        case STATE_DISABLED:
            disable(); // repeat disable kill until no more heartbeat, just in case...
            break;
        case STATE_STOPPED:
        case STATE_STARTING:
            changeState(STATE_STARTED);
            break;
    }
}
