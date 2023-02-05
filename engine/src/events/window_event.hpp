#ifndef MYENGINE_WINDOWEVENT_HPP
#define MYENGINE_WINDOWEVENT_HPP

#include "event.hpp"

struct Window_Closed_Event : public basic_event {
    EVENT_CLASS_TYPE(window_closed);
};

struct Window_Focused_Event : public basic_event {
    EVENT_CLASS_TYPE(window_focused);
};

struct Window_Lost_Focus_Event : public basic_event {
    EVENT_CLASS_TYPE(window_lost_focus);
};

struct Window_Maximized_Event : public basic_event {
    EVENT_CLASS_TYPE(window_maximized);
};

struct Window_Restored_Event : public basic_event {
    EVENT_CLASS_TYPE(window_restored);
};

struct Window_Minimized_Event : public basic_event {
    EVENT_CLASS_TYPE(window_minimized);
};

struct Window_Not_Minimized_Event : public basic_event {
    EVENT_CLASS_TYPE(window_not_minimized);
};

struct Window_Resized_Event : public basic_event {
    Window_Resized_Event(uint32_t width, uint32_t height)
        : m_Width(width), m_Height(height)
    {}

    uint32_t get_width() const { return m_Width; }
    uint32_t get_height() const { return m_Height; }

    EVENT_CLASS_TYPE(window_resized);
private:
    uint32_t m_Width;
    uint32_t m_Height;
};

struct Window_Dropped_Event : public basic_event {
    Window_Dropped_Event(const std::vector<std::string>& paths)
        : m_Paths(paths)
    {}

    EVENT_CLASS_TYPE(window_dropped);
private:
    std::vector<std::string> m_Paths;
};

#endif