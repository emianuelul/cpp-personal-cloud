#ifndef CPP_PERSONAL_CLOUD_ENCRYPTION_MANAGER_H
#define CPP_PERSONAL_CLOUD_ENCRYPTION_MANAGER_H

#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "aes.h"
}

class EncryptionManager {
private:
    static void prepare_iv(uint8_t *iv) {
        uint8_t default_iv[16] = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
        };
        std::memcpy(iv, default_iv, 16);
    }

public:
    static void encrypt(uint8_t *data, size_t length, const std::string &key_hash_hex) {
        if (key_hash_hex.length() != 64) {
            return;
        }

        if (key_hash_hex.length() != 64) return;

        uint8_t key[32];
        for (unsigned int i = 0; i < 32; i++) {
            std::string byteString = key_hash_hex.substr(i * 2, 2);
            key[i] = (uint8_t) std::strtol(byteString.c_str(), nullptr, 16);
        }

        uint8_t iv[16];
        uint8_t default_iv[16] = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
        };
        std::memcpy(iv, default_iv, 16);

        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, key, iv);

        std::cout << "Encrypting file...\n";

        AES_CTR_xcrypt_buffer(&ctx, data, (uint32_t) length);
    }

    static void decrypt(uint8_t *data, size_t length, const std::string &key_hash_hex) {
        std::cout << "Decrypting file...\n";

        encrypt(data, length, key_hash_hex);
    }
};

#endif
