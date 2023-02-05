#include "time.hpp"

#include "logging.hpp"

Duration get_duration(typedef_time_point start, typedef_time_point end /*= clock::now()*/) {
    const auto duration = end - start;

    const int hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    const int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count() % 60;
    const int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;

    return { hours, minutes, seconds };
}

Timer::Timer() {
    m_StartTime = std::chrono::high_resolution_clock::now();
}

float Timer::elapsed_millis() {
    using namespace std::chrono;

    return duration_cast<milliseconds>(high_resolution_clock::now() - m_StartTime).count();
}

Scoped_Timer::Scoped_Timer(const std::string& name)
    : m_Name(name) {

}

Scoped_Timer::~Scoped_Timer() {
    const float duration = m_Timer.elapsed_millis();

    logger::info("[TIMER] {} - {}ms", m_Name, duration);
}
