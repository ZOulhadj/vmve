#ifndef VMVE_H
#define VMVE_H

// TODO: Create a custom file format

// VMVE file format draft design
// 
// +----------- HEADER ------------+
// | Application version (v0.0.1)  |
// | Encryption mode (AES, DH)     |
// +------------ DATA -------------+
// | Scene Graph / Model Data      |
// +-------------------------------+
//

#include "security.h"

enum class vmve_encryption_mode
{
    aes
};

struct vmve_header
{
    const char* version;
    vmve_encryption_mode encryption_mode;
};

struct vmve_file_format
{
    vmve_header header;
    encrypted_data data;
};

void vmve_write_to_file(vmve_file_format& file_format, const char* path);
bool vmve_read_from_file(const char* path, std::string& out_data);

// Data


#endif
