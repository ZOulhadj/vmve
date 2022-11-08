#include "input.hpp"

#include "renderer/renderer.hpp"

bool is_key_down(int keycode) {
    // HACK: The renderer has a pointer to the window which we can use for now.
    const renderer_context_t* rc = get_renderer_context();

    const int state = glfwGetKey(rc->window->handle, keycode);

    return state == GLFW_PRESS;
}

bool is_mouse_button_down(int buttoncode) {
    return false;
}

glm::vec2 get_mouse_position() {
    // HACK: The renderer has a pointer to the window which we can use for now.
    const renderer_context_t* rc = get_renderer_context();

    double x, y;
    glfwGetCursorPos(rc->window->handle, &x, &y);


    return { x, y };
}
