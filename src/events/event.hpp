#ifndef MYENGINE_EVENT_HPP
#define MYENGINE_EVENT_HPP

enum class event_type {
    none = 0,

    key_pressed,
    key_released,

    mouse_button_pressed,
    mouse_button_released,
    mouse_moved,
    mouse_entered,
    mouse_left,
    mouse_scrolled_up,
    mouse_scrolled_down,

    window_closed,
    window_resized,

};


#define EVENT_CLASS_TYPE(type) static event_type get_static_type() { return event_type::type; } \
event_type get_type() const override { return get_static_type(); }                           \
const char* get_name() const override { return #type; }

struct event {
    virtual event_type get_type() const = 0;
    virtual const char* get_name() const = 0;

    bool Handled = false;
};


using event_func = std::function<void(event&)>;

#define BIND_EVENT(x) std::bind(&x, this, std::placeholders::_1)

#endif