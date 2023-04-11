#ifndef MY_ENGINE_UTILITY_HPP
#define MY_ENGINE_UTILITY_HPP


#include "core/platform_window.h"
#include "rendering/camera.h"

namespace engine {
    // Loads a plain text file from the filesystem.
    std::string load_file(const std::filesystem::path& path);


    // For a given world-space position this function converts that into a screen-space
    // local coordinate. In other words, it returns a position on the screen of
    // an object in the world.
    glm::vec2 world_to_screen(const Platform_Window* window, const Camera& camera, const glm::vec3& position, const glm::vec2& offset = glm::vec2(0.0f));

}


#endif
