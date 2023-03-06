#ifndef VMVE_VMVE_H
#define VMVE_VMVE_H

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


enum class encryption_mode
{
    aes,
    dh,
    gc,
    rc6
};

struct encryption_keys
{
    std::string key;
    std::string iv;
};

struct vmve_header
{
    std::string version;
    encryption_mode encrypt_mode;
    encryption_keys keys;
};

struct vmve_data
{
    std::string encrypted_data;
};

struct vmve_file
{
    vmve_header header;
    vmve_data   data;

    // cereal serialization
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(header.version, header.encrypt_mode, header.keys.key, header.keys.iv, data.encrypted_data);
    }
};

encryption_keys generate_key_iv(unsigned char keyLength);
encryption_keys key_iv_to_hex(encryption_keys& keys);

// AES

std::string encrypt_aes(const std::string& text, encryption_keys& keys);
std::string encrypt_aes(const std::string& text, unsigned char keyLength);
std::string decrypt_aes(const std::string& encrypted_text, const encryption_keys& keys);

bool vmve_write_to_file(vmve_file& file_format, const std::string& path);
bool vmve_read_from_file(std::string& out_data, const std::string& path);




#endif
