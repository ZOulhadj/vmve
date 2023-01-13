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


struct KeyIV
{
    CryptoPP::SecByteBlock key{};
    CryptoPP::SecByteBlock iv{};
};


struct KeyIVString
{
    std::string key;
    std::string iv;
};

struct AES_Data
{
    KeyIV keyIV;

    std::string data;
};



KeyIV GenerateKeyIV(unsigned char keyLength);
KeyIVString KeyIVToHex(KeyIV& keyIV);

AES_Data EncryptAES(const std::string& text, KeyIV& keyIV);
AES_Data EncryptAES(const std::string& text, unsigned char keyLength);

std::string DecrptAES(const AES_Data& data);


#endif
