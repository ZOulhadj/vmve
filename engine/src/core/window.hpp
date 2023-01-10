#ifndef MYENGINE_WINDOW_HPP
#define MYENGINE_WINDOW_HPP


#include "events/event.hpp"


struct Window
{
    GLFWwindow* handle;
    const char* name;
    uint32_t    width;
    uint32_t    height;

    EventFunc  event_callback;
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
Window* CreateWindow(const char* name, uint32_t width, uint32_t height);


///
/// Sets an icon for the specified window handle.
/// 
void SetWindowIcon(const Window* window, const std::filesystem::path& iconPath);

///
/// Destroys the window and terminates the GLFW library.
///
/// @param window A valid pointer to a window structure to free resources.
///
void DestroyWindow(Window* window);

///
/// Updates a window by polling for any new events since the last window update
/// function call.
///
/// @param window A valid pointer to a window structure to update.
///
void UpdateWindow(Window* window);



#endif
