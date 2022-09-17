#ifndef MYENGINE_WINDOW_HPP
#define MYENGINE_WINDOW_HPP

#include "events/event.hpp"


struct Window
{
    GLFWwindow* handle;
    const char* name;
    uint32_t width;
    uint32_t height;

    event_func event_callback;
};

// Initialized the GLFW library and creates a window. Window callbacks send
// events to the application callback.
Window* create_window(const char* name, uint32_t width, uint32_t height);

// Destroys the window and terminates the GLFW library.
void destroy_window(const Window* window);

// Updates a window by polling for any new events since the last window update
// function call.
void update_window();



#endif
