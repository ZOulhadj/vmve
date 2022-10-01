#ifndef MYENGINE_EVENTDISPATCHER_HPP
#define MYENGINE_EVENTDISPATCHER_HPP

#include "event.hpp"

class event_dispatcher
{
public:
    event_dispatcher(Event& e)
        : m_Event(e)
    {}

    template <typename T>
    void dispatch(void (*func)(T&))
    {
        if (m_Event.GetType() != T::GetStaticType())
            return;

        func(*(T*)&m_Event);
    }

private:
    Event& m_Event;
};

#endif