#ifndef MYENGINE_KEYEVENT_HPP
#define MYENGINE_KEYEVENT_HPP

#include "event.h"

struct Key_Event : public basic_event {
    int get_key_code() const { return m_KeyCode; }

protected:
    Key_Event(int keycode)
        : m_KeyCode(keycode)
    {}

private:
    int m_KeyCode;
};

struct Key_Pressed_Event : public Key_Event {
    Key_Pressed_Event(int keycode)
        : Key_Event(keycode)
    {}

    EVENT_CLASS_TYPE(key_pressed);
};

struct Key_Released_Event : public Key_Event {
    Key_Released_Event(int keycode)
        : Key_Event(keycode)
    {}

    EVENT_CLASS_TYPE(key_released);
};

#endif