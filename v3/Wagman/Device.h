#ifndef __H_DEVICE__
#define __H_DEVICE__

#include <Arduino.h>
#include "Timer.h"

// TODO Track what kind of things killed the device! This will help us determine
// if we're having current problems, heartbeat problems, etc.

const byte CURRENT_NORMAL = 0;
const byte CURRENT_STRESSED = 1;
const byte CURRENT_LOW = 2;
const byte CURRENT_HIGH = 3;

const byte STATE_DISABLED = 0;
const byte STATE_STOPPED = 1;
const byte STATE_STARTING = 2;
const byte STATE_STARTED = 3;
const byte STATE_STOPPING = 4;

class Device
{
    public:

        void init();
        void update();

        // device commands
        byte start();
        byte stop();
        byte kill();
        byte enable();
        byte disable();

        bool canStart() const;

        bool warning() const;

        byte getBootMedia() const;

        void sendExternalHeartbeat();

        unsigned long timeSinceHeartbeat() const;
        unsigned long lastHeartbeatTime() const;

        const char *name;
        byte port;
        byte bootSelector;
        byte primaryMedia;
        byte secondaryMedia;

        bool shouldForceBootMedia;
        byte forceBootMedia;

        bool watchHeartbeat;
        bool watchCurrent;

        unsigned long getStartDelay() const;
        void setStartDelay(unsigned long t);

    private:

        byte state;

        void changeState(byte newState);

        void updateHeartbeat();
        void updateFault();
        void updateState();

        void updateUnknown();
        void updateDisabled();
        void updateStopped();
        void updateStarting();
        void updateStarted();
        void updateStopping();

        void onHeartbeat();

        bool managed;

        byte repeatedResetCount;

        bool shouldRestart;

        byte currentLevel;
        Timer currentLevelTimer;

        Timer stateTimer;
        Timer stopMessageTimer;
        Timer heartbeatTimer;

        unsigned long startDelay;
};

#endif
