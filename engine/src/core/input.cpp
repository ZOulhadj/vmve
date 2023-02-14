#include "input.h"

#include "rendering/api/vulkan/renderer.h"

bool is_key_down(int keycode)
{
    // HACK: The renderer has a pointer to the window which we can use for now.
    const vk_context& ctx = get_vulkan_context();

    const int state = glfwGetKey(ctx.window->handle, keycode);

    return state == GLFW_PRESS;
}

bool is_mouse_button_down(int buttoncode)
{
    return false;
}

glm::vec2 get_cursor_position()
{
    // HACK: The renderer has a pointer to the window which we can use for now.
    const vk_context& ctx = get_vulkan_context();

    double x, y;
    glfwGetCursorPos(ctx.window->handle, &x, &y);

    return { x, y };
}

void update_input(Camera& camera, double deltaTime)
{
    float dt = camera.speed * (float)deltaTime;
    if (is_key_down(GLFW_KEY_W))
        camera.position += camera.front_vector * dt;
    if (is_key_down(GLFW_KEY_S))
        camera.position -= camera.front_vector * dt;
    if (is_key_down(GLFW_KEY_A))
        camera.position -= camera.right_vector * dt;
    if (is_key_down(GLFW_KEY_D))
        camera.position += camera.right_vector * dt;
    if (is_key_down(GLFW_KEY_SPACE))
        camera.position += camera.up_vector * dt;
    if (is_key_down(GLFW_KEY_LEFT_CONTROL) || is_key_down(GLFW_KEY_CAPS_LOCK))
        camera.position -= camera.up_vector * dt;
    /*if (is_key_down(GLFW_KEY_Q))
        camera.roll -= camera.roll_speed * deltaTime;
    if (is_key_down(GLFW_KEY_E))
        camera.roll += camera.roll_speed * deltaTime;*/
}
