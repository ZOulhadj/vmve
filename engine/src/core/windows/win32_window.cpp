#include "pch.h"
#include "../platform_window.h"

#include "events/window_event.h"
#include "events/key_event.h"
#include "events/mouse_event.h"

#include "utils/logging.h"

namespace engine {

    struct Platform_Window {
        GLFWwindow* handle;
        std::string name;
        glm::u32vec2 size;

        bool        minimized;
        bool        fullscreen;

        Event_Func  event_callback;
    };


    static void glfw_error_callback(int code, const char* description)
    {
        error("GLFW error ({} : {}).", code, description);
    }

    static void window_close_callback(GLFWwindow* window)
    {
        const auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));

        window_closed_event e;
        ptr->event_callback(e);
    }

    static void window_focus_callback(GLFWwindow* window, int focused)
    {
        const auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));

        if (focused) {
            window_focused_event e;
            ptr->event_callback(e);
        }
        else {
            window_lost_focus_event e;
            ptr->event_callback(e);
        }
    }

    static void window_maximized_callback(GLFWwindow* window, int maximized)
    {
        const auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));

        if (maximized) {
            window_maximized_event e;
            ptr->event_callback(e);
        }
        else {
            window_restored_event e;
            ptr->event_callback(e);
        }
    }

    static void window_minimized_callback(GLFWwindow* window, int minimized)
    {
        const auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));

        if (minimized) {
            window_minimized_event e;
            ptr->event_callback(e);
        }
        else {
            window_not_minimized_event e;
            ptr->event_callback(e);
        }
    }

    static void window_resize_callback(GLFWwindow* window, int width, int height)
    {
        // todo: window resizing is done within the framebuffer callback since that
        // todo: returns the actual pixel count of the display. This ensures that
        // todo: for monitors with a high DPI we return the real pixels.
    }

    static void window_framebuffer_resize_callback(GLFWwindow* window, int width, int height)
    {
        auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));
        ptr->size = glm::u32vec2(width, height);

        window_resized_event e(ptr->size);
        ptr->event_callback(e);
    }

    static void window_drop_callback(GLFWwindow* window, int path_count, const char* in_paths[])
    {
        const auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));

        window_dropped_event e(path_count, in_paths);
        ptr->event_callback(e);
    }

    static void window_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        const auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));

        if (action == GLFW_PRESS) {
            key_pressed_event e(key, mods);
            ptr->event_callback(e);
        }
        else if (action == GLFW_REPEAT) {
            key_released_event e(key, mods);
            ptr->event_callback(e);
        }
        else if (action == GLFW_RELEASE) {
            key_released_event e(key, mods);
            ptr->event_callback(e);
        }
    }

    static void window_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
    {
        const auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));

        if (action == GLFW_PRESS) {
            mouse_button_pressed_event e(button);
            ptr->event_callback(e);
        }
        else if (action == GLFW_REPEAT) {
            mouse_button_pressed_event e(button);
            ptr->event_callback(e);
        }
        else if (action == GLFW_RELEASE) {
            mouse_button_released_event e(button);
            ptr->event_callback(e);
        }
    }

    static void window_mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
        const auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));

        if (yoffset == 1.0) {
            mouse_scrolled_up_event e;
            ptr->event_callback(e);
        }
        else if (yoffset == -1.0) {
            mouse_scrolled_down_event e;
            ptr->event_callback(e);
        }
    }

    static void window_cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
    {
        const auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));

        mouse_moved_event e(xpos, ypos);
        ptr->event_callback(e);
    }

    static void window_cursor_enter_callback(GLFWwindow* window, int entered)
    {
        const auto ptr = static_cast<Platform_Window*>(glfwGetWindowUserPointer(window));

        if (entered) {
            mouse_entered_event e;
            ptr->event_callback(e);
        }
        else {
            mouse_left_event e;
            ptr->event_callback(e);
        }
    }

    // Initialized the GLFW library and creates a window. Window callbacks send
    // events to the application callback.
    Platform_Window* create_platform_window(std::string_view name, const glm::u32vec2& size, bool fullscreen)
    {
        info("Initializing window ({}, {}).", size.x, size.y);

        assert(size.x > 0 && size.y > 0);

        Platform_Window* window = new Platform_Window();
        window->name = name;
        window->size = size;
        window->minimized = false;
        window->fullscreen = fullscreen;

        glfwSetErrorCallback(glfw_error_callback);

        if (!glfwInit()) {
            error("Failed to initialize GLFW.");

            return nullptr;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, true);
        glfwWindowHint(GLFW_VISIBLE, false);

        window->handle = glfwCreateWindow(size.x, size.y, name.data(), nullptr, nullptr);
        if (!window->handle) {
            error("Failed to create GLFW window.");

            glfwTerminate();

            return nullptr;
        }

        if (window->fullscreen) {
            int x = 0, y = 0;
            glfwGetWindowPos(window->handle, &x, &y);
            glfwSetWindowMonitor(window->handle, glfwGetPrimaryMonitor(), x, y, size.x, size.y, 0);
        }

        //glfwSetInputMode(window->handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // window callbacks
        glfwSetWindowUserPointer(window->handle, window);
        glfwSetWindowCloseCallback(window->handle, window_close_callback);
        glfwSetWindowFocusCallback(window->handle, window_focus_callback);
        glfwSetWindowMaximizeCallback(window->handle, window_maximized_callback);
        glfwSetWindowIconifyCallback(window->handle, window_minimized_callback);
        glfwSetWindowSizeCallback(window->handle, window_resize_callback);
        glfwSetFramebufferSizeCallback(window->handle, window_framebuffer_resize_callback);
        glfwSetDropCallback(window->handle, window_drop_callback);

        // input callbacks
        glfwSetKeyCallback(window->handle, window_key_callback);
        glfwSetMouseButtonCallback(window->handle, window_mouse_button_callback);
        glfwSetScrollCallback(window->handle, window_mouse_scroll_callback);
        glfwSetCursorPosCallback(window->handle, window_cursor_position_callback);
        glfwSetCursorEnterCallback(window->handle, window_cursor_enter_callback);

        return window;
    }

    void destroy_platform_window(Platform_Window* window)
    {
        if (!window)
            return;

        info("Destroying window.");

        glfwDestroyWindow(window->handle);
        glfwTerminate();

        delete window;
    }

    void set_window_event_callback(Platform_Window* window, Event_Func callback)
    {
        window->event_callback = callback;
    }

    void set_window_icon(const Platform_Window* window, const std::filesystem::path& iconPath)
    {
        GLFWimage images[1]{};
        images[0].pixels = stbi_load(iconPath.string().c_str(), &images[0].width, &images[0].height, 0, STBI_rgb_alpha);

        if (!images[0].pixels) {
            error("Failed to load window icon {}.", iconPath.string());

            stbi_image_free(images[0].pixels);
            return;
        }

        glfwSetWindowIcon(window->handle, 1, images);

        stbi_image_free(images[0].pixels);
    }

    void set_window_icon(const Platform_Window* window, unsigned char* data, int width, int height)
    {
        GLFWimage image[1];
        image[0].width = width;
        image[0].height = height;
        image[0].pixels = data;

        glfwSetWindowIcon(window->handle, 1, image);
    }

    void set_window_minimized(Platform_Window* window, bool status)
    {
        window->minimized = status;
    }

    std::string get_window_name(const Platform_Window* window)
    {
        return window->name;
    }

    glm::u32vec2 get_window_size(const Platform_Window* window)
    {
        return window->size;
    }

    float get_window_dpi_scale(const Platform_Window* window)
    {
        GLFWmonitor* monitor = glfwGetWindowMonitor(window->handle);

        // Is nullptr if in windowed mode
        if (!monitor)
            monitor = glfwGetPrimaryMonitor();

        if (!monitor)
            return 1.0f;

        float x, y;
        glfwGetMonitorContentScale(monitor, &x, &y);

        // Simply return one of the axis since they are most likely the same
        assert(x == y && "Currently, X and Y are assumed to be the same. Fix if assert is hit.");

        return x;
    }

    void show_window(const Platform_Window* window)
    {
        glfwShowWindow(window->handle);
    }


    // Updates a window by polling for any new events since the last window update
    // function call.
    void update_window(Platform_Window* window)
    {
        glfwPollEvents();

        // TODO: Possibly implement a better solution as this will not work if 
        // the client application requires continuous updates. For example,
        // if networking is being used then waiting here is not possible since
        // network updates must be sent/received.

        // If the application is minimized then only wait for events and don't
        // do anything else. This ensures the application does not waste resources
        // performing other operations such as maths and rendering when the window
        // is not visible.
        while (window->minimized || (window->size.x == 0 || window->size.y == 0))
            glfwWaitEvents();
    }

    GLFWwindow* get_window_handle(const Platform_Window* window)
    {
        return window->handle;
    }
}




#if 0
namespace engine {
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
            error("Failed to register win32 window class.");
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
            error("Failed to create win32 window.");
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
}
#endif