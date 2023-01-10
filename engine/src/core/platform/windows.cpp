#include "windows.hpp"

MEMORYSTATUSEX get_memory_status()
{
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    return memInfo;
}