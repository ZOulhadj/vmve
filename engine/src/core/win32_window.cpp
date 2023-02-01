#include "win32_window.hpp"


#include "logging.hpp"

static LRESULT CALLBACK win32_window_callback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

Win32_Window* create_win32_window(const char* title, int width, int height) {
    Win32_Window* window = reinterpret_cast<Win32_Window*>(malloc(sizeof(Win32_Window)));

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
        Logger::error("Failed to register win32 window class");
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
        Logger::error("Failed to create win32 window");
        return nullptr;
    }

    SetWindowLongPtrA(window->handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

    window->title = title;
    window->width = width;
    window->height = height;

    return window;
}

void destroy_win32_window(Win32_Window* window) {
    free(window);
}

LRESULT CALLBACK win32_window_callback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    Win32_Window* window = reinterpret_cast<Win32_Window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

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