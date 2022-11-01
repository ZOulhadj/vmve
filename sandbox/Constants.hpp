#ifndef SANDBOX_CONSTANTS_HPP
#define SANDBOX_CONSTANTS_HPP

//
// This file contains constant values used throughout the application.
// Any value that uses the constexpr keyword is compiled at compile
// time.
//
// All values use the 'International System of units' most commonly
// known as SI units. The base units are as follows:
//
//  Symbol | Name |   Quantity
// ---------------------------
//  s       second    time
//  m       meter     distance
//  kg      kilogram  mass
//  A       ampere    electric current
//  K       kelvin    thermodynamic temperature
//  mol     mole      amount of substance
//  cd      candela   luminous intensity


// Also known as AU is the distance between the Sun and Earth.
constexpr float astronomicalUnit = 149'120'000'000.0f;

constexpr float sunRadius   = 696'340'000.0f;
constexpr float earthRadius = 6'378'137.0f;
constexpr float moonRadius  = 1'737'400.0f;

// The number of degrees earth rotates each second.
constexpr float earthAngularVelocity = 0.004178074321326839639f;

constexpr float earthToSunDistance  = 149'120'000'000.0f;
constexpr float earthToMoonDistance = 384'400'000.0f;

constexpr float lightSpeed = 299'792'458.0f; // The fastest speed in the universe

constexpr float cameraHeight = 1.70f; // The height of the camera from the ground
constexpr float cameraSpeed  = 3.57f; // The speed at which the camera travels at
constexpr float cameraWeight = 65.0f; // The weight of the camera for gravity






// Misc
constexpr float iss_altitude = 408'000.0f;
constexpr float iss_speed    = 7660.0f;

const char* appDescription = R"(
    3D Earth Visualizer is an application created and maintained by
    Zakariya Oulhadj.

)";

#endif
