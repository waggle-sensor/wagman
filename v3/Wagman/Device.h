#include <Arduino.h>

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

        int getBootMedia() const;

        unsigned long timeSinceHeartbeat() const;
        unsigned long faultModeTime() const;

        const char *name;
        byte port;
        byte bootSelector;
        byte primaryMedia;
        byte secondaryMedia;

    private:

        byte state;
        unsigned long stateStartTime;

        void changeState(byte newState);

        void updateHeartbeat();
        void updateFault();

        bool heartbeatTimeout();
        bool aboveFaultTimeout();
        bool stopTimeout();

        unsigned long heartbeatTime;

        bool managed;

        byte repeatedResetCount;

        bool aboveFault;
        unsigned long faultModeStartTime;
        unsigned long faultTime;
        
        
        int lastHeartbeat;

};

