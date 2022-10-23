#ifndef MYENGINE_WINDOWEVENT_HPP
#define MYENGINE_WINDOWEVENT_HPP

#include "event.hpp"

class WindowClosedEvent : public Event
{
public:
    EVENT_CLASS_TYPE(WindowClosedEvent);
};

class WindowResizedEvent : public Event
{
public:
    WindowResizedEvent(uint32_t width, uint32_t height)
        : m_Width(width), m_Height(height)
    {}

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

    EVENT_CLASS_TYPE(WindowResizedEvent);
private:
    uint32_t m_Width;
    uint32_t m_Height;
};

#endif