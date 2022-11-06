#ifndef MYENGINE_MOUSEEVENT_HPP
#define MYENGINE_MOUSEEVENT_HPP

#include "event.hpp"

class MouseButtonEvent : public Event {
public:
    int GetButtonCode() const { return m_ButtonCode; }

protected:
    MouseButtonEvent(int buttonCode)
        : m_ButtonCode(buttonCode)
    {}

private:
    int m_ButtonCode;
};

class MouseButtonPressedEvent : public MouseButtonEvent {
public:
    MouseButtonPressedEvent(int buttonCode)
        : MouseButtonEvent(buttonCode)
    {}

    EVENT_CLASS_TYPE(MouseButtonPressedEvent);
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    MouseButtonReleasedEvent(int buttonCode)
        : MouseButtonEvent(buttonCode)
    {}

    EVENT_CLASS_TYPE(MouseButtonReleasedEvent);
};


class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(double x, double y)
        : m_XPos(x), m_YPos(y)
    {}

    double GetX() const { return m_XPos; }
    double GetY() const { return m_YPos; }

    EVENT_CLASS_TYPE(MouseMovedEvent);
private:
    double m_XPos;
    double m_YPos;
};

class MouseEnteredEvent : public Event {
public:
    EVENT_CLASS_TYPE(MouseEnteredEvent);
};

class MouseLeftEvent : public Event {
public:
    EVENT_CLASS_TYPE(MouseLeftEvent);
};

class MouseScrolledUpEvent : public Event {
public:
    EVENT_CLASS_TYPE(MouseScrolledUpEvent);
};

class MouseScrolledDownEvent : public Event {
public:
    EVENT_CLASS_TYPE(MouseScrolledDownEvent);
};

#endif