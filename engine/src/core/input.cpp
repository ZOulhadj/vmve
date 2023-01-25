#include "input.hpp"

#include "rendering/api/vulkan/renderer.hpp"

bool is_key_down(int keycode) {
    // HACK: The renderer has a pointer to the window which we can use for now.
    const Vulkan_Context& ctx = get_vulkan_context();

    const int state = glfwGetKey(ctx.window->handle, keycode);

    return state == GLFW_PRESS;
}

bool is_mouse_button_down(int buttoncode) {
    return false;
}

glm::vec2 get_cursor_position() {
    // HACK: The renderer has a pointer to the window which we can use for now.
    const Vulkan_Context& ctx = get_vulkan_context();

    double x, y;
    glfwGetCursorPos(ctx.window->handle, &x, &y);

    return { x, y };
}
