#ifndef MY_ENGINE_WIN32_WINDOW_HPP
#define MY_ENGINE_WIN32_WINDOW_HPP

struct Win32_Window {
    HWND handle;
    const char* title;
    int width;
    int height;
};



Win32_Window* create_win32_window(const char* title, int width, int height);
void destroy_win32_window(Win32_Window* window);

#endif