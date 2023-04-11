#ifndef MY_ENGINE_WINDOWS_HPP
#define MY_ENGINE_WINDOWS_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace engine {
    MEMORYSTATUSEX get_windows_memory_status();
}

#endif