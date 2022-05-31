// This file is part of the Waggle Platform.Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.For more details on the Waggle project, visit:
// http://www.wa8.gl
#include "Timer.h"
#include <Arduino.h>

void DurationTimer::reset() {
    start = millis();
}

unsigned long DurationTimer::elapsed() const {
    return millis() - start;
}

bool DurationTimer::exceeds(unsigned long time) const {
  return (millis() - start) > time;
}

