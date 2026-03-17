#include "gopost/template_parser.h"
#include "gopost/crypto.h"
#include "gopost/engine.h"

#include <cstring>
#include <cstdlib>
#include <new>
#include <string>
#include <vector>

#if defined(__unix__)
#include <sys/mman.h>
#elif defined(_WIN32) && !defined(GOPOST_HAS_SHA256)
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#define GOPOST_HAS_SHA256 1
#elif defined(GOPOST_USE_OPENSSL)
#include <openssl/sha.h>
#define GOPOST_HAS_SHA256 1
#elif defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#define GOPOST_HAS_SHA256 1
#endif

struct GopostSection {
    GopostSectionType type;
    uint32_t length;
    uint8_t* decrypted_data;
};

struct GopostTemplate {
    GopostTemplateType template_type;
    uint16_t version;
    uint16_t flags;

    uint8_t algorithm_id;
    uint8_t iv[12];
    uint8_t auth_tag[16];
    uint8_t key_id[16];

    std::vector<GopostSection> sections;
    std::string metadata_json;
    uint32_t layer_count;

    uint8_t file_checksum[32];
    bool checksum_valid;
};

static GopostError validate_magic(const uint8_t* data, size_t len) {
    if (len < GOPOST_FILE_HEADER_SIZE) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (memcmp(data, GOPOST_MAGIC_BYTES, GOPOST_MAGIC_SIZE) != 0) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    return GOPOST_OK;
}

static uint16_t read_u16(const uint8_t* ptr) {
    return static_cast<uint16_t>(ptr[0]) |
           (static_cast<uint16_t>(ptr[1]) << 8);
}

static uint32_t read_u32(const uint8_t* ptr) {
    return static_cast<uint32_t>(ptr[0]) |
           (static_cast<uint32_t>(ptr[1]) << 8) |
           (static_cast<uint32_t>(ptr[2]) << 16) |
           (static_cast<uint32_t>(ptr[3]) << 24);
}

static void sha256_hash(const uint8_t* data, size_t len, uint8_t out[32]) {
#if defined(__APPLE__)
    CC_SHA256(data, static_cast<CC_LONG>(len), out);
#elif defined(GOPOST_USE_OPENSSL)
    SHA256(data, len, out);
#elif defined(_WIN32)
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0);
    BCryptHashData(hHash, const_cast<PUCHAR>(data), static_cast<ULONG>(len), 0);
    BCryptFinishHash(hHash, out, 32, 0);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
#else
    #error "No SHA-256 backend available. Define GOPOST_USE_OPENSSL or build on Apple/Windows."
#endif
}

static void* secure_alloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) return nullptr;
#if defined(__unix__)
    mlock(ptr, size);
#elif defined(_WIN32)
    VirtualLock(ptr, size);
#endif
    return ptr;
}

static void secure_free(void* ptr, size_t size) {
    if (!ptr) return;
    memset(ptr, 0, size);
#if defined(__unix__)
    munlock(ptr, size);
#elif defined(_WIN32)
    VirtualUnlock(ptr, size);
#endif
    free(ptr);
}

static GopostError parse_encryption_header(
    const uint8_t* data, size_t offset, GopostTemplate* tpl
) {
    const uint8_t* hdr = data + offset;

    tpl->algorithm_id = hdr[0];
    if (tpl->algorithm_id != 1) {
        return GOPOST_ERROR_CRYPTO;
    }

    memcpy(tpl->iv, hdr + 1, 12);
    memcpy(tpl->auth_tag, hdr + 13, 16);
    memcpy(tpl->key_id, hdr + 29, 16);

    return GOPOST_OK;
}

static GopostError parse_sections(
    GopostCryptoContext* crypto,
    const uint8_t* data, size_t data_len, size_t offset,
    const uint8_t* key, size_t key_len,
    const uint8_t* iv, size_t iv_len,
    GopostTemplate* tpl
) {
    size_t pos = offset;
    int section_idx = 0;

    while (pos + 4 <= data_len - 32 && section_idx <= GOPOST_SECTION_BLOB) {
        uint32_t section_len = read_u32(data + pos);
        pos += 4;

        if (pos + section_len > data_len - 32) {
            return GOPOST_ERROR_IO;
        }

        uint8_t* plaintext = nullptr;
        size_t plaintext_len = 0;

        GopostError err = gopost_crypto_aes_gcm_decrypt(
            crypto,
            data + pos, section_len,
            key, key_len,
            iv, iv_len,
            nullptr, 0,
            &plaintext, &plaintext_len
        );

        if (err != GOPOST_OK) {
            return err;
        }

        GopostSection section;
        section.type = static_cast<GopostSectionType>(section_idx);
        section.length = static_cast<uint32_t>(plaintext_len);
        section.decrypted_data = plaintext;

        tpl->sections.push_back(section);

        if (section_idx == GOPOST_SECTION_METADATA && plaintext_len > 0) {
            tpl->metadata_json.assign(
                reinterpret_cast<const char*>(plaintext), plaintext_len);
        }

        pos += section_len;
        section_idx++;
    }

    return GOPOST_OK;
}

