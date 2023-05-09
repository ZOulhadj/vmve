#ifndef VMVE_VMVE_H
#define VMVE_VMVE_H

enum class encryption_mode
{
    aes
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
    encryption_keys encrypted_keys; // used to check if keys match input
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
        ar(header.version, header.encrypt_mode, header.encrypted_keys.key, header.encrypted_keys.iv, data.encrypted_data);
    }
};

enum class decrypt_error
{
    no_file,
    key_mismatch
};

encryption_keys generate_key_iv(unsigned int keyLength);
encryption_keys base16_to_bytes(const encryption_keys& keys);
encryption_keys bytes_to_base16(const encryption_keys& keys);

std::string encrypt_aes(const std::string& text, encryption_keys& keys, int key_size);
std::string encrypt_aes(const std::string& text, unsigned char keyLength);
std::string decrypt_aes(const std::string& encrypted_text, const encryption_keys& keys);

bool vmve_write_to_file(vmve_file& file_format, const std::string& path);
std::expected<std::string, decrypt_error> vmve_read_from_file(const std::string& path, const encryption_keys& keys);




#endif
