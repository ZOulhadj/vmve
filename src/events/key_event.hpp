#ifndef MYENGINE_KEYEVENT_HPP
#define MYENGINE_KEYEVENT_HPP

#include "event.hpp"

class key_event : public Event
{
public:
    int get_key_code() const { return m_KeyCode; }

protected:
    key_event(int keycode)
        : m_KeyCode(keycode)
    {}

private:
    int m_KeyCode;
};


class key_pressed_event : public key_event
{
public:
    key_pressed_event(int keycode)
        : key_event(keycode)
    {}

    EVENT_CLASS_TYPE(KeyPressedEvent);
};

class key_released_event : public key_event
{
public:
    key_released_event(int keycode)
        : key_event(keycode)
    {}

    EVENT_CLASS_TYPE(KeyReleasedEvent);
};

#endif