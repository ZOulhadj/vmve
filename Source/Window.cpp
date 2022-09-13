#include "Window.hpp"

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
        //Event e{};
        //e.type = window_closed_event;

        //EngineCallback(e);
    });

    glfwSetWindowSizeCallback(window->handle, [](GLFWwindow* window, int width, int height) {
        //Event e{};
        //e.type = window_resized_event;

        //EngineCallback(e);

        auto window_ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        window_ptr->width = width;
        window_ptr->height = height;
    });

    glfwSetKeyCallback(window->handle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        // todo(zak): error check for if a keycode is -1 (invalid)

/*        if (action == GLFW_PRESS) {
          *//*  Event pressed_event{};
            pressed_event.type = key_pressed_event;

            EngineCallback(pressed_event);*//*
        } else if (action == GLFW_RELEASE) {
            *//*Event released_event{};
            released_event.type = key_released_event;

            EngineCallback(released_event);*//*
        }*/


        /*if (action == GLFW_PRESS || action == GLFW_REPEAT)
            //keys[key] = true;
        else if (action == GLFW_RELEASE)
            //keys[key] = false;*/
    });

    glfwSetCursorPosCallback(window->handle, [](GLFWwindow* window, double xpos, double ypos) {
        /*Event e{};

        EngineCallback(e);

        cursor_x = static_cast<float>(xpos);
        cursor_y = static_cast<float>(ypos);*/
    });

    glfwSetScrollCallback(window->handle, [](GLFWwindow* window, double xoffset, double yoffset) {
/*        if (yoffset == -1.0) {
            Event scrolled_forward{};
            scrolled_forward.type = mouse_scrolled_forward_event;

            EngineCallback(scrolled_forward);
        } else if (yoffset == 1.0) {
            Event scrolled_backward{};
            scrolled_backward.type = mouse_scrolled_backwards_event;

            EngineCallback(scrolled_backward);
        }


        scroll_offset = yoffset;*/
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