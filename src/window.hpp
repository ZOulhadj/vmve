#ifndef MYENGINE_WINDOW_HPP
#define MYENGINE_WINDOW_HPP


#include "events/event.hpp"


struct window_t {
    GLFWwindow* handle;
    const char* name;
    uint32_t    width;
    uint32_t    height;

    event_func  event_callback;
};

///
/// Initialized the GLFW library and creates a window. Window callbacks send
/// events to the application callback.
//
/// @param name   The name for the current window which will be visible in the
///               title bar.
/// @param width  The window in pixels of the current window
/// @param height The height in pixels of the current window
///
window_t* create_window(const char* name, uint32_t width, uint32_t height);

///
/// Destroys the window and terminates the GLFW library.
///
/// @param window A valid pointer to a window structure to free resources.
///
void destroy_window(window_t* window);

///
/// Updates a window by polling for any new events since the last window update
/// function call.
///
/// @param window A valid pointer to a window structure to update.
///
void update_window(window_t* window);



#endif
