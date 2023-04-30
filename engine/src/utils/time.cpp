#include "pch.h"
#include "time.h"

namespace engine
{
    float get_delta_time()
    {
        static auto last_frame_time = std::chrono::high_resolution_clock::now();
        auto current_frame_time = std::chrono::high_resolution_clock::now();
        const auto delta_time = std::chrono::duration<float, std::milli>(current_frame_time - last_frame_time).count() / 1000.0f;
        last_frame_time = current_frame_time;

        return delta_time;
    }
}