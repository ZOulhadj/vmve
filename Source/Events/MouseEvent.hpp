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

    EVENT_CLASS_TYPE(CursorMovedEvent);
private:
    double m_XPos;
    double m_YPos;
};

class ScrolledForwardEvent : public MouseEvent
{
public:
    EVENT_CLASS_TYPE(ScrolledForwardEvent);
};

class ScrolledBackwardEvent : public MouseEvent
{
public:
    EVENT_CLASS_TYPE(ScrolledBackwardEvent);
};

#endif