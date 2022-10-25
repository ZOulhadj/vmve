#ifndef MYENGINE_EVENT_HPP
#define MYENGINE_EVENT_HPP

enum class EventType
{
    None = 0,

    KeyPressedEvent,
    KeyReleasedEvent,

    MouseButtonPressedEvent,
    MouseButtonReleasedEvent,
    MouseMovedEvent,
    MouseEnteredEvent,
    MouseLeftEvent,
    MouseScrolledUpEvent,
    MouseScrolledDownEvent,

    WindowClosedEvent,
    WindowResizedEvent,

};


#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; } \
EventType GetType() const override { return GetStaticType(); }                           \
const char* GetName() const override { return #type; }

class Event
{
public:
    virtual EventType GetType() const = 0;
    virtual const char* GetName() const = 0;

public:
    bool Handled = false;
};


using EventFunc = std::function<void(Event&)>;

#define BIND_EVENT(x) std::bind(&x, this, std::placeholders::_1)

#endif