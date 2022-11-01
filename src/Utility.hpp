#ifndef MY_ENGINE_UTILITY_HPP
#define MY_ENGINE_UTILITY_HPP


#include "Window.hpp"
#include "Camera.hpp"


// Loads a plain text file from the filesytem.
std::string LoadTextFile(std::string_view path) {
    std::ifstream file(path.data());
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}



// For a given world-space position this function converts that into a screen-space
// local coordinate. In other words, it returns a position on the screen of
// an object in the world.
glm::vec2 WorldToScreen(Window* window,
                        QuatCamera& camera,
                        const glm::vec3& position,
                        const glm::vec2& offset = glm::vec2(0.0f)) {
    const glm::vec2 windowSize = glm::vec2(window->width, window->height);

    const glm::vec4 clip   = camera.viewProj.proj * camera.viewProj.view * glm::vec4(position, 1.0);
    const glm::vec3 ndc    = clip.xyz() / clip.w;
    const glm::vec2 screen = ((ndc.xy() + 1.0f) / 2.0f) * windowSize + offset;

    return screen;
}

#endif
