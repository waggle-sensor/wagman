// ANL:waggle-license
// This file is part of the Waggle Platform.  Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.  For more details on the Waggle project, visit:
//          http://www.wa8.gl
// ANL:waggle-license
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

