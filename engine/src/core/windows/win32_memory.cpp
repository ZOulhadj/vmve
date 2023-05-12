#include "pch.h"
#include "win32_memory.h"

namespace engine {
    MEMORYSTATUSEX get_windows_memory_status()
    {
        MEMORYSTATUSEX memInfo{};
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);

        return memInfo;
    }
}
