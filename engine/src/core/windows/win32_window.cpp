#include "pch.h"
#include "win32_window.h"


#include "logging.h"

static LRESULT CALLBACK win32_window_callback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

win32_window* create_win32_window(const char* title, int width, int height)
{
    win32_window* window = reinterpret_cast<win32_window*>(malloc(sizeof(win32_window)));

    WNDCLASSEXA wndClass = {};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = win32_window_callback;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = GetModuleHandleA(0);
    wndClass.hIcon = nullptr;
    wndClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND + 1);
    wndClass.lpszMenuName = nullptr;
    wndClass.lpszClassName = title;
    wndClass.hIconSm = nullptr;

    if (!RegisterClassExA(&wndClass)) {
        print_log("Failed to register win32 window class.\n");
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

    if (!window->handle) {
        print_log("Failed to create win32 window.\n");
        return nullptr;
    }

    SetWindowLongPtrA(window->handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

    window->title = title;
    window->width = width;
    window->height = height;

    return window;
}

void destroy_win32_window(win32_window* window)
{
    free(window);
}

LRESULT CALLBACK win32_window_callback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    win32_window* window = reinterpret_cast<win32_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

    switch (uMsg) {
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