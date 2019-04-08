// This file is part of the Waggle Platform.Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.For more details on the Waggle project, visit:
// http://www.wa8.gl
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
