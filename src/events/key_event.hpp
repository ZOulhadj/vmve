#ifndef MYENGINE_KEYEVENT_HPP
#define MYENGINE_KEYEVENT_HPP

#include "event.hpp"

class KeyEvent : public Event
{
public:
    int GetKeyCode() const { return m_KeyCode; }

protected:
    KeyEvent(int keycode)
        : m_KeyCode(keycode)
    {}

private:
    int m_KeyCode;
};


class KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(int keycode)
        : KeyEvent(keycode)
    {}

    EVENT_CLASS_TYPE(KeyPressedEvent);
};

class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(int keycode)
        : KeyEvent(keycode)
    {}

    EVENT_CLASS_TYPE(KeyReleasedEvent);
};

#endif