#ifndef VMVE_SECURITY_H
#define VMVE_SECURITY_H

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

key_iv generate_key_iv(unsigned char keyLength);
key_iv_string key_iv_to_hex(key_iv& keyIV);

// AES

encrypted_data encrypt_aes(const std::string& text, key_iv& keyIV);
encrypted_data encrypt_aes(const std::string& text, unsigned char keyLength);
std::string decrypt_aes(const encrypted_data& data);

// DH

encrypted_data encrypt_dh(const std::string& text, key_iv& keys);
std::string decrypt_dh(const encrypted_data& data);


#endif
