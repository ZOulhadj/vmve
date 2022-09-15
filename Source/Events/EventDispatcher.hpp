#ifndef MYENGINE_EVENTDISPATCHER_HPP
#define MYENGINE_EVENTDISPATCHER_HPP

#include "Event.hpp"

#include <functional>

class EventDispatcher
{
public:
    explicit EventDispatcher(Event& e)
        : m_Event(e)
    {}

    template <typename T>
    void Dispatch(std::function<void(T&)> func)
    {
        if (m_Event.GetType() != T::GetStaticType())
            return;

        func(*(T*)&m_Event);
    }

private:
    Event& m_Event;
};

#endif