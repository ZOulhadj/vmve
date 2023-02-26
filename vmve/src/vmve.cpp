#include "pch.h"
#include "vmve.h"

key_iv generate_key_iv(unsigned char keyLength)
{
    assert(keyLength == 16 || keyLength == 32);

    key_iv keyIV{};

    CryptoPP::AutoSeededRandomPool randomPool;

    // Set key and IV byte lengths
    keyIV.key = CryptoPP::SecByteBlock(keyLength);
    keyIV.iv = CryptoPP::SecByteBlock(CryptoPP::AES::BLOCKSIZE);

    // Generate key and IV
    randomPool.GenerateBlock(keyIV.key, keyIV.key.size());
    randomPool.GenerateBlock(keyIV.iv, keyIV.iv.size());

    return keyIV;
}

key_iv_string key_iv_to_hex(key_iv& keyIV)
{
    key_iv_string strings{};

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


encrypted_data encrypt_aes(const std::string& text, key_iv& keyIV)
{
    encrypted_data data{};

    data.keyIV = keyIV;

    // Set key and IV to encryption mode
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKeyWithIV(data.keyIV.key, data.keyIV.key.size(), data.keyIV.iv);

    // Encrypt text
    CryptoPP::StringSource s(text, true, new CryptoPP::StreamTransformationFilter(encryption, new CryptoPP::StringSink(data.data)));


    return data;
}


encrypted_data encrypt_aes(const std::string& text, unsigned char keyLength)
{
    encrypted_data data{};

    data.keyIV = generate_key_iv(keyLength);

    // Set key and IV to encryption mode
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKeyWithIV(data.keyIV.key, data.keyIV.key.size(), data.keyIV.iv);

    // Encrypt text
    CryptoPP::StringSource s(text, true, new CryptoPP::StreamTransformationFilter(encryption, new CryptoPP::StringSink(data.data)));


    return data;
}

std::string decrypt_aes(const encrypted_data& data)
{
    std::string text;

    // Set decryption key and IV
    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption;
    decryption.SetKeyWithIV(data.keyIV.key, data.keyIV.key.size(), data.keyIV.iv);

    // Decrypt
    CryptoPP::StringSource s(data.data.data(), true, new CryptoPP::StreamTransformationFilter(decryption, new CryptoPP::StringSink(text)));

    return text;
}

encrypted_data encrypt_dh(const std::string& text, key_iv& keys)
{
    return {};
}

std::string decrypt_dh(const encrypted_data& data)
{
    return "";
}

static bool vmve_decrypt(vmve_file_structure& file_format, std::string& out_raw_data)
{
    const vmve_header& header = file_format.header;

    // parse the vmve file to figure out which encryption method was used
    if (header.encrypt_mode == encryption_mode::aes) {
        out_raw_data = decrypt_aes(file_format.data);
    }

    return true;
}

bool vmve_write_to_file(vmve_file_structure& file_format, const std::string& path)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    cereal::BinaryOutputArchive output(file);
    output(file_format);

    return true;
}

bool vmve_read_from_file(std::string& out_data, const std::string& path)
{
    vmve_file_structure file_structure;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    cereal::BinaryInputArchive input(file);
    input(file_structure);

    // decrypt file
    bool vmve_decrypted = vmve_decrypt(file_structure, out_data);

    return vmve_decrypted;
}

