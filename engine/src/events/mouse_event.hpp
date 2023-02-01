#ifndef MYENGINE_MOUSEEVENT_HPP
#define MYENGINE_MOUSEEVENT_HPP

#include "event.hpp"

struct Mouse_Button_Event : public Basic_Event {
    int get_button_code() const { return m_ButtonCode; }

protected:
    Mouse_Button_Event(int buttonCode)
        : m_ButtonCode(buttonCode)
    {}

private:
    int m_ButtonCode;
};

struct Mouse_Button_Pressed_Event : public Mouse_Button_Event {
    Mouse_Button_Pressed_Event(int buttonCode)
        : Mouse_Button_Event(buttonCode)
    {}

    EVENT_CLASS_TYPE(mouse_button_pressed);
};

struct Mouse_Button_Released_Event : public Mouse_Button_Event {
    Mouse_Button_Released_Event(int buttonCode)
        : Mouse_Button_Event(buttonCode)
    {}

    EVENT_CLASS_TYPE(mouse_button_released);
};


struct Mouse_Moved_Event : public Basic_Event {
    Mouse_Moved_Event(double x, double y)
        : m_XPos(x), m_YPos(y)
    {}

    double get_x() const { return m_XPos; }
    double get_y() const { return m_YPos; }

    EVENT_CLASS_TYPE(mouse_moved);

private:
    double m_XPos;
    double m_YPos;
};

struct Mouse_Entered_Event : public Basic_Event {
    EVENT_CLASS_TYPE(mouse_entered);
};

struct Mouse_Left_Event : public Basic_Event {
    EVENT_CLASS_TYPE(mouse_left);
};

struct Mouse_Scrolled_Up_Event : public Basic_Event {
    EVENT_CLASS_TYPE(mouse_scrolled_up);
};

struct Mouse_Scrolled_Down_Event : public Basic_Event {
    EVENT_CLASS_TYPE(mouse_scrolled_down);
};

#endif