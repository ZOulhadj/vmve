#include "pch.h"
#include "time.h"

namespace engine
{
    void time::calculate_delta_time()
    {
        static auto last_frame_time = std::chrono::high_resolution_clock::now();
        auto current_frame_time = std::chrono::high_resolution_clock::now();
        m_delta_time = std::chrono::duration<float, std::milli>(current_frame_time - last_frame_time).count() / 1000.0f;
        last_frame_time = current_frame_time;
    }

    float time::get_delta_time() const
    {
        return m_delta_time;
    }
}