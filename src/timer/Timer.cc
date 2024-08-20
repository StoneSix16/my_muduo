#include "timer/Timer.h"

std::atomic<int> Timer::s_numCreated_(0);

void Timer::restart(Timestamp now)
{   
    if(repeat_)
    {
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}

