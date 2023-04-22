#include "pch.h"
#include "vmve.h"

encryption_keys generate_key_iv(unsigned int keyLength)
{
    encryption_keys keys{};

    CryptoPP::AutoSeededRandomPool randomPool;

    assert(keyLength == 16 || keyLength == 32);

    keys.key.resize(keyLength);
    keys.iv.resize(CryptoPP::AES::BLOCKSIZE);

    randomPool.GenerateBlock(reinterpret_cast<CryptoPP::byte*>(keys.key.data()), keys.key.size());
    randomPool.GenerateBlock(reinterpret_cast<CryptoPP::byte*>(keys.iv.data()), keys.iv.size());

    return keys;
}

encryption_keys string_to_base16(const encryption_keys& keyIV)
{
    encryption_keys keys{};

    {
        CryptoPP::HexDecoder encoder;
        encoder.Put(reinterpret_cast<const CryptoPP::byte*>(keyIV.key.data()), sizeof(CryptoPP::byte) * keyIV.key.size());
        encoder.MessageEnd();

        keys.key.resize(encoder.MaxRetrievable());
        encoder.Get((CryptoPP::byte*)&keys.key[0], keys.key.size());

    }
    {
        CryptoPP::HexDecoder encoder;
        encoder.Put(reinterpret_cast<const CryptoPP::byte*>(keyIV.iv.data()), sizeof(CryptoPP::byte) * keyIV.iv.size());
        encoder.MessageEnd();

        keys.iv.resize(encoder.MaxRetrievable());
        encoder.Get((CryptoPP::byte*)&keys.iv[0], keys.iv.size());

    }

    return keys;
}

encryption_keys base16_to_string(const encryption_keys& keyIV)
{
    encryption_keys keys{};

    {
        CryptoPP::HexDecoder decoder;
        decoder.Put(reinterpret_cast<const CryptoPP::byte*>(keyIV.key.data()), sizeof(CryptoPP::byte) * keyIV.key.size());
        decoder.MessageEnd();

        keys.key.resize(decoder.MaxRetrievable());
        decoder.Get((CryptoPP::byte*)&keys.key[0], keys.key.size());

    }
    {
        CryptoPP::HexDecoder decoder;
        decoder.Put(reinterpret_cast<const CryptoPP::byte*>(keyIV.iv.data()), sizeof(CryptoPP::byte) * keyIV.iv.size());
        decoder.MessageEnd();

        keys.iv.resize(decoder.MaxRetrievable());
        decoder.Get((CryptoPP::byte*)&keys.iv[0], keys.iv.size());
    }

    return keys;
}


std::string encrypt_aes(const std::string& text, encryption_keys& keys, int key_size)
{
    std::string encrypted_text;

    // Set key and IV to encryption mode
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKeyWithIV(
        reinterpret_cast<const CryptoPP::byte*>(keys.key.data()), 
        key_size, 
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

    // todo: catch exception so that it does not crash at runtime

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
        // todo: !!!
        out_raw_data = decrypt_aes(file_format.data.encrypted_data, {});

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

std::expected<std::string, decrypt_error> vmve_read_from_file(const std::string& path, const encryption_keys& keys)
{
    vmve_file file_structure;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return std::unexpected(decrypt_error::no_file);

    // deserialize vmve file contents into structure
    cereal::BinaryInputArchive input(file);
    input(file_structure);


    std::string content;
    if (file_structure.header.encrypt_mode == encryption_mode::aes) {
        // check if key and iv match file
        const encryption_keys& secret_keys = file_structure.header.encrypted_keys;

        // todo: figure out why using a valid length key/iv but only changing a single number
        // results in an exception.
        std::string key, iv;
        try {
            key = decrypt_aes(secret_keys.key, keys);
            iv = decrypt_aes(secret_keys.iv, keys);
        } catch (const std::exception& e) {
            return std::unexpected(decrypt_error::key_mismatch);
        }

        if (key != keys.key || iv != keys.iv)
            return std::unexpected(decrypt_error::key_mismatch);

        // decrypt file
        content = decrypt_aes(file_structure.data.encrypted_data, keys);
    }
    

    return content;
}

