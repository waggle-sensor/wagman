// This file is part of the Waggle Platform.Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.For more details on the Waggle project, visit:
// http://www.wa8.gl
#include "Arduino.h"

extern bool logging;

namespace Logger {
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
};  // namespace Logger
