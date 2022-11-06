#include "window.hpp"

#include "events/window_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"




static void GLFWErrorCallback(int code, const char* description) {
     printf("(%d) %s\n", code, description);
}

static void WindowCloseCallback(GLFWwindow* window) {
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    WindowClosedEvent e;
    ptr->event_callback(e);
}

static void WindowResizeCallback(GLFWwindow* window, int width, int height) {
    // todo: window resizing is done within the framebuffer callback since that
    // todo: returns the actual pixel count of the display. This ensures that
    // todo: for monitors with a high DPI we return the real pixels.
}

static void WindowFramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);
    ptr->width = width;
    ptr->height = height;

    WindowResizedEvent e(width, height);
    ptr->event_callback(e);
}

static void WindowKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        KeyPressedEvent e(key);
        ptr->event_callback(e);
    } else if (action == GLFW_REPEAT) {
        KeyPressedEvent e(key);
        ptr->event_callback(e);
    } else if (action == GLFW_RELEASE) {
        KeyReleasedEvent e(key);
        ptr->event_callback(e);
    }
}

static void WindowMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        MouseButtonPressedEvent e(button);
        ptr->event_callback(e);
    } else if (action == GLFW_REPEAT) {
        MouseButtonPressedEvent e(button);
        ptr->event_callback(e);
    } else if (action == GLFW_RELEASE) {
        MouseButtonReleasedEvent e(button);
        ptr->event_callback(e);
    }
}

static void WindowMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    if (yoffset == 1.0) {
        MouseScrolledUpEvent e;
        ptr->event_callback(e);
    } else if (yoffset == -1.0) {
        MouseScrolledDownEvent e;
        ptr->event_callback(e);
    }
}

static void WindowCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    MouseMovedEvent e(xpos, ypos);
    ptr->event_callback(e);
}

static void WindowCursorEnterCallback(GLFWwindow* window, int entered) {
    window_t* ptr = (window_t*)glfwGetWindowUserPointer(window);

    if (entered) {
        MouseEnteredEvent e;
        ptr->event_callback(e);
    } else {
        MouseLeftEvent e;
        ptr->event_callback(e);
    }
}

// Initialized the GLFW library and creates a window. Window callbacks send
// events to the application callback.
window_t* create_window(const char* name, uint32_t width, uint32_t height) {
    window_t* window = new window_t();

    glfwSetErrorCallback(GLFWErrorCallback);

    if (!glfwInit())
        return nullptr;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    window->handle = glfwCreateWindow((int)width, (int)height, name, nullptr, nullptr);
    window->name   = name;
    window->width  = width;
    window->height = height;

    if (!window->handle)
        return nullptr;

    //glfwSetInputMode(window->handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // window callbacks
    glfwSetWindowUserPointer(window->handle, window);
    glfwSetWindowCloseCallback(window->handle, WindowCloseCallback);
    glfwSetWindowSizeCallback(window->handle, WindowResizeCallback);
    glfwSetFramebufferSizeCallback(window->handle, WindowFramebufferResizeCallback);

    // input callbacks
    glfwSetKeyCallback(window->handle, WindowKeyCallback);
    glfwSetMouseButtonCallback(window->handle, WindowMouseButtonCallback);
    glfwSetScrollCallback(window->handle, WindowMouseScrollCallback);
    glfwSetCursorPosCallback(window->handle, WindowCursorPosCallback);
    glfwSetCursorEnterCallback(window->handle, WindowCursorEnterCallback);

    return window;
}

// Destroys the window and terminates the GLFW library.
void destroy_window(window_t* window) {
    if (!window)
        return;

    glfwDestroyWindow(window->handle);
    glfwTerminate();

    delete window;
}

// Updates a window by polling for any new events since the last window update
// function call.
void update_window(window_t* window) {
    glfwPollEvents();
}

