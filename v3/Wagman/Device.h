#include <Arduino.h>

class Device
{
    public:

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

        void updateHeartbeat();
        void checkStopConditions();

        bool stopping;
        bool managed;
        unsigned long startTime;
        unsigned long stopTime;
        
        int lastHeartbeat;
};

