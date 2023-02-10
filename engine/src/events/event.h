#ifndef MYENGINE_EVENT_HPP
#define MYENGINE_EVENT_HPP


enum class Event_Type {
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
    window_focused,
    window_lost_focus,
    window_maximized,
    window_restored,
    window_minimized,
    window_not_minimized,
    window_resized,
    window_dropped
};




#define EVENT_CLASS_TYPE(type) static Event_Type get_static_type() { return Event_Type::type; } \
Event_Type get_type() const override { return get_static_type(); }                           \
const char* get_name() const override { return #type; }


struct basic_event {
    virtual Event_Type get_type() const = 0;
    virtual const char* get_name() const = 0;

    bool Handled = false;
};



using Event_Func = std::function<void(basic_event&)>;
//void (*EventFunc)(event&);
//#define BIND_EVENT(x) std::bind(&x, this, std::placeholders::_1)

#endif