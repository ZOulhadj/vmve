#ifndef MYENGINE_MOUSEEVENT_HPP
#define MYENGINE_MOUSEEVENT_HPP

#include "event.h"

namespace engine {
    struct mouse_button_event : public Basic_Event
    {
        int get_button_code() const { return m_ButtonCode; }

    protected:
        mouse_button_event(int buttonCode)
            : m_ButtonCode(buttonCode)
        {}

    private:
        int m_ButtonCode;
    };

    struct mouse_button_pressed_event : public mouse_button_event
    {
        mouse_button_pressed_event(int buttonCode)
            : mouse_button_event(buttonCode)
        {}

        EVENT_CLASS_TYPE(mouse_button_pressed);
    };

    struct mouse_button_released_event : public mouse_button_event
    {
        mouse_button_released_event(int buttonCode)
            : mouse_button_event(buttonCode)
        {}

        EVENT_CLASS_TYPE(mouse_button_released);
    };


    struct mouse_moved_event : public Basic_Event
    {
        mouse_moved_event(double x, double y)
            : m_XPos(x), m_YPos(y)
        {}

        double get_x() const { return m_XPos; }
        double get_y() const { return m_YPos; }

        EVENT_CLASS_TYPE(mouse_moved);

    private:
        double m_XPos;
        double m_YPos;
    };

    struct mouse_entered_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(mouse_entered);
    };

    struct mouse_left_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(mouse_left);
    };

    struct mouse_scrolled_up_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(mouse_scrolled_up);
    };

    struct mouse_scrolled_down_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(mouse_scrolled_down);
    };
}

#endif