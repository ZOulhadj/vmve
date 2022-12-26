#include "input.hpp"

#include "rendering/vulkan/renderer.hpp"

bool is_key_down(int keycode)
{
    // HACK: The renderer has a pointer to the window which we can use for now.
    const renderer_context_t& ctx = get_renderer_context();

    const int state = glfwGetKey(ctx.window->handle, keycode);

    return state == GLFW_PRESS;
}

bool is_mouse_button_down(int buttoncode)
{
    return false;
}

glm::vec2 get_mouse_position()
{
    // HACK: The renderer has a pointer to the window which we can use for now.
    const renderer_context_t& ctx = get_renderer_context();

    double x, y;
    glfwGetCursorPos(ctx.window->handle, &x, &y);

    return { x, y };
}
