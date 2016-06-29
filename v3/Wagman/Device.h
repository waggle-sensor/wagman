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
        void start();
        void stop();
        void kill();
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

    private:

        byte state;

        void changeState(byte newState);

        void updateHeartbeat();
        void updateFault();
        void updateState();

        bool managed;

        byte repeatedResetCount;

        bool aboveFault;
//        unsigned long faultModeStartTime;

        Timer stateTimer;
        Timer stopMessageTimer;
        Timer heartbeatTimer;
        Timer steadyCurrentTimer;
        
        byte lastHeartbeat;

};

