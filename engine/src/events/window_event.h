#ifndef MYENGINE_WINDOWEVENT_HPP
#define MYENGINE_WINDOWEVENT_HPP

#include "event.h"

namespace engine {
    struct window_closed_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(window_closed);
    };

    struct window_focused_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(window_focused);
    };

    struct window_lost_focus_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(window_lost_focus);
    };

    struct window_maximized_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(window_maximized);
    };

    struct window_restored_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(window_restored);
    };

    struct window_minimized_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(window_minimized);
    };

    struct window_not_minimized_event : public Basic_Event
    {
        EVENT_CLASS_TYPE(window_not_minimized);
    };

    struct window_resized_event : public Basic_Event
    {
        window_resized_event(const glm::u32vec2& size)
            : size(size)
        {}

        glm::u32vec2 get_size() const { return size; }

        EVENT_CLASS_TYPE(window_resized);
    private:
        glm::u32vec2 size;
    };

    struct window_dropped_event : public Basic_Event
    {
        window_dropped_event(int count, const char* paths[])
            : path_count(count), _paths(paths)
        {

        }

        EVENT_CLASS_TYPE(window_dropped);


        int path_count;
        const char** _paths;
    };
}

#endif