#ifndef MYENGINE_WINDOW_HPP
#define MYENGINE_WINDOW_HPP

#include "events/event.h"

namespace engine {
    struct Platform_Window;

    Platform_Window* create_platform_window(std::string_view name, const glm::u32vec2& size, bool fullscreen = false);
    void destroy_platform_window(Platform_Window* window);

    void set_window_event_callback(Platform_Window* window, Event_Func callback);
    void set_window_icon(const Platform_Window* window, const std::filesystem::path& iconPath);
    void set_window_icon(const Platform_Window* window, unsigned char* data, int width, int height);
    void set_window_minimized(Platform_Window* window, bool status);

    std::string get_window_name(const Platform_Window* window);
    glm::u32vec2 get_window_size(const Platform_Window* window);
    float get_window_dpi_scale(const Platform_Window* window);
    void show_window(const Platform_Window* window);

    void update_window(Platform_Window* window);

    // temp:
    GLFWwindow* get_window_handle(const Platform_Window* window);
}



#endif
