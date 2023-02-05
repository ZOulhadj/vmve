#ifndef VMVE_SECURITY_HPP
#define VMVE_SECURITY_HPP


#include <string>


#include <cryptlib.h>
#include <rijndael.h>
#include <modes.h>
#include <files.h>
#include <osrng.h>
#include <hex.h>
#include <integer.h>
#include <dh.h>


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
