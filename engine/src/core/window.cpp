#include "window.hpp"

#include "events/window_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"


#include "logging.hpp"


static void ErrorCallback(int code, const char* description)
{
    Logger::Error("GLFW error ({} : {})", code, description);
}

static void CloseCallback(GLFWwindow* window)
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    window_closed_event e;
    ptr->event_callback(e);
}

static void FocusCallback(GLFWwindow* window, int focused)
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (focused) {
        window_focused_event e;
        ptr->event_callback(e);
    } else {
        window_lost_focus_event e;
        ptr->event_callback(e);
    }
}

static void MaximizedCallback(GLFWwindow* window, int maximized)
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (maximized) {
        window_maximized_event e;
        ptr->event_callback(e);
    } else {
        window_restored_event e;
        ptr->event_callback(e);
    }
}

static void MinimizedCallback(GLFWwindow* window, int minimized)
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (minimized) {
        window_minimized_event e;
        ptr->event_callback(e);
    } else {
        window_not_minimized_event e;
        ptr->event_callback(e);
    }
}

static void ResizeCallback(GLFWwindow* window, int width, int height)
{
    // todo: window resizing is done within the framebuffer callback since that
    // todo: returns the actual pixel count of the display. This ensures that
    // todo: for monitors with a high DPI we return the real pixels.
}

static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);
    ptr->width  = width;
    ptr->height = height;

    window_resized_event e(width, height);
    ptr->event_callback(e);
}

static void DropCallback(GLFWwindow* window, int path_count, const char* in_paths[])
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    // todo: should this be handled in the event?
    std::vector<std::string> paths(path_count);
    for (std::size_t i = 0; i < paths.size(); ++i) {
        paths[i] = in_paths[i];
    }

    window_dropped_callback e(paths);
    ptr->event_callback(e);
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        key_pressed_event e(key);
        ptr->event_callback(e);
    } else if (action == GLFW_REPEAT) {
        key_released_event e(key);
        ptr->event_callback(e);
    } else if (action == GLFW_RELEASE) {
        key_released_event e(key);
        ptr->event_callback(e);
    }
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        mouse_button_pressed_event e(button);
        ptr->event_callback(e);
    } else if (action == GLFW_REPEAT) {
        mouse_button_pressed_event e(button);
        ptr->event_callback(e);
    } else if (action == GLFW_RELEASE) {
        mouse_button_released_event e(button);
        ptr->event_callback(e);
    }
}

static void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (yoffset == 1.0) {
        mouse_scrolled_up_event e;
        ptr->event_callback(e);
    } else if (yoffset == -1.0) {
        mouse_scrolled_down_event e;
        ptr->event_callback(e);
    }
}

static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    mouse_moved_event e(xpos, ypos);
    ptr->event_callback(e);
}

static void CursorEnterCallback(GLFWwindow* window, int entered)
{
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (entered) {
        mouse_entered_event e;
        ptr->event_callback(e);
    } else {
        mouse_left_event e;
        ptr->event_callback(e);
    }
}

// Initialized the GLFW library and creates a window. Window callbacks send
// events to the application callback.
Window* CreateWindow(const char* name, int width, int height)
{
    Logger::Info("Initializing window ({}, {})", width, height);

    assert(width > 0 && height > 0);

    Window* window = new Window();

    glfwSetErrorCallback(ErrorCallback);

    if (!glfwInit()) {
        Logger::Error("Failed to initialize GLFW");

        delete window;

        return nullptr;
    }


    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    window->handle = glfwCreateWindow(width, height, name, nullptr, nullptr);
    window->name   = name;
    window->width  = width;
    window->height = height;

    if (!window->handle) {
        Logger::Error("Failed to create GLFW window");   
        
        glfwTerminate();

        delete window;

        return nullptr;
    }

    //glfwSetInputMode(window->handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // window callbacks
    glfwSetWindowUserPointer(window->handle, window);
    glfwSetWindowCloseCallback(window->handle, CloseCallback);
    glfwSetWindowFocusCallback(window->handle, FocusCallback);
    glfwSetWindowMaximizeCallback(window->handle, MaximizedCallback);
    glfwSetWindowIconifyCallback(window->handle, MinimizedCallback);
    glfwSetWindowSizeCallback(window->handle, ResizeCallback);
    glfwSetFramebufferSizeCallback(window->handle, FramebufferResizeCallback);
    glfwSetDropCallback(window->handle, DropCallback);

    // input callbacks
    glfwSetKeyCallback(window->handle, KeyCallback);
    glfwSetMouseButtonCallback(window->handle, MouseButtonCallback);
    glfwSetScrollCallback(window->handle, MouseScrollCallback);
    glfwSetCursorPosCallback(window->handle, CursorPosCallback);
    glfwSetCursorEnterCallback(window->handle, CursorEnterCallback);

    return window;
}


void SetWindowIcon(const Window* window, const std::filesystem::path& iconPath)
{
    GLFWimage images[1]{};
    images[0].pixels = stbi_load(iconPath.string().c_str(), &images[0].width, &images[0].height, 0, STBI_rgb_alpha);

    if (!images[0].pixels)
    {
        Logger::Error("Failed to load window icon {}", iconPath.string());

        stbi_image_free(images[0].pixels);
        return;
    }

    glfwSetWindowIcon(window->handle, 1, images);

    stbi_image_free(images[0].pixels);
}

// Destroys the window and terminates the GLFW library.
void DestroyWindow(Window* window)
{
    if (!window)
        return;

    Logger::Info("Destroying window");

    glfwDestroyWindow(window->handle);
    glfwTerminate();

    delete window;
}

// Updates a window by polling for any new events since the last window update
// function call.
void UpdateWindow(Window* window)
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
    while (window->minimized || (window->width == 0 || window->height == 0))
    {
        glfwWaitEvents();
    }
}

