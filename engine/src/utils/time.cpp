#include "pch.h"
#include "time.h"

namespace engine {
    double get_delta_time()
    {

#define CHRONO 0
#if CHRONO
        static auto last_time = std::chrono::high_resolution_clock::now();
        const auto current_time = std::chrono::high_resolution_clock::now();
        using ts = std::chrono::duration<double, std::milli>;
        const double delta_time = std::chrono::duration_cast<ts>(current_time - last_time).count() / 1000.0f;
        last_time = current_time;
#else
        static double lastTime;
        double currentTime = glfwGetTime();
        double delta_time = currentTime - lastTime;
        lastTime = currentTime;
#endif

        return delta_time;
    }

    time_point get_time()
    {
        return std::chrono::high_resolution_clock::now();
    }

    float get_duration(time_point t1, time_point t2)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }
}