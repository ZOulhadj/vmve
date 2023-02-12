#include "vmve.h"


#include <fstream>
#include <ostream>

static bool vmve_decrypt(vmve_file_format& file_format, std::string& out_raw_data)
{
    const vmve_header& header = file_format.header;

    // parse the vmve file to figure out which encryption method was used
    if (header.encryption_mode == vmve_encryption_mode::aes) {
        out_raw_data = decrypt_aes(file_format.data);
    }

    return true;
}

void vmve_write_to_file(vmve_file_format& file_format, const char* path)
{
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(&file_format), sizeof(vmve_file_format));
}

bool vmve_read_from_file(const char* path, std::string& out_data)
{
    vmve_file_format file_format{};

    std::ifstream file(path, std::ios::binary);
    file.read(reinterpret_cast<char*>(&file_format), sizeof(vmve_file_format));

    // decrypt file
    bool vmve_decrypted = vmve_decrypt(file_format, out_data);

    return vmve_decrypted;
}

