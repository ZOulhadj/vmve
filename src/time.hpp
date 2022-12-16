#ifndef MY_ENGINE_TIME_HPP
#define MY_ENGINE_TIME_HPP


struct Duration
{
    int hours;
    int minutes;
    int seconds;
};


using Clock     = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

Duration GetDuration(TimePoint start, TimePoint end = Clock::now());


#endif