#include "pch.h"
#include "vmve.h"

encryption_keys generate_key_iv(unsigned char keyLength)
{
    encryption_keys keys{};

    CryptoPP::AutoSeededRandomPool randomPool;

    keys.key.resize(keyLength);
    keys.iv.resize(CryptoPP::AES::BLOCKSIZE);

    randomPool.GenerateBlock(reinterpret_cast<CryptoPP::byte*>(keys.key.data()), keys.key.size());
    randomPool.GenerateBlock(reinterpret_cast<CryptoPP::byte*>(keys.iv.data()), keys.iv.size());

    return keys;
}

encryption_keys key_iv_to_hex(encryption_keys& keyIV)
{
    encryption_keys key_strings;

    CryptoPP::HexEncoder keyHexEncoder;
    keyHexEncoder.Put(reinterpret_cast<const CryptoPP::byte*>(keyIV.key.data()), sizeof(CryptoPP::byte) * keyIV.key.size());
    keyHexEncoder.MessageEnd();

    key_strings.key.resize(keyHexEncoder.MaxRetrievable());
    keyHexEncoder.Get((CryptoPP::byte*)&key_strings.key[0], key_strings.key.size());

    CryptoPP::HexEncoder ivHexEncoder;
    ivHexEncoder.Put(reinterpret_cast<const CryptoPP::byte*>(keyIV.iv.data()), sizeof(CryptoPP::byte) * keyIV.iv.size());
    ivHexEncoder.MessageEnd();

    key_strings.iv.resize(ivHexEncoder.MaxRetrievable());
    ivHexEncoder.Get((CryptoPP::byte*)&key_strings.iv[0], key_strings.iv.size());

    return key_strings;
}


std::string encrypt_aes(const std::string& text, encryption_keys& keys)
{
    std::string encrypted_text;

    // Set key and IV to encryption mode
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKeyWithIV(
        reinterpret_cast<const CryptoPP::byte*>(keys.key.data()), 
        keys.key.size(), 
        reinterpret_cast<const CryptoPP::byte*>(keys.iv.data())
    );

    // Encrypt text
    CryptoPP::StringSource s(text, true, new CryptoPP::StreamTransformationFilter(encryption, new CryptoPP::StringSink(encrypted_text)));

    return encrypted_text;
}


std::string encrypt_aes(const std::string& text, unsigned char keyLength)
{
    std::string encrypted_text;

    encryption_keys keys = generate_key_iv(keyLength);

    // Set key and IV to encryption mode
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKeyWithIV(
        reinterpret_cast<const CryptoPP::byte*>(keys.key.data()),
        keys.key.size(),
        reinterpret_cast<const CryptoPP::byte*>(keys.iv.data())
    );

    // Encrypt text
    CryptoPP::StringSource s(text, true, new CryptoPP::StreamTransformationFilter(encryption, new CryptoPP::StringSink(encrypted_text)));

    return encrypted_text;
}

std::string decrypt_aes(const std::string& encrypted_text, const encryption_keys& keys)
{
    std::string text;

    // Set decryption key and IV
    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption;
    decryption.SetKeyWithIV(
        reinterpret_cast<const CryptoPP::byte*>(keys.key.data()),
        keys.key.size(),
        reinterpret_cast<const CryptoPP::byte*>(keys.iv.data())
    );

    // Decrypt
    CryptoPP::StringSource s(encrypted_text, true, new CryptoPP::StreamTransformationFilter(decryption, new CryptoPP::StringSink(text)));

    return text;
}

static bool vmve_decrypt(vmve_file& file_format, std::string& out_raw_data)
{
    // parse the vmve file to figure out which encryption method was used
    if (file_format.header.encrypt_mode == encryption_mode::aes) {
        out_raw_data = decrypt_aes(file_format.data.encrypted_data, file_format.header.keys);

        return true;
    }

    return false;
}

bool vmve_write_to_file(vmve_file& file_format, const std::string& path)
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
    vmve_file file_structure;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    cereal::BinaryInputArchive input(file);
    input(file_structure);

    // decrypt file
    bool vmve_decrypted = vmve_decrypt(file_structure, out_data);

    return vmve_decrypted;
}