extern "C" {

GopostError gopost_template_parse(
    GopostEngine* engine,
    const uint8_t* data, size_t data_len,
    const uint8_t* session_key, size_t key_len,
    GopostTemplate** out_template
) {
    if (!engine || !data || !session_key || !out_template) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (data_len < GOPOST_FILE_HEADER_SIZE + GOPOST_ENCRYPTION_HEADER_SIZE + 32) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    GopostError err = validate_magic(data, data_len);
    if (err != GOPOST_OK) return err;

    auto* tpl = new (std::nothrow) GopostTemplate{};
    if (!tpl) return GOPOST_ERROR_OUT_OF_MEMORY;

    tpl->version = read_u16(data + 4);
    tpl->flags = read_u16(data + 6);
    tpl->template_type = static_cast<GopostTemplateType>(data[8]);

    err = parse_encryption_header(data, GOPOST_FILE_HEADER_SIZE, tpl);
    if (err != GOPOST_OK) {
        delete tpl;
        return err;
    }

    // Verify file checksum (SHA-256 of everything except last 32 bytes)
    uint8_t computed_checksum[32];
    sha256_hash(data, data_len - 32, computed_checksum);
    memcpy(tpl->file_checksum, data + data_len - 32, 32);
    tpl->checksum_valid = (memcmp(computed_checksum, tpl->file_checksum, 32) == 0);

    GopostCryptoContext* crypto = nullptr;
    err = gopost_crypto_create(&crypto);
    if (err != GOPOST_OK) {
        delete tpl;
        return err;
    }

    size_t sections_offset = GOPOST_FILE_HEADER_SIZE + GOPOST_ENCRYPTION_HEADER_SIZE;
    err = parse_sections(
        crypto,
        data, data_len, sections_offset,
        session_key, key_len,
        tpl->iv, 12,
        tpl
    );

    if (err != GOPOST_OK) {
        for (auto& s : tpl->sections) {
            if (s.decrypted_data) {
                gopost_crypto_secure_free(crypto, s.decrypted_data, s.length);
                s.decrypted_data = nullptr;
            }
        }
        gopost_crypto_destroy(crypto);
        delete tpl;
        return err;
    }

    gopost_crypto_destroy(crypto);

    tpl->layer_count = 0;
    if (tpl->sections.size() > GOPOST_SECTION_LAYERS) {
        const auto& layer_section = tpl->sections[GOPOST_SECTION_LAYERS];
        if (layer_section.decrypted_data && layer_section.length >= 4) {
            tpl->layer_count = read_u32(layer_section.decrypted_data);
        }
    }

    *out_template = tpl;
    return GOPOST_OK;
}

GopostError gopost_template_parse_with_wrapped_key(
    GopostEngine* engine,
    const uint8_t* data, size_t data_len,
    const uint8_t* wrapped_session_key, size_t wrapped_key_len,
    const uint8_t* private_key_der, size_t private_key_der_len,
    GopostTemplate** out_template
) {
    if (!engine || !data || !wrapped_session_key || !private_key_der || !out_template) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    GopostCryptoContext* crypto = nullptr;
    GopostError err = gopost_crypto_create(&crypto);
    if (err != GOPOST_OK) return err;

    uint8_t* session_key = nullptr;
    size_t session_key_len = 0;
    err = gopost_crypto_rsa_oaep_unwrap(
        crypto,
        wrapped_session_key, wrapped_key_len,
        private_key_der, private_key_der_len,
        &session_key, &session_key_len
    );
    if (err != GOPOST_OK) {
        gopost_crypto_destroy(crypto);
        return err;
    }

    err = gopost_template_parse(engine, data, data_len, session_key, session_key_len, out_template);

    gopost_crypto_secure_free(crypto, session_key, session_key_len);
    gopost_crypto_destroy(crypto);

    return err;
}

GopostError gopost_template_get_metadata(
    GopostTemplate* tpl,
    const char** out_json
) {
    if (!tpl || !out_json) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_json = tpl->metadata_json.c_str();
    return GOPOST_OK;
}

GopostError gopost_template_get_layer_count(
    GopostTemplate* tpl,
    uint32_t* out_count
) {
    if (!tpl || !out_count) return GOPOST_ERROR_INVALID_ARGUMENT;
    *out_count = tpl->layer_count;
    return GOPOST_OK;
}

GopostError gopost_template_unload(
    GopostEngine* engine,
    GopostTemplate* tpl
) {
    (void)engine;
    if (!tpl) return GOPOST_ERROR_INVALID_ARGUMENT;

    GopostCryptoContext* crypto = nullptr;
    if (gopost_crypto_create(&crypto) == GOPOST_OK) {
        for (auto& section : tpl->sections) {
            if (section.decrypted_data) {
                gopost_crypto_secure_free(crypto, section.decrypted_data, section.length);
                section.decrypted_data = nullptr;
            }
        }
        gopost_crypto_destroy(crypto);
    } else {
        for (auto& section : tpl->sections) {
            if (section.decrypted_data) {
                secure_free(section.decrypted_data, section.length);
                section.decrypted_data = nullptr;
            }
        }
    }
    tpl->sections.clear();
    tpl->metadata_json.clear();

    delete tpl;
    return GOPOST_OK;
}

} // extern "C"
