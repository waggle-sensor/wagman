#include "Logger.h"
#include "Arduino.h"

namespace Logger
{

void begin(const char *name)
{
    if (logging) {
        Serial.print("log: ");
        Serial.print(name);
        Serial.print(' ');
    }
}

void end()
{
    if (logging) {
        Serial.println();
    }
}

void log(const char *str)
{
    if (logging) {
        Serial.print(str);
    }
}

};

