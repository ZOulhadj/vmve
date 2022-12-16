#include "time.hpp"

Duration GetDuration(TimePoint start, TimePoint end /*= Clock::now()*/)
{
    const auto duration = end - start;

    const int hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    const int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count() % 60;
    const int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;

    return { hours, minutes, seconds };
}
