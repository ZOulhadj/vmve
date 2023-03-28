#ifndef MY_ENGINE_KEYEVENT_HPP
#define MY_ENGINE_KEYEVENT_HPP

#include "event.h"

struct key_event : public basic_event
{
    int get_key_code() const { return m_KeyCode; }
    int get_mods() const { return m_Modifiers; }

    bool is_control_down() const { return m_Modifiers & GLFW_MOD_CONTROL; }
    bool is_alt_down() const { return m_Modifiers & GLFW_MOD_ALT; }
protected:
    key_event(int keycode, int mods)
        : m_KeyCode(keycode), m_Modifiers(mods)
    {}

private:
    int m_KeyCode;
    int m_Modifiers;
};

struct key_pressed_event : public key_event
{
    key_pressed_event(int keycode, int mods)
        : key_event(keycode, mods)
    {}

    EVENT_CLASS_TYPE(key_pressed);
};

struct key_released_event : public key_event
{
    key_released_event(int keycode, int mods)
        : key_event(keycode, mods)
    {}

    EVENT_CLASS_TYPE(key_released);
};

#endif