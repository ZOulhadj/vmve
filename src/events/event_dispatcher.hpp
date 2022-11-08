#ifndef MYENGINE_EVENTDISPATCHER_HPP
#define MYENGINE_EVENTDISPATCHER_HPP

#include "event.hpp"

struct event_dispatcher {
    event_dispatcher(event& e)
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
    event& m_Event;
};

#endif