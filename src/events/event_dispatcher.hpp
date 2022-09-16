#ifndef MYENGINE_EVENTDISPATCHER_HPP
#define MYENGINE_EVENTDISPATCHER_HPP

#include "event.hpp"

#define BIND_EVENT(func) std::bind(func, std::placeholders::_1)

class event_dispatcher
{
public:
    event_dispatcher(Event& e)
        : m_Event(e)
    {}

    template <typename T>
    void dispatch(std::function<void(T&)> func)
    {
        if (m_Event.GetType() != T::GetStaticType())
            return;

        func(*(T*)&m_Event);
    }

private:
    Event& m_Event;
};

#endif