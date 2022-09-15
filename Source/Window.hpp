#ifndef MYENGINE_WINDOW_HPP
#define MYENGINE_WINDOW_HPP

#include "Events/Event.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

struct Window
{
    GLFWwindow* handle;
    const char* name;
    uint32_t width;
    uint32_t height;

    EventFunc EventCallback;
};

// Initialized the GLFW library and creates a window. Window callbacks send
// events to the application callback.
Window* CreateWindow(const char* name, uint32_t width, uint32_t height);

// Destroys the window and terminates the GLFW library.
void DestroyWindow(Window* window);

// Updates a window by polling for any new events since the last window update
// function call.
void UpdateWindow();



#endif
