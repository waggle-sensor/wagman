#include <Arduino.h>

class Device
{
    public:

        Device();
        void start();
        void stop(bool failure=false);
        void kill();
        void update();

        const char *name;
        byte port;
        byte bootSelector;
        bool started;
        byte primaryMedia;
        byte secondaryMedia;

        unsigned long heartbeatTime;

    private:

        void checkHeartbeat();
        void checkCurrent();
        void checkStopConditions();

        bool stopping;
        bool managed;
        unsigned long startTime;
        unsigned long stopTime;

        byte repeatedResetCount;
        
        unsigned long brownoutTime; // how do we infer a reset from this?
        unsigned long brownoutWarnTime;
        
        int lastHeartbeat;
};

