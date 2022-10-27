#ifndef SANDBOX_CONSTANTS_HPP
#define SANDBOX_CONSTANTS_HPP


// This is the applications scale factor. This value gets applied to every
// object in the scene in order to maintain correct scaling values.
constexpr float scale_factor = 1.0f;

// This is the applications speed factor relative to real world speeds.
// A value of 10 means that the simulation will run 10% faster than
// world speed.
constexpr int speed_factor = 1.0f;

// angular velocity of earth in degrees per second
// radians 0.0000729211533f
constexpr float angular_velocity = 0.004178074321326839639f * speed_factor;

constexpr float sun_radius = 696'340'000.0f * scale_factor;
constexpr float earth_radius = 6'378'137.0f * scale_factor;
constexpr float moon_radius = 1'737'400.0f * scale_factor;
constexpr float iss_altitude = 408'000.0f * scale_factor;

constexpr float iss_speed    = 0.07725304476742584f * speed_factor;

constexpr float sun_from_earth = 149'120'000'000.0f * scale_factor;
constexpr float moon_from_earth = 384'400'000.0f * scale_factor;

const char* application_about = R"(
    3D Earth Visualizer is an application created and maintained by
    Zakariya Oulhadj.

)";

#endif
