#ifndef MY_ENGINE_WIN32_WINDOW_HPP
#define MY_ENGINE_WIN32_WINDOW_HPP

struct Win32Window
{
    HWND handle;
    const char* title;
    int width;
    int height;
};



Win32Window* CreateWin32Window(const char* title, int width, int height);
void DestroyWin32Window(Win32Window* window);

#endif