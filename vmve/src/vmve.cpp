#include "vmve.hpp"


#include <fstream>
#include <ostream>

void vmve_write_to_file(VMVE& file_format, const char* path) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(&file_format), sizeof(VMVE));
}

VMVE vmve_read_from_file(const char* path) {
    VMVE file_format{};

    std::ifstream file(path, std::ios::binary);
    file.read(reinterpret_cast<char*>(&file_format), sizeof(VMVE));

    return file_format;
}

