#include <Arduino.h>

class Device
{
    public:

        void init();
        void start();
        void stop();
        void kill();
        void update();

        bool canStart() const;

        int getBootMedia() const;

        unsigned long timeSinceHeartbeat() const;
        unsigned long timeSinceFault() const;

        const char *name;
        byte port;
        byte bootSelector;
        bool started;
        byte primaryMedia;
        byte secondaryMedia;

    private:

        void checkHeartbeat();
        void checkCurrent();
        void checkStopConditions();

        unsigned long heartbeatTime;

        bool stopping;
        bool managed;
        unsigned long startTime;
        unsigned long stopTime;

        byte repeatedResetCount;
        
        unsigned long faultTime;
        bool faultDetected;
        
        int lastHeartbeat;

};

