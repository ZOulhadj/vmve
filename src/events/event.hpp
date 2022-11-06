#ifndef MYENGINE_EVENT_HPP
#define MYENGINE_EVENT_HPP

enum class event_type {
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


#define EVENT_CLASS_TYPE(type) static event_type GetStaticType() { return event_type::type; } \
event_type GetType() const override { return GetStaticType(); }                           \
const char* GetName() const override { return #type; }

class Event {
public:
    virtual event_type GetType() const = 0;
    virtual const char* GetName() const = 0;

public:
    bool Handled = false;
};


using event_func = std::function<void(Event&)>;

#define BIND_EVENT(x) std::bind(&x, this, std::placeholders::_1)

#endif