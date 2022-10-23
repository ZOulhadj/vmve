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


class KeyPressedEvent : public key_event
{
public:
    KeyPressedEvent(int keycode)
        : key_event(keycode)
    {}

    EVENT_CLASS_TYPE(KeyPressedEvent);
};

class KeyReleasedEvent : public key_event
{
public:
    KeyReleasedEvent(int keycode)
        : key_event(keycode)
    {}

    EVENT_CLASS_TYPE(KeyReleasedEvent);
};

#endif