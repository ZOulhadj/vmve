#ifndef MYENGINE_EVENTDISPATCHER_HPP
#define MYENGINE_EVENTDISPATCHER_HPP

#include "event.hpp"

class EventDispatcher {
public:
    EventDispatcher(Event& e)
        : m_Event(e)
    {}

    template <typename T>
    bool Dispatch(std::function<bool(T&)> func) {
        if (m_Event.GetType() != T::GetStaticType())
            return false;

        m_Event.Handled = func(*(T*)&m_Event);

        return true;
    }

private:
    Event& m_Event;
};

#endif