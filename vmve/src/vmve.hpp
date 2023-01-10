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


struct VmveHeader
{
    const char* version;
    int encryption_mode;
};

struct VmveData
{
    const char* data;
};

struct Vmve
{
    VmveHeader header;
    VmveData   data;
};

void VmveWriteFile(Vmve& file_format, const char* path);
Vmve VmveReadFile(const char* path);


// Data


#endif
