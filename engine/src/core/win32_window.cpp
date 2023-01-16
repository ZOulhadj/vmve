#include "win32_window.hpp"


#include "logging.hpp"

static LRESULT CALLBACK Win32WindowCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

Win32Window* CreateWin32Window(const char* title, int width, int height)
{
    Win32Window* window = reinterpret_cast<Win32Window*>(malloc(sizeof(Win32Window)));

    WNDCLASSEXA wndClass = {};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = Win32WindowCallback;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = GetModuleHandleA(0);
    wndClass.hIcon = nullptr;
    wndClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND + 1);
    wndClass.lpszMenuName = nullptr;
    wndClass.lpszClassName = title;
    wndClass.hIconSm = nullptr;

    if (!RegisterClassExA(&wndClass))
    {
        Logger::Error("Failed to register win32 window class");
        return nullptr;
    }

    window->handle = CreateWindowExA(
        WS_EX_OVERLAPPEDWINDOW,
        wndClass.lpszClassName,
        title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        wndClass.hInstance,
        nullptr
    );

    if (!window->handle)
    {
        Logger::Error("Failed to create win32 window");
        return nullptr;
    }

    SetWindowLongPtrA(window->handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

    window->title = title;
    window->width = width;
    window->height = height;

    return window;
}

void DestroyWin32Window(Win32Window* window)
{
    free(window);
}

LRESULT CALLBACK Win32WindowCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    Win32Window* window = reinterpret_cast<Win32Window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

    switch (uMsg)
    {
    case WM_CREATE:
        break;
    case WM_SIZE:
        if (!window)
            break;
        window->width = LOWORD(lParam);
        window->height = HIWORD(lParam);

        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        result = DefWindowProcA(hWnd, uMsg, wParam, lParam);
        break;
    }

    return result;
}