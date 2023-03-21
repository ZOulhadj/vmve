#ifndef MY_ENGINE_KEYEVENT_HPP
#define MY_ENGINE_KEYEVENT_HPP

#include "event.h"

struct key_event : public basic_event
{
    int get_key_code() const { return m_KeyCode; }

protected:
    key_event(int keycode)
        : m_KeyCode(keycode)
    {}

private:
    int m_KeyCode;
};

struct key_pressed_event : public key_event
{
    key_pressed_event(int keycode)
        : key_event(keycode)
    {}

    EVENT_CLASS_TYPE(key_pressed);
};

struct key_released_event : public key_event
{
    key_released_event(int keycode)
        : key_event(keycode)
    {}

    EVENT_CLASS_TYPE(key_released);
};

#endif