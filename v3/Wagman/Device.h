#include <Arduino.h>

// TODO Track what kind of things killed the device! This will help us determine
// if we're having current problems, heartbeat problems, etc.

class Timer
{
    public:

        void reset();
        unsigned long elapsed() const;
        bool exceeds(unsigned long time) const;

    private:

        unsigned long start;
};

enum {
    STATE_STOPPED,
    STATE_STARTED,
    STATE_STOPPING,
};

class Device
{
    public:

        void init();
        byte start();
        void stop();
        void kill();
        void restart();
        void update();

        bool canStart() const;
        
        bool started() const;
        bool stopped() const;
        
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

    private:

        byte state;

        void changeState(byte newState);

        void updateHeartbeat();
        void updateFault();
        void updateState();

        void updateStopped(); // can explicitly inline these since only used in updateState()
        void updateStarted();
        void updateStopping();

        bool managed;

        byte repeatedResetCount;

        bool aboveFault;
//        unsigned long faultModeStartTime;
        
        bool shouldRestart;

        Timer stateTimer;
        Timer stopMessageTimer;
        Timer heartbeatTimer;
        Timer steadyCurrentTimer;
};

