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


struct vmve_file_header
{
    std::string version;
    int encryption_mode;
};

struct vmve_file_data
{
    const char* data;
};

struct vmve_file_format
{
    vmve_file_header header;
    vmve_file_data   data;
};

void write_vmve_file(vmve_file_format& file_format, std::string_view path)
{
    std::ofstream file(path.data(), std::ios::binary);
    file.write(reinterpret_cast<const char*>(&file_format), sizeof(vmve_file_format));
}

vmve_file_format read_vmve_file(std::string_view path)
{
    vmve_file_format file_format{};

    std::ifstream file(path.data(), std::ios::binary);
    file.read(reinterpret_cast<char*>(&file_format), sizeof(vmve_file_format));

    return file_format;
}


// Data


#endif
