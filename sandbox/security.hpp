#ifndef VMVE_SECURITY_HPP
#define VMVE_SECURITY_HPP


#include <string>


#include "cryptlib.h"
#include "rijndael.h"
#include "modes.h"
#include "files.h"
#include "osrng.h"
#include "hex.h"


struct aes_data
{
    CryptoPP::SecByteBlock key{};
    CryptoPP::SecByteBlock iv{};

    std::string data;
};

aes_data aes_encrypt(const std::string& text)
{
    aes_data data{};

    CryptoPP::AutoSeededRandomPool random_pool;

    // Set key and IV byte lengths
    data.key = CryptoPP::SecByteBlock(CryptoPP::AES::DEFAULT_KEYLENGTH);
    data.iv  = CryptoPP::SecByteBlock(CryptoPP::AES::BLOCKSIZE);

    // Generate key and IV
    random_pool.GenerateBlock(data.key, data.key.size());
    random_pool.GenerateBlock(data.iv, data.iv.size());

    // Set key and IV to encryption mode
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKeyWithIV(data.key, data.key.size(), data.iv);

    // Encrypt text
    CryptoPP::StringSource s(text, true, new CryptoPP::StreamTransformationFilter(encryption,new CryptoPP::StringSink(data.data)));


    return data;
}

std::string aes_decrypt(const aes_data& data)
{
    std::string text;

    // Set decryption key and IV
    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption;
    decryption.SetKeyWithIV(data.key, data.key.size(), data.iv);

    // Decrypt
    CryptoPP::StringSource s(data.data, true, new CryptoPP::StreamTransformationFilter(decryption, new CryptoPP::StringSink(text)));

    return text;
}


#endif
