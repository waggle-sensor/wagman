#include "Logger.h"

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

};

