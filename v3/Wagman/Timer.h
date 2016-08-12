#ifndef __H_TIMER__
#define __H_TIMER__

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

