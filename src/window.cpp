#include "window.hpp"

#include "Events/window_event.hpp"
#include "Events/key_event.hpp"
#include "Events/mouse_event.hpp"


#include <cstdio>

// Initialized the GLFW library and creates a window. Window callbacks send
// events to the application callback.
Window* create_window(const char* name, uint32_t width, uint32_t height)
{
    auto window = new Window();

    if (!glfwInit()) {
        printf("Failed to initialize GLFW.\n");
        return nullptr;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    window->handle = glfwCreateWindow(width, height, name, nullptr, nullptr);
    window->name   = name;
    window->width  = width;
    window->height = height;

    if (!window->handle) {
        printf("Failed to create window.\n");
        return nullptr;
    }

    //glfwSetInputMode(gWindow->handle, GLFW_CURSOR, cursor_locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // window callbacks
    glfwSetWindowUserPointer(window->handle, window);
    glfwSetWindowCloseCallback(window->handle, [](GLFWwindow* window) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        window_closed_event e;
        ptr->EventCallback(e);
    });

    glfwSetWindowSizeCallback(window->handle, [](GLFWwindow* window, int width, int height) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        ptr->width = width;
        ptr->height = height;

        window_resized_event e(width, height);
        ptr->EventCallback(e);
    });

    glfwSetKeyCallback(window->handle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (action == GLFW_PRESS) {
            key_pressed_event e(key);
            ptr->EventCallback(e);
        }  else if (action == GLFW_REPEAT) {
            key_pressed_event e(key);
            ptr->EventCallback(e);
        } else if (action == GLFW_RELEASE) {
            key_released_event e(key);
            ptr->EventCallback(e);
        }
    });

    glfwSetMouseButtonCallback(window->handle, [](GLFWwindow* window, int button, int action, int mods) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (action == GLFW_PRESS) {
            mouse_button_pressed_event e(button);
            ptr->EventCallback(e);
        } else if (action == GLFW_REPEAT) {
            mouse_button_pressed_event e(button);
            ptr->EventCallback(e);
        } else if (action == GLFW_RELEASE) {
            mouse_button_released_event e(button);
            ptr->EventCallback(e);
        }

    });

    glfwSetCursorPosCallback(window->handle, [](GLFWwindow* window, double xpos, double ypos) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        mouse_moved_event e(xpos, ypos);
        ptr->EventCallback(e);
    });

    glfwSetCursorEnterCallback(window->handle, [](GLFWwindow* window, int entered) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (entered) {
            mouse_entered_event e;
            ptr->EventCallback(e);
        } else {
            mouse_left_event e;
            ptr->EventCallback(e);
        }
    });

    glfwSetScrollCallback(window->handle, [](GLFWwindow* window, double xoffset, double yoffset) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (yoffset == 1.0) {
            mouse_scrolled_up_event e;
            ptr->EventCallback(e);
        } else if (yoffset == -1.0) {
            mouse_scrolled_down_event e;
            ptr->EventCallback(e);
        }
    });

    return window;
}

// Destroys the window and terminates the GLFW library.
void destroy_window(Window* window)
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

