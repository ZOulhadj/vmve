#include "windows.hpp"

MEMORYSTATUSEX GetMemoryStatus()
{
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    return memInfo;
}