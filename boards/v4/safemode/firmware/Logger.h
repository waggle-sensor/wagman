#include "Arduino.h"

extern bool logging;

namespace Logger
{
    void begin(const char *name);
    void end();

    template <class T>
    void log(T value) {
        if (logging) {
            SerialUSB.print(value);
        }
    }

    template <class T>
    void logHex(T value) {
        if (logging) {
            SerialUSB.print(value, HEX);
        }
    }

    void logID();
    void logDate();
    void logCurrent();
    void logHeartbeat();
    void logThermistors();
};

