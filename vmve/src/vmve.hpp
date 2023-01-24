#ifndef SANDBOX_VMVE_HPP
#define SANDBOX_VMVE_HPP

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


struct VMVE_Header {
    const char* version;
    int encryption_mode;
};

struct VMVE_Data {
    const char* data;
};

struct VMVE {
    VMVE_Header header;
    VMVE_Data   data;
};

void vmve_write_to_file(VMVE& file_format, const char* path);
VMVE vmve_read_from_file(const char* path);


// Data


#endif
