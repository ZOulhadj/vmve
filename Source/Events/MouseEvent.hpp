#ifndef MYENGINE_MOUSEEVENT_HPP
#define MYENGINE_MOUSEEVENT_HPP

#include "Event.hpp"

class MouseEvent : public Event
{
public:

};

class MouseMovedEvent : public MouseEvent
{
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

class MouseScrolledUpEvent : public MouseEvent
{
public:
    EVENT_CLASS_TYPE(ScrolledUpEvent);
};

class MouseScrolledDownEvent : public MouseEvent
{
public:
    EVENT_CLASS_TYPE(ScrolledDownEvent);
};

#endif