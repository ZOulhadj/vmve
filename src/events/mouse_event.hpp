#ifndef MYENGINE_MOUSEEVENT_HPP
#define MYENGINE_MOUSEEVENT_HPP

#include "event.hpp"

class mouse_button_event : public Event
{
public:
    int GetButtonCode() const { return m_ButtonCode; }

protected:
    mouse_button_event(int buttonCode)
        : m_ButtonCode(buttonCode)
    {}

private:
    int m_ButtonCode;
};

class mouse_button_pressed_event : public mouse_button_event
{
public:
    mouse_button_pressed_event(int buttonCode)
        : mouse_button_event(buttonCode)
    {}

    EVENT_CLASS_TYPE(MouseButtonPressedEvent);
};

class mouse_button_released_event : public mouse_button_event
{
public:
    mouse_button_released_event(int buttonCode)
        : mouse_button_event(buttonCode)
    {}

    EVENT_CLASS_TYPE(MouseButtonReleasedEvent);
};


class mouse_moved_event : public Event
{
public:
    mouse_moved_event(double x, double y)
        : m_XPos(x), m_YPos(y)
    {}

    double GetX() const { return m_XPos; }
    double GetY() const { return m_YPos; }

    EVENT_CLASS_TYPE(MouseMovedEvent);
private:
    double m_XPos;
    double m_YPos;
};

class mouse_entered_event : public Event
{
public:
    EVENT_CLASS_TYPE(MouseEnteredEvent);
};

class mouse_left_event : public Event
{
public:
    EVENT_CLASS_TYPE(MouseLeftEvent);
};

class mouse_scrolled_up_event : public Event
{
public:
    EVENT_CLASS_TYPE(MouseScrolledUpEvent);
};

class mouse_scrolled_down_event : public Event
{
public:
    EVENT_CLASS_TYPE(MouseScrolledDownEvent);
};

#endif