#include "window.hpp"

#include "events/window_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"

// Initialized the GLFW library and creates a window. Window callbacks send
// events to the application callback.
Window* create_window(const char* name, uint32_t width, uint32_t height)
{
    auto window = new Window();

    glfwSetErrorCallback([](int error_code, const char* description) {
        printf("%s\n", description);
    });

    if (!glfwInit())
        return nullptr;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    window->handle = glfwCreateWindow(
            static_cast<int>(width),
            static_cast<int>(height),
            name,
            nullptr,
            nullptr);
    window->name   = name;
    window->width  = width;
    window->height = height;

    if (!window->handle)
        return nullptr;

    glfwSetInputMode(window->handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // window callbacks
    glfwSetWindowUserPointer(window->handle, window);
    glfwSetWindowCloseCallback(window->handle, [](GLFWwindow* window) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        window_closed_event e;
        ptr->event_callback(e);
    });

    glfwSetWindowSizeCallback(window->handle, [](GLFWwindow* window, int width, int height) {
        // todo: window resizing is done within the framebuffer callback since that
        // todo: returns the actual pixel count of the display. This ensures that
        // todo: for monitors with a high DPI we return the real pixels.
    });

    glfwSetFramebufferSizeCallback(window->handle, [](GLFWwindow* window, int width, int height) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        ptr->width = width;
        ptr->height = height;

        window_resized_event e(width, height);
        ptr->event_callback(e);
    });

    glfwSetKeyCallback(window->handle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (action == GLFW_PRESS) {
            key_pressed_event e(key);
            ptr->event_callback(e);
        }  else if (action == GLFW_REPEAT) {
            key_pressed_event e(key);
            ptr->event_callback(e);
        } else if (action == GLFW_RELEASE) {
            key_released_event e(key);
            ptr->event_callback(e);
        }
    });

    glfwSetMouseButtonCallback(window->handle, [](GLFWwindow* window, int button, int action, int mods) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (action == GLFW_PRESS) {
            mouse_button_pressed_event e(button);
            ptr->event_callback(e);
        } else if (action == GLFW_REPEAT) {
            mouse_button_pressed_event e(button);
            ptr->event_callback(e);
        } else if (action == GLFW_RELEASE) {
            mouse_button_released_event e(button);
            ptr->event_callback(e);
        }

    });

    glfwSetCursorPosCallback(window->handle, [](GLFWwindow* window, double xpos, double ypos) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        mouse_moved_event e(xpos, ypos);
        ptr->event_callback(e);
    });

    glfwSetCursorEnterCallback(window->handle, [](GLFWwindow* window, int entered) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (entered) {
            mouse_entered_event e;
            ptr->event_callback(e);
        } else {
            mouse_left_event e;
            ptr->event_callback(e);
        }
    });

    glfwSetScrollCallback(window->handle, [](GLFWwindow* window, double xoffset, double yoffset) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (yoffset == 1.0) {
            mouse_scrolled_up_event e;
            ptr->event_callback(e);
        } else if (yoffset == -1.0) {
            mouse_scrolled_down_event e;
            ptr->event_callback(e);
        }
    });

    return window;
}

// Destroys the window and terminates the GLFW library.
void destroy_window(const Window* window)
{
    glfwDestroyWindow(window->handle);
    glfwTerminate();

    delete window;
}

// Updates a window by polling for any new events since the last window update
// function call.
void update_window()
{
    glfwPollEvents();
}

