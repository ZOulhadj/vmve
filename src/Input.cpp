#include "Input.hpp"

#include "Renderer/Renderer.hpp"

bool IsKeyDown(int keycode) {
    // HACK: The renderer has a pointer to the window which we can use for now.
    const RendererContext* rc = GetRendererContext();

    const int state = glfwGetKey(rc->window->handle, keycode);

    return state == GLFW_PRESS;
}

bool IsMouseButtonDown(int buttoncode) {
    return false;
}