#include "Timer.h"
#include <Arduino.h>

void Timer::reset()
{
    start = millis();
}

unsigned long Timer::elapsed() const
{
    return millis() - start;
}

bool Timer::exceeds(unsigned long time) const
{
    return (millis() - start) > time;
}

