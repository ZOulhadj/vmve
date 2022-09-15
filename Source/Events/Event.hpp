#ifndef MYENGINE_EVENT_HPP
#define MYENGINE_EVENT_HPP

#include <functional>

enum class EventType
{
    None = 0,

    KeyPressedEvent,
    KeyReleasedEvent,

    WindowClosedEvent,
    WindowResizedEvent,

    MouseMovedEvent,

    ScrolledUpEvent,
    ScrolledDownEvent
};


#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; } \
EventType GetType() const override { return GetStaticType(); }                           \
const char* GetName() const override { return #type; }

class Event
{
public:
    virtual EventType GetType() const = 0;
    virtual const char* GetName() const = 0;
};


using EventFunc = std::function<void(Event&)>;


#endif