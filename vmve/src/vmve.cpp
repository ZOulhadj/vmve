#include "vmve.hpp"


#include <fstream>
#include <ostream>

void VmveWriteFile(Vmve& file_format, const char* path)
{
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(&file_format), sizeof(Vmve));
}

Vmve VmveReadFile(const char* path)
{
    Vmve file_format{};

    std::ifstream file(path, std::ios::binary);
    file.read(reinterpret_cast<char*>(&file_format), sizeof(Vmve));

    return file_format;
}

