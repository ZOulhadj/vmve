#ifndef MYENGINE_WINDOW_HPP
#define MYENGINE_WINDOW_HPP


#include "events/event.h"


struct Window
{
    GLFWwindow* handle;
    const char* name;
    uint32_t    width;
    uint32_t    height;
    bool        minimized;

    Event_Func  event_callback;
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
Window* create_window(const char* name, int width, int height);


///
/// Loads an image to be used as an icon for the specified window handle.
/// 
void set_window_icon(const Window* window, const std::filesystem::path& iconPath);

///
/// Uses the data from an array as the icon for the specified window handle.
/// 
void set_window_icon(const Window* window, unsigned char* data, int width, int height);

///
/// Destroys the window and terminates the GLFW library.
///
/// @param window A valid pointer to a window structure to free resources.
///
void destroy_window(Window* window);

///
/// Updates a window by polling for any new events since the last window update
/// function call.
///
/// @param window A valid pointer to a window structure to update.
///
void update_window(Window* window);



#endif
