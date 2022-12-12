#include "window.hpp"

#include "events/window_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"


#include "logging.hpp"


static void glfw_error_callback(int code, const char* description)
{
     printf("(%d) %s\n", code, description);
}

static void window_close_callback(GLFWwindow* window)
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    window_closed_event e;
    ptr->event_callback(e);
}

static void window_focus_callback(GLFWwindow* window, int focused)
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    if (focused) {
        window_focused_event e;
        ptr->event_callback(e);
    } else {
        window_lost_focus_event e;
        ptr->event_callback(e);
    }
}

static void window_maximized_callback(GLFWwindow* window, int maximized)
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    if (maximized) {
        window_maximized_event e;
        ptr->event_callback(e);
    } else {
        window_restored_event e;
        ptr->event_callback(e);
    }
}

static void window_minimized_callback(GLFWwindow* window, int minimized)
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    if (minimized) {
        window_minimized_event e;
        ptr->event_callback(e);
    } else {
        window_not_minimized_event e;
        ptr->event_callback(e);
    }
}

static void window_resize_callback(GLFWwindow* window, int width, int height)
{
    // todo: window resizing is done within the framebuffer callback since that
    // todo: returns the actual pixel count of the display. This ensures that
    // todo: for monitors with a high DPI we return the real pixels.
    logger::info("Window resizing ({}, {})", width, height);
}

static void window_framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);
    ptr->width  = width;
    ptr->height = height;

    window_resized_event e(width, height);
    ptr->event_callback(e);
}

static void window_drop_callback(GLFWwindow* window, int path_count, const char* in_paths[])
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    // todo: should this be handled in the event?
    std::vector<std::string> paths(path_count);
    for (std::size_t i = 0; i < paths.size(); ++i) {
        paths[i] = in_paths[i];
    }

    window_dropped_callback e(paths);
    ptr->event_callback(e);
}

static void window_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

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

static void window_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

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

static void window_mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    if (yoffset == 1.0) {
        mouse_scrolled_up_event e;
        ptr->event_callback(e);
    } else if (yoffset == -1.0) {
        mouse_scrolled_down_event e;
        ptr->event_callback(e);
    }
}

static void window_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    mouse_moved_event e(xpos, ypos);
    ptr->event_callback(e);
}

static void window_cursor_enter_callback(GLFWwindow* window, int entered)
{
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

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
window_t* create_window(const char* name, uint32_t width, uint32_t height)
{
    window_t* window = new window_t();

    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
        return nullptr;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    window->handle = glfwCreateWindow(width, height, name, nullptr, nullptr);
    window->name   = name;
    window->width  = width;
    window->height = height;

    if (!window->handle)
        return nullptr;

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
    glfwSetCursorPosCallback(window->handle, window_cursor_pos_callback);
    glfwSetCursorEnterCallback(window->handle, window_cursor_enter_callback);

    logger::info("Created window ({}, {})", window->width, window->height);

    return window;
}

// Destroys the window and terminates the GLFW library.
void destroy_window(window_t* window)
{
    if (!window)
        return;

    logger::info("Destroying window");

    glfwDestroyWindow(window->handle);
    glfwTerminate();

    delete window;
}

// Updates a window by polling for any new events since the last window update
// function call.
void update_window(window_t* window)
{
    glfwPollEvents();
}

