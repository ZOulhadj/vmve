#include "input.hpp"

#include "rendering/vulkan/renderer.hpp"

bool IsKeyDown(int keycode)
{
    // HACK: The renderer has a pointer to the window which we can use for now.
    const RendererContext& ctx = GetRendererContext();

    const int state = glfwGetKey(ctx.window->handle, keycode);

    return state == GLFW_PRESS;
}

bool IsMouseButtonDown(int buttoncode)
{
    return false;
}

glm::vec2 GetMousePos()
{
    // HACK: The renderer has a pointer to the window which we can use for now.
    const RendererContext& ctx = GetRendererContext();

    double x, y;
    glfwGetCursorPos(ctx.window->handle, &x, &y);

    return { x, y };
}
