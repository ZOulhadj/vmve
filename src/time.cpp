#include "time.hpp"

#include "logging.hpp"

Duration GetDuration(typedef_time_point start, typedef_time_point end /*= clock::now()*/)
{
    const auto duration = end - start;

    const int hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    const int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count() % 60;
    const int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;

    return { hours, minutes, seconds };
}

Timer::Timer()
{
    m_StartTime = std::chrono::high_resolution_clock::now();
}

float Timer::ElapsedMillis()
{
    using namespace std::chrono;

    return duration_cast<milliseconds>(high_resolution_clock::now() - m_StartTime).count();
}

ScopedTimer::ScopedTimer(const std::string& name)
    : m_Name(name)
{

}

ScopedTimer::~ScopedTimer()
{
    const float duration = m_Timer.ElapsedMillis();

    Logger::Info("[TIMER] {} - {}ms", m_Name, duration);
}
