#ifndef __H_TIMER__
#define __H_TIMER__

const unsigned long MILLISECONDS = 1L;
const unsigned long SECONDS = 1000L * MILLISECONDS;
const unsigned long MINUTES = 60L * SECONDS;
const unsigned long HOURS = 60L * MINUTES;
const unsigned long DAYS = 24L * HOURS;

class Timer
{
    public:

        void reset();
        unsigned long elapsed() const;
        bool exceeds(unsigned long time) const;

    private:

        unsigned long start;
};

#endif
