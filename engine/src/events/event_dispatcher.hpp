#ifndef MYENGINE_EVENTDISPATCHER_HPP
#define MYENGINE_EVENTDISPATCHER_HPP

#include "event.hpp"

struct Event_Dispatcher {
    Event_Dispatcher(Basic_Event& e)
        : m_Event(e)
    {}

    template <typename T>
    bool dispatch(std::function<bool(T&)> func) {
        if (m_Event.get_type() != T::get_static_type())
            return false;

        m_Event.Handled = func(*(T*)&m_Event);

        return true;
    }

private:
    Basic_Event& m_Event;
};

#endif