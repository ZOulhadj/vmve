#ifndef TIME_H
#define TIME_H

namespace engine {
    using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;

    // Calculates the delta time between previous and current frame. This
    // allows for frame dependent systems such as movement and translation
    // to run at the same speed no matter the time difference between two
    // frames.
    double get_delta_time();

    time_point get_time();
    float get_duration(time_point t1, time_point t2);
    
}


#endif