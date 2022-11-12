#ifndef MYENGINE_WINDOWEVENT_HPP
#define MYENGINE_WINDOWEVENT_HPP

#include "event.hpp"

struct window_closed_event : public event {
    EVENT_CLASS_TYPE(window_closed);
};

struct window_focused_event : public event {
    EVENT_CLASS_TYPE(window_focused);
};

struct window_lost_focus_event : public event {
    EVENT_CLASS_TYPE(window_lost_focus);
};

struct window_maximized_event : public event {
    EVENT_CLASS_TYPE(window_maximized);
};

struct window_restored_event : public event {
    EVENT_CLASS_TYPE(window_restored);
};

struct window_minimized_event : public event {
    EVENT_CLASS_TYPE(window_minimized);
};

struct window_not_minimized_event : public event {
    EVENT_CLASS_TYPE(window_not_minimized);
};

struct window_resized_event : public event {
    window_resized_event(uint32_t width, uint32_t height)
        : m_Width(width), m_Height(height)
    {}

    uint32_t get_width() const { return m_Width; }
    uint32_t get_height() const { return m_Height; }

    EVENT_CLASS_TYPE(window_resized);
private:
    uint32_t m_Width;
    uint32_t m_Height;
};

#endif