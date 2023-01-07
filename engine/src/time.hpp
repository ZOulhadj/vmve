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

Duration GetDuration(typedef_time_point start, typedef_time_point end = typedef_clock::now());


class Timer
{
public:
    Timer();

    float ElapsedMillis();
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
};


class ScopedTimer
{
public:
    ScopedTimer(const std::string& name);
    ~ScopedTimer();


private:
    std::string m_Name;

    Timer m_Timer;
};



#endif