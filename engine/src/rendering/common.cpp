#include "pch.h"
#include "common.h"

namespace engine {
    glm::vec2 world_to_screen(const Platform_Window* window,
        const perspective_camera& camera,
        const glm::vec3& position,
        const glm::vec2& offset)
    {
        const glm::vec2 windowSize = get_window_size(window);
        const glm::vec4 clip = camera.vp.proj * camera.vp.view * glm::vec4(position, 1.0);
        const glm::vec3 ndc = clip.xyz() / clip.w;
        const glm::vec2 screen = ((ndc.xy() + 1.0f) / 2.0f) * windowSize + offset;

        return screen;
    }

}
