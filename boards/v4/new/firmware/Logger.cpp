// This file is part of the Waggle Platform.Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.For more details on the Waggle project, visit:
// http://www.wa8.gl
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

