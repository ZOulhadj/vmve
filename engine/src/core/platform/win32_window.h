#ifndef MY_ENGINE_WIN32_WINDOW_HPP
#define MY_ENGINE_WIN32_WINDOW_HPP

struct win32_window
{
    HWND handle;
    const char* title;
    int width;
    int height;
};

win32_window* create_win32_window(const char* title, int width, int height);
void destroy_win32_window(win32_window* window);

#endif