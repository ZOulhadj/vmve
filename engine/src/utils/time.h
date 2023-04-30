#ifndef TIME_H
#define TIME_H

namespace engine {
    // Calculates the delta time between previous and current frame. This
    // allows for frame dependent systems such as movement and translation
    // to run at the same speed no matter the time difference between two
    // frames.
    float get_delta_time();
}


#endif