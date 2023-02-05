#include "window.hpp"

#include "events/window_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"


#include "logging.hpp"


static void glfw_error_callback(int code, const char* description) {
    logger::error("GLFW error ({} : {})", code, description);
}

static void window_close_callback(GLFWwindow* window) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    Window_Closed_Event e;
    ptr->event_callback(e);
}

static void window_focus_callback(GLFWwindow* window, int focused) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (focused) {
        Window_Focused_Event e;
        ptr->event_callback(e);
    } else {
        Window_Lost_Focus_Event e;
        ptr->event_callback(e);
    }
}

static void window_maximized_callback(GLFWwindow* window, int maximized) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (maximized) {
        Window_Maximized_Event e;
        ptr->event_callback(e);
    } else {
        Window_Restored_Event e;
        ptr->event_callback(e);
    }
}

static void window_minimized_callback(GLFWwindow* window, int minimized) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (minimized) {
        Window_Minimized_Event e;
        ptr->event_callback(e);
    } else {
        Window_Not_Minimized_Event e;
        ptr->event_callback(e);
    }
}

static void window_resize_callback(GLFWwindow* window, int width, int height) {
    // todo: window resizing is done within the framebuffer callback since that
    // todo: returns the actual pixel count of the display. This ensures that
    // todo: for monitors with a high DPI we return the real pixels.
}

static void window_framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);
    ptr->width  = width;
    ptr->height = height;

    Window_Resized_Event e(width, height);
    ptr->event_callback(e);
}

static void window_drop_callback(GLFWwindow* window, int path_count, const char* in_paths[]) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    // todo: should this be handled in the event?
    std::vector<std::string> paths(path_count);
    for (std::size_t i = 0; i < paths.size(); ++i) {
        paths[i] = in_paths[i];
    }

    Window_Dropped_Event e(paths);
    ptr->event_callback(e);
}

static void window_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        Key_Pressed_Event e(key);
        ptr->event_callback(e);
    } else if (action == GLFW_REPEAT) {
        Key_Released_Event e(key);
        ptr->event_callback(e);
    } else if (action == GLFW_RELEASE) {
        Key_Released_Event e(key);
        ptr->event_callback(e);
    }
}

static void window_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        Mouse_Button_Pressed_Event e(button);
        ptr->event_callback(e);
    } else if (action == GLFW_REPEAT) {
        Mouse_Button_Pressed_Event e(button);
        ptr->event_callback(e);
    } else if (action == GLFW_RELEASE) {
        Mouse_Button_Released_Event e(button);
        ptr->event_callback(e);
    }
}

static void window_mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (yoffset == 1.0) {
        Mouse_Scrolled_Up_Event e;
        ptr->event_callback(e);
    } else if (yoffset == -1.0) {
        Mouse_Scrolled_Down_Event e;
        ptr->event_callback(e);
    }
}

static void window_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    Mouse_Moved_Event e(xpos, ypos);
    ptr->event_callback(e);
}

static void window_cursor_enter_callback(GLFWwindow* window, int entered) {
    Window* ptr = (Window*)glfwGetWindowUserPointer(window);

    if (entered) {
        Mouse_Entered_Event e;
        ptr->event_callback(e);
    } else {
        Mouse_Left_Event e;
        ptr->event_callback(e);
    }
}

// Initialized the GLFW library and creates a window. Window callbacks send
// events to the application callback.
bool create_window(Window*& out_window, const char* name, int width, int height)
{
    logger::info("Initializing window ({}, {})", width, height);

    assert(width > 0 && height > 0);

    out_window = new Window();



    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        logger::error("Failed to initialize GLFW");

        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    out_window->handle = glfwCreateWindow(width, height, name, nullptr, nullptr);
    out_window->name   = name;
    out_window->width  = width;
    out_window->height = height;

    if (!out_window->handle) {
        logger::error("Failed to create GLFW window");   
        
        glfwTerminate();

        return false;
    }

    //glfwSetInputMode(window->handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // window callbacks
    glfwSetWindowUserPointer(out_window->handle, out_window);
    glfwSetWindowCloseCallback(out_window->handle, window_close_callback);
    glfwSetWindowFocusCallback(out_window->handle, window_focus_callback);
    glfwSetWindowMaximizeCallback(out_window->handle, window_maximized_callback);
    glfwSetWindowIconifyCallback(out_window->handle, window_minimized_callback);
    glfwSetWindowSizeCallback(out_window->handle, window_resize_callback);
    glfwSetFramebufferSizeCallback(out_window->handle, window_framebuffer_resize_callback);
    glfwSetDropCallback(out_window->handle, window_drop_callback);

    // input callbacks
    glfwSetKeyCallback(out_window->handle, window_key_callback);
    glfwSetMouseButtonCallback(out_window->handle, window_mouse_button_callback);
    glfwSetScrollCallback(out_window->handle, window_mouse_scroll_callback);
    glfwSetCursorPosCallback(out_window->handle, window_cursor_position_callback);
    glfwSetCursorEnterCallback(out_window->handle, window_cursor_enter_callback);

    return true;
}

void set_window_icon(const Window* window, const std::filesystem::path& iconPath)
{
    GLFWimage images[1]{};
    images[0].pixels = stbi_load(iconPath.string().c_str(), &images[0].width, &images[0].height, 0, STBI_rgb_alpha);

    if (!images[0].pixels) {
        logger::error("Failed to load window icon {}", iconPath.string());

        stbi_image_free(images[0].pixels);
        return;
    }

    glfwSetWindowIcon(window->handle, 1, images);

    stbi_image_free(images[0].pixels);
}

void set_window_icon(const Window* window, unsigned char* data, int width, int height)
{
    GLFWimage image[1];
    image[0].width = width;
    image[0].height = height;
    image[0].pixels = data;

    glfwSetWindowIcon(window->handle, 1, image);
}

// Destroys the window and terminates the GLFW library.
void destroy_window(Window* window)
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
void update_window(Window* window) {
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
        glfwWaitEvents();
}

