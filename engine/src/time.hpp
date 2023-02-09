#ifndef MY_ENGINE_TIME_HPP
#define MY_ENGINE_TIME_HPP


struct Duration
{
    int hours;
    int minutes;
    int seconds;
};


using typedef_clock      = std::chrono::high_resolution_clock;
using typedef_time_point = typedef_clock::time_point;

Duration get_duration(typedef_time_point start, typedef_time_point end = typedef_clock::now());


#endif