#include "Window.hpp"

#include "Events/WindowEvent.hpp"
#include "Events/KeyEvent.hpp"
#include "Events/MouseEvent.hpp"


#include <cstdio>

// Initialized the GLFW library and creates a window. Window callbacks send
// events to the application callback.
Window* CreateWindow(const char* name, uint32_t width, uint32_t height)
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

        WindowClosedEvent e;
        ptr->EventCallback(e);
    });

    glfwSetWindowSizeCallback(window->handle, [](GLFWwindow* window, int width, int height) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        ptr->width = width;
        ptr->height = height;

        WindowResizedEvent e(width, height);
        ptr->EventCallback(e);
    });

    glfwSetKeyCallback(window->handle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (action == GLFW_PRESS) {
            KeyPressedEvent e(key);
            ptr->EventCallback(e);
        }  else if (action == GLFW_REPEAT) {
            KeyPressedEvent e(key);
            ptr->EventCallback(e);
        } else if (action == GLFW_RELEASE) {
            KeyReleasedEvent e(key);
            ptr->EventCallback(e);
        }
    });

    glfwSetCursorPosCallback(window->handle, [](GLFWwindow* window, double xpos, double ypos) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        MouseMovedEvent e(xpos, ypos);
        ptr->EventCallback(e);
    });

    glfwSetScrollCallback(window->handle, [](GLFWwindow* window, double xoffset, double yoffset) {
        const auto ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (yoffset == 1.0) {
            MouseScrolledUpEvent e;
            ptr->EventCallback(e);
        } else if (yoffset == -1.0) {
            MouseScrolledDownEvent e;
            ptr->EventCallback(e);
        }
    });

    return window;
}

// Destroys the window and terminates the GLFW library.
void DestroyWindow(Window* window)
{
    glfwDestroyWindow(window->handle);
    glfwTerminate();

    delete window;
}

// Updates a window by polling for any new events since the last window update
// function call.
void UpdateWindow()
{
    glfwPollEvents();
}

