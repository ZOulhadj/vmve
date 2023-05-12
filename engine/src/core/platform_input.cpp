#include "pch.h"
#include "platform_input.h"

#include "rendering/api/vulkan/vk_renderer.h"

namespace engine {

    struct Platform_Input {

    };

    bool key_pressed(int keycode)
    {
        // HACK: The renderer has a pointer to the window which we can use for now.
        const vk_context& ctx = get_vulkan_context();

        const int state = glfwGetKey(get_window_handle(ctx.window), keycode);

        return state == GLFW_PRESS;
    }

    bool mouse_button_pressed(int buttoncode)
    {
        return false;
    }

    glm::vec2 get_cursor_position()
    {
        // HACK: The renderer has a pointer to the window which we can use for now.
        const vk_context& ctx = get_vulkan_context();

        double x, y;
        glfwGetCursorPos(get_window_handle(ctx.window), &x, &y);

        return { x, y };
    }
}
