#include "security.hpp"

#include <cassert>

KeyIV GenerateKeyIV(unsigned char keyLength)
{
    assert(keyLength == 16 || keyLength == 32);

    KeyIV keyIV{};

    CryptoPP::AutoSeededRandomPool randomPool;

    // Set key and IV byte lengths
    keyIV.key = CryptoPP::SecByteBlock(keyLength);
    keyIV.iv = CryptoPP::SecByteBlock(CryptoPP::AES::BLOCKSIZE);

    // Generate key and IV
    randomPool.GenerateBlock(keyIV.key, keyIV.key.size());
    randomPool.GenerateBlock(keyIV.iv, keyIV.iv.size());

    return keyIV;
}

KeyIVString KeyIVToHex(KeyIV& keyIV)
{
    KeyIVString strings{};

    CryptoPP::HexEncoder keyHexEncoder;
    keyHexEncoder.Put(keyIV.key, sizeof(CryptoPP::byte) * keyIV.key.size());
    keyHexEncoder.MessageEnd();

    strings.key.resize(keyHexEncoder.MaxRetrievable());
    keyHexEncoder.Get((CryptoPP::byte*)&strings.key[0], strings.key.size());



    CryptoPP::HexEncoder ivHexEncoder;
    ivHexEncoder.Put(keyIV.iv, sizeof(CryptoPP::byte) * keyIV.iv.size());
    ivHexEncoder.MessageEnd();

    strings.iv.resize(ivHexEncoder.MaxRetrievable());
    ivHexEncoder.Get((CryptoPP::byte*)&strings.iv[0], strings.iv.size());

    return strings;
}


AES_Data EncryptAES(const std::string& text, KeyIV& keyIV)
{
    AES_Data data{};

    data.keyIV = keyIV;

    // Set key and IV to encryption mode
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKeyWithIV(data.keyIV.key, data.keyIV.key.size(), data.keyIV.iv);

    // Encrypt text
    CryptoPP::StringSource s(text, true, new CryptoPP::StreamTransformationFilter(encryption, new CryptoPP::StringSink(data.data)));


    return data;
}


AES_Data EncryptAES(const std::string& text, unsigned char keyLength)
{
    AES_Data data{};

    data.keyIV = GenerateKeyIV(keyLength);

    // Set key and IV to encryption mode
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKeyWithIV(data.keyIV.key, data.keyIV.key.size(), data.keyIV.iv);

    // Encrypt text
    CryptoPP::StringSource s(text, true, new CryptoPP::StreamTransformationFilter(encryption, new CryptoPP::StringSink(data.data)));


    return data;
}

std::string DecrptAES(const AES_Data& data)
{
    std::string text;

    // Set decryption key and IV
    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption;
    decryption.SetKeyWithIV(data.keyIV.key, data.keyIV.key.size(), data.keyIV.iv);

    // Decrypt
    CryptoPP::StringSource s(data.data.data(), true, new CryptoPP::StreamTransformationFilter(decryption, new CryptoPP::StringSink(text)));

    return text;
}

