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

struct key_iv
{
    CryptoPP::SecByteBlock key{};
    CryptoPP::SecByteBlock iv{};
};


struct key_iv_string
{
    std::string key;
    std::string iv;
};

struct encrypted_data
{
    key_iv keyIV;
    std::string data;
};

enum class encryption_mode
{
    aes,
    dh,
    gc,
    rc6
};

struct vmve_header
{
    std::string version;
    encryption_mode encrypt_mode;
};

struct vmve_file_structure
{
    vmve_header header;
    encrypted_data data;

    // cereal serialization
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(header.version, header.encrypt_mode, data.data);
    }
};

key_iv generate_key_iv(unsigned char keyLength);
key_iv_string key_iv_to_hex(key_iv& keyIV);

// AES

encrypted_data encrypt_aes(const std::string& text, key_iv& keyIV);
encrypted_data encrypt_aes(const std::string& text, unsigned char keyLength);
std::string decrypt_aes(const encrypted_data& data);

// DH

encrypted_data encrypt_dh(const std::string& text, key_iv& keys);
std::string decrypt_dh(const encrypted_data& data);

bool vmve_write_to_file(vmve_file_structure& file_format, const std::string& path);
bool vmve_read_from_file(std::string& out_data, const std::string& path);

// Data


#endif
