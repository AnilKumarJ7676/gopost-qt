#include "gopost/crypto.h"
#include "crypto/secure_memory.h"
#include <cstdlib>
#include <cstring>
#include <new>

#if defined(GOPOST_USE_OPENSSL)
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#endif

namespace gopost {
namespace crypto {

#if defined(GOPOST_USE_OPENSSL)
static const size_t kRSAOAEPMaxUnwrappedSize = 256;
#endif

static GopostError rsa_oaep_unwrap_impl(
    const uint8_t* wrapped_key, size_t wrapped_key_len,
    const uint8_t* private_key_der, size_t private_key_der_len,
    uint8_t** out_key, size_t* out_key_len
) {
#if defined(GOPOST_USE_OPENSSL)
    BIO* bio = BIO_new_mem_buf(private_key_der, static_cast<int>(private_key_der_len));
    if (!bio) return GOPOST_ERROR_CRYPTO;

    EVP_PKEY* pkey = nullptr;
    if (private_key_der[0] == '-') {
        pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    } else {
        pkey = d2i_PrivateKey_bio(bio, nullptr);
    }
    BIO_free(bio);
    if (!pkey) return GOPOST_ERROR_CRYPTO;

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) { EVP_PKEY_free(pkey); return GOPOST_ERROR_CRYPTO; }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return GOPOST_ERROR_CRYPTO;
    }
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return GOPOST_ERROR_CRYPTO;
    }
    if (EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return GOPOST_ERROR_CRYPTO;
    }

    size_t out_len = kRSAOAEPMaxUnwrappedSize;
    uint8_t* buf = static_cast<uint8_t*>(malloc(out_len));
    if (!buf) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    int ok = EVP_PKEY_decrypt(ctx, buf, &out_len,
                              wrapped_key, wrapped_key_len);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    if (ok <= 0) {
        free(buf);
        return GOPOST_ERROR_CRYPTO;
    }

    if (!lock_memory(buf, out_len)) {
        secure_zero(buf, out_len);
        free(buf);
        return GOPOST_ERROR_OUT_OF_MEMORY;
    }

    *out_key = buf;
    *out_key_len = out_len;
    return GOPOST_OK;
#else
    (void)wrapped_key;
    (void)wrapped_key_len;
    (void)private_key_der;
    (void)private_key_der_len;
    (void)out_key;
    (void)out_key_len;
    return GOPOST_ERROR_CRYPTO;
#endif
}

}  // namespace crypto
}  // namespace gopost

extern "C" {

GopostError gopost_crypto_rsa_oaep_unwrap(
    GopostCryptoContext* ctx,
    const uint8_t* wrapped_key, size_t wrapped_key_len,
    const uint8_t* private_key_der, size_t private_key_der_len,
    uint8_t** out_key, size_t* out_key_len
) {
    if (!ctx) return GOPOST_ERROR_NOT_INITIALIZED;
    if (!wrapped_key || !private_key_der || !out_key || !out_key_len) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }
    if (private_key_der_len == 0) return GOPOST_ERROR_INVALID_ARGUMENT;
    return gopost::crypto::rsa_oaep_unwrap_impl(
        wrapped_key, wrapped_key_len,
        private_key_der, private_key_der_len,
        out_key, out_key_len
    );
}

}
