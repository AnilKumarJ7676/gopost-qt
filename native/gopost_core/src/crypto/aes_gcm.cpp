#include "gopost/crypto.h"
#include "crypto/secure_memory.h"
#include <cstdlib>
#include <cstring>
#include <new>

#if defined(GOPOST_USE_OPENSSL)
#include <openssl/evp.h>
#include <openssl/err.h>
#endif

/* AES-GCM: OpenSSL on all platforms when available. Apple CommonCrypto has no public GCM API. */

namespace gopost {
namespace crypto {

static const size_t kAESKeySize = 32;
static const size_t kGCMNonceSize = 12;
static const size_t kGCMTagSize = 16;

class AesGcmDecryptor {
public:
    GopostError decrypt(
        const uint8_t* ciphertext, size_t ciphertext_len,
        const uint8_t* key, size_t key_len,
        const uint8_t* nonce, size_t nonce_len,
        const uint8_t* aad, size_t aad_len,
        uint8_t** out_plaintext, size_t* out_len
    ) {
        if (!ciphertext || !key || !nonce || !out_plaintext || !out_len) {
            return GOPOST_ERROR_INVALID_ARGUMENT;
        }
        if (key_len != kAESKeySize) {
            return GOPOST_ERROR_INVALID_ARGUMENT;
        }
        if (nonce_len != kGCMNonceSize) {
            return GOPOST_ERROR_INVALID_ARGUMENT;
        }
        if (ciphertext_len < kGCMTagSize) {
            return GOPOST_ERROR_INVALID_ARGUMENT;
        }

        const size_t payload_len = ciphertext_len - kGCMTagSize;
        const uint8_t* ct = ciphertext;
        const uint8_t* tag = ciphertext + payload_len;

        uint8_t* plaintext = static_cast<uint8_t*>(malloc(payload_len));
        if (!plaintext) {
            return GOPOST_ERROR_OUT_OF_MEMORY;
        }

        if (!lock_memory(plaintext, payload_len)) {
            free(plaintext);
            return GOPOST_ERROR_OUT_OF_MEMORY;
        }

        GopostError err = decrypt_impl(ct, payload_len, tag, key, nonce, aad, aad_len, plaintext, out_len);
        if (err != GOPOST_OK) {
            secure_zero(plaintext, payload_len);
            unlock_memory(plaintext, payload_len);
            free(plaintext);
            return err;
        }

        *out_plaintext = plaintext;
        return GOPOST_OK;
    }

    void secure_free(uint8_t* data, size_t len) {
        if (!data || len == 0) return;
        secure_zero(data, len);
        unlock_memory(data, len);
        free(data);
    }

private:
    GopostError decrypt_impl(
        const uint8_t* ciphertext, size_t ciphertext_len,
        const uint8_t* tag,
        const uint8_t* key, const uint8_t* nonce,
        const uint8_t* aad, size_t aad_len,
        uint8_t* plaintext, size_t* out_len
    ) {
#if defined(GOPOST_USE_OPENSSL)
        return decrypt_openssl(ciphertext, ciphertext_len, tag, key, nonce, aad, aad_len, plaintext, out_len);
#else
        (void)ciphertext;
        (void)ciphertext_len;
        (void)tag;
        (void)key;
        (void)nonce;
        (void)aad;
        (void)aad_len;
        (void)plaintext;
        (void)out_len;
        return GOPOST_ERROR_CRYPTO;
#endif
    }

#if defined(GOPOST_USE_OPENSSL)
    GopostError decrypt_openssl(
        const uint8_t* ciphertext, size_t ciphertext_len,
        const uint8_t* tag,
        const uint8_t* key, const uint8_t* nonce,
        const uint8_t* aad, size_t aad_len,
        uint8_t* plaintext, size_t* out_len
    ) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return GOPOST_ERROR_CRYPTO;

        int ok = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        if (ok != 1) { EVP_CIPHER_CTX_free(ctx); return GOPOST_ERROR_CRYPTO; }

        ok = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(kGCMNonceSize), nullptr);
        if (ok != 1) { EVP_CIPHER_CTX_free(ctx); return GOPOST_ERROR_CRYPTO; }

        ok = EVP_DecryptInit_ex(ctx, nullptr, nullptr, key, nonce);
        if (ok != 1) { EVP_CIPHER_CTX_free(ctx); return GOPOST_ERROR_CRYPTO; }

        int len = 0;
        if (aad_len > 0 && aad) {
            ok = EVP_DecryptUpdate(ctx, nullptr, &len, aad, static_cast<int>(aad_len));
            if (ok != 1) { EVP_CIPHER_CTX_free(ctx); return GOPOST_ERROR_CRYPTO; }
        }

        ok = EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, static_cast<int>(ciphertext_len));
        if (ok != 1) { EVP_CIPHER_CTX_free(ctx); return GOPOST_ERROR_CRYPTO; }
        *out_len = static_cast<size_t>(len);

        ok = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(kGCMTagSize), const_cast<uint8_t*>(tag));
        if (ok != 1) { EVP_CIPHER_CTX_free(ctx); return GOPOST_ERROR_CRYPTO; }

        ok = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
        EVP_CIPHER_CTX_free(ctx);
        if (ok != 1) return GOPOST_ERROR_CRYPTO;
        *out_len += static_cast<size_t>(len);
        return GOPOST_OK;
    }
#endif

};

}  // namespace crypto
}  // namespace gopost

struct GopostCryptoContext {
    gopost::crypto::AesGcmDecryptor decryptor;
};

extern "C" {

GopostError gopost_crypto_create(GopostCryptoContext** ctx) {
    if (!ctx) return GOPOST_ERROR_INVALID_ARGUMENT;
    auto* c = new (std::nothrow) GopostCryptoContext{};
    if (!c) return GOPOST_ERROR_OUT_OF_MEMORY;
    *ctx = c;
    return GOPOST_OK;
}

GopostError gopost_crypto_destroy(GopostCryptoContext* ctx) {
    if (!ctx) return GOPOST_ERROR_INVALID_ARGUMENT;
    delete ctx;
    return GOPOST_OK;
}

GopostError gopost_crypto_aes_gcm_decrypt(
    GopostCryptoContext* ctx,
    const uint8_t* ciphertext, size_t ciphertext_len,
    const uint8_t* key, size_t key_len,
    const uint8_t* nonce, size_t nonce_len,
    const uint8_t* aad, size_t aad_len,
    uint8_t** plaintext, size_t* plaintext_len
) {
    if (!ctx) return GOPOST_ERROR_NOT_INITIALIZED;
    return ctx->decryptor.decrypt(
        ciphertext, ciphertext_len,
        key, key_len,
        nonce, nonce_len,
        aad, aad_len,
        plaintext, plaintext_len
    );
}

GopostError gopost_crypto_secure_free(
    GopostCryptoContext* ctx,
    uint8_t* data, size_t data_len
) {
    if (!ctx) return GOPOST_ERROR_NOT_INITIALIZED;
    ctx->decryptor.secure_free(data, data_len);
    return GOPOST_OK;
}

}
