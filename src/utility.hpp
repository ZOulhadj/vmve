#ifndef MY_ENGINE_UTILITY_HPP
#define MY_ENGINE_UTILITY_HPP


#include "window.hpp"
#include "camera.hpp"


// Loads a plain text file from the filesytem.

std::string load_text_file(std::string_view path);

// Calculates the delta time between previous and current frame. This
// allows for frame dependent systems such as movement and translation
// to run at the same speed no matter the time difference between two
// frames.
double get_delta_time();

// For a given world-space position this function converts that into a screen-space
// local coordinate. In other words, it returns a position on the screen of
// an object in the world.
glm::vec2 world_to_screen(window_t* window, camera_t& camera, const glm::vec3& position, const glm::vec2& offset = glm::vec2(0.0f));

#endif
