#ifndef GOPOST_CRYPTO_H
#define GOPOST_CRYPTO_H

#include "gopost/types.h"
#include "gopost/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GopostCryptoContext GopostCryptoContext;

GopostError gopost_crypto_create(GopostCryptoContext** ctx);
GopostError gopost_crypto_destroy(GopostCryptoContext* ctx);

GopostError gopost_crypto_aes_gcm_decrypt(
    GopostCryptoContext* ctx,
    const uint8_t* ciphertext, size_t ciphertext_len,
    const uint8_t* key, size_t key_len,
    const uint8_t* nonce, size_t nonce_len,
    const uint8_t* aad, size_t aad_len,
    uint8_t** plaintext, size_t* plaintext_len
);

GopostError gopost_crypto_secure_free(
    GopostCryptoContext* ctx,
    uint8_t* data, size_t data_len
);

/** RSA-OAEP (SHA-256) unwrap: decrypt wrapped_key using private_key_der (PEM or DER). */
GopostError gopost_crypto_rsa_oaep_unwrap(
    GopostCryptoContext* ctx,
    const uint8_t* wrapped_key, size_t wrapped_key_len,
    const uint8_t* private_key_der, size_t private_key_der_len,
    uint8_t** out_key, size_t* out_key_len
);

#ifdef __cplusplus
}
#endif

#endif
