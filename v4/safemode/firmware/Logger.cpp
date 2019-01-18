#include "Logger.h"

namespace Logger
{

void begin(const char *name)
{
    if (logging) {
        SerialUSB.print("log: ");
        SerialUSB.print(name);
        SerialUSB.print(' ');
    }
}

void end()
{
    if (logging) {
        SerialUSB.println();
    }
}

};

