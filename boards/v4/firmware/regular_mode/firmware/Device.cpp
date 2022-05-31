// This file is part of the Waggle Platform.Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.For more details on the Waggle project, visit:
// http://www.wa8.gl
#include "Device.h"

#include "Error.h"
#include "Logger.h"
#include "Record.h"
#include "Wagman.h"

const byte PORT_NC = 0;
const byte PORT_GN = 1;
const byte PORT_CORESENSE = 2;

// const unsigned long HEARTBEAT_TIMEOUT = 300000L; // 5min
const unsigned long HEARTBEAT_TIMEOUT = 3600000L;  // 1h
const unsigned long FAULT_TIMEOUT = 10000L;
const unsigned long DETECT_CURRENT_TIMEOUT = 10000L;
const unsigned long STOP_MESSAGE_TIMEOUT = 10000L;

const unsigned int FAIL_COUNT_THRESHHOLD = 1024;

void Device::init() {
  shouldForceBootMedia = false;
  forceBootMedia = MEDIA_SD;

  currentLevel = CURRENT_LOW;

  startDelay = 0;

  setStopTimeout(60000);

  if (Record::getDeviceEnabled(port)) {
    changeState(STATE_STOPPED);
  } else {
    changeState(STATE_DISABLED);
  }
}

bool Device::canStart() const {
  return state == STATE_STOPPED && stateTimer.exceeds(startDelay);
}

unsigned long Device::timeSinceHeartbeat() const {
  return heartbeatTimer.elapsed();
}

unsigned long Device::lastHeartbeatTime() const {
  return heartbeatTimer.elapsed();
}

byte Device::getNextBootMedia() const {
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

void Device::setNextBootMedia(byte media) {
  shouldForceBootMedia = true;
  forceBootMedia = media;

  // This will allow the device to reboot itself into another media.
  Wagman::setBootMedia(bootSelector, media);
}

byte Device::start() {
  if (state == STATE_STOPPING) {
    return ERROR_INVALID_ACTION;
  }

  managed = Record::getBootFailures(port) < 30;

  /* note: depends on force boot media flag. don't change the order! */
  byte bootMedia = getNextBootMedia();

  /* override boot media only applies to next boot! */
  shouldForceBootMedia = false;

  Wagman::setBootMedia(bootSelector, bootMedia);

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

// TODO Revist state diagram.
// IDEA now we really do want to use the sensors to better determine
// the state. For us, if we want a device to be stopped, we need to know what
// the last valid state of the relay. We can gracefully degrade this by allowing
// the current sensor to override the last remembered relay state?

byte Device::stop() {
  if (state == STATE_STOPPED) {
    kill();
    return 0;
  }

  if (state == STATE_STOPPING) {
    return ERROR_INVALID_ACTION;
  }

  changeState(STATE_STOPPING);

  return 0;
}

byte Device::kill() {
  if (state == STATE_DISABLED) {
    return ERROR_INVALID_ACTION;
  }

  Record::setRelayState(port, RELAY_TURNING_OFF);
  delay(10);
  Wagman::setRelay(port, false);
  delay(500);
  Record::setRelayState(port, RELAY_OFF);
  delay(10);

  changeState(STATE_STOPPED);

  startTimer.reset();

  return 0;
}

byte Device::enable() {
  Record::setDeviceEnabled(port, true);
  changeState(STATE_STOPPED);
  return 0;
}

byte Device::disable() {
  if (port != 0) Record::setDeviceEnabled(port, false);

  kill();

  if (port != 0) changeState(STATE_DISABLED);

  return 0;
}

unsigned long Device::getStopTimeout() const { return stopTimeout; }

void Device::setStopTimeout(unsigned long timeout) { stopTimeout = timeout; }

void Device::update() {
  updateHeartbeat();
  updateFault();
  updateState();
}

void Device::updateHeartbeat() {
  if (heartbeatCounters[port] >= 4) {
    heartbeatCounters[port] = 0;
    onHeartbeat();
  }
}

void Device::updateFault() {
  unsigned int current = Wagman::getCurrent(port);
  byte newCurrentLevel;

  // current sensor error
  if (current == 0) return;

  if (current < 120) {
    newCurrentLevel = CURRENT_LOW;
  } else if (current < 300) {
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

// Probably want to move a lot of this into state handler tables to dispatch
// on events. Or, on a state change, update the function pointer for the
// update function.
void Device::updateState() {
  switch (state) {
    case STATE_DISABLED:
      updateDisabled();
      break;
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

void Device::updateDisabled() {
  // never allow node controller to remain in this state for more than a minute.
  if (port == 0 && stateTimer.exceeds(60000)) {
    enable();
  }
}

void Device::updateStopped() {}

void Device::updateStarted() {
  if (managed) {
    updateStartedManaged();
  } else {
    updateStartedUnmanaged();
  }
}

void Device::updateStartedManaged() {
  if (watchHeartbeat && heartbeatTimer.exceeds(HEARTBEAT_TIMEOUT)) {
    Record::incrementBootFailures(port);
    stop();
  }

  if (watchCurrent && currentLevelTimer.exceeds(10000)) {
    currentLevelTimer.reset();
  }
}

void Device::updateStartedUnmanaged() {
  if (stateTimer.exceeds(14400000L)) {
    setNextBootMedia((getNextBootMedia() == MEDIA_SD) ? MEDIA_EMMC : MEDIA_SD);
    stop();
  }
}

void Device::updateStopping() {
  // periodically send a stop message to the device.
  if (stopMessageTimer.exceeds(STOP_MESSAGE_TIMEOUT)) {
    stopMessageTimer.reset();
  }

  // device had sufficient time to shutdown, so kill it.
  if (stateTimer.exceeds(getStopTimeout())) {
    setStopTimeout(60000);  // hack for now...
    kill();
  }
}

void Device::changeState(int newState) {
  // reset all timers
  stateTimer.reset();
  stopMessageTimer.reset();
  heartbeatTimer.reset();
  currentLevelTimer.reset();

  state = newState;
}

void Device::sendExternalHeartbeat() { onHeartbeat(); }

unsigned long Device::getStartDelay() const { return startDelay; }

void Device::setStartDelay(unsigned long t) { startDelay = t; }

void Device::onHeartbeat() {
  heartbeatTimer.reset();

  switch (state) {
    case STATE_DISABLED:
      disable();  // repeat disable kill until no more heartbeat, just in
                  // case...
      break;
    case STATE_STOPPED:
      changeState(STATE_STARTED);  // if device is heartbeat, assume it's
                                   // stated. need to decide how to handle this.
      break;
  }
}
