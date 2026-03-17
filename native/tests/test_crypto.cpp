#include <gtest/gtest.h>
#include "gopost/crypto.h"
#include <cstring>

TEST(CryptoTest, CreateAndDestroy) {
    GopostCryptoContext* ctx = nullptr;
    ASSERT_EQ(gopost_crypto_create(&ctx), GOPOST_OK);
    ASSERT_NE(ctx, nullptr);
    ASSERT_EQ(gopost_crypto_destroy(ctx), GOPOST_OK);
}

TEST(CryptoTest, NullArguments) {
    EXPECT_EQ(gopost_crypto_create(nullptr), GOPOST_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(gopost_crypto_destroy(nullptr), GOPOST_ERROR_INVALID_ARGUMENT);
}

TEST(CryptoTest, DecryptInvalidKeySize) {
    GopostCryptoContext* ctx = nullptr;
    gopost_crypto_create(&ctx);

    uint8_t ciphertext[32] = {};
    uint8_t key[16] = {};  // Wrong size (should be 32)
    uint8_t nonce[12] = {};
    uint8_t* plaintext = nullptr;
    size_t plaintext_len = 0;

    GopostError err = gopost_crypto_aes_gcm_decrypt(
        ctx, ciphertext, 32, key, 16, nonce, 12,
        nullptr, 0, &plaintext, &plaintext_len
    );
    EXPECT_EQ(err, GOPOST_ERROR_INVALID_ARGUMENT);

    gopost_crypto_destroy(ctx);
}

/* NIST SP 800-38D Test Case 14: AES-256-GCM, 96-bit IV, 128-bit tag.
 * Key = 32 zero bytes, IV = 12 zero bytes, PT = 16 zero bytes, AAD = none.
 * CT = cea7403d4d606b6e074ec5d3baf39d18, Tag = d0d1c8a799996bf0265b98b5d48ab919
 */
static const uint8_t kAesGcmKey[32] = {0};
static const uint8_t kAesGcmNonce[12] = {0};
static const uint8_t kAesGcmCiphertextAndTag[32] = {
    0xce, 0xa7, 0x40, 0x3d, 0x4d, 0x60, 0x6b, 0x6e,
    0x07, 0x4e, 0xc5, 0xd3, 0xba, 0xf3, 0x9d, 0x18,
    0xd0, 0xd1, 0xc8, 0xa7, 0x99, 0x99, 0x6b, 0xf0,
    0x26, 0x5b, 0x98, 0xb5, 0xd4, 0x8a, 0xb9, 0x19,
};
static const uint8_t kAesGcmExpectedPlaintext[16] = {0};

TEST(CryptoTest, AesGcmKnownAnswer) {
    GopostCryptoContext* ctx = nullptr;
    ASSERT_EQ(gopost_crypto_create(&ctx), GOPOST_OK);

    uint8_t* plaintext = nullptr;
    size_t plaintext_len = 0;

    GopostError err = gopost_crypto_aes_gcm_decrypt(
        ctx,
        kAesGcmCiphertextAndTag, sizeof(kAesGcmCiphertextAndTag),
        kAesGcmKey, sizeof(kAesGcmKey),
        kAesGcmNonce, sizeof(kAesGcmNonce),
        nullptr, 0,
        &plaintext, &plaintext_len
    );

#if defined(GOPOST_USE_OPENSSL)
    ASSERT_EQ(err, GOPOST_OK) << "AES-GCM decrypt should succeed with OpenSSL";
    ASSERT_NE(plaintext, nullptr);
    ASSERT_EQ(plaintext_len, 16u);
    EXPECT_EQ(0, memcmp(plaintext, kAesGcmExpectedPlaintext, 16));
    gopost_crypto_secure_free(ctx, plaintext, plaintext_len);
#else
    EXPECT_EQ(err, GOPOST_ERROR_CRYPTO) << "No crypto backend; expect CRYPTO";
#endif

    gopost_crypto_destroy(ctx);
}

TEST(CryptoTest, DecryptInvalidTagFailsWithCrypto) {
    GopostCryptoContext* ctx = nullptr;
    ASSERT_EQ(gopost_crypto_create(&ctx), GOPOST_OK);

    uint8_t ciphertext_and_tag[32];
    memcpy(ciphertext_and_tag, kAesGcmCiphertextAndTag, 32);
    ciphertext_and_tag[31] ^= 0x01;  /* Tamper with last tag byte */

    uint8_t key[32];
    memcpy(key, kAesGcmKey, 32);
    uint8_t nonce[12];
    memcpy(nonce, kAesGcmNonce, 12);

    uint8_t* plaintext = nullptr;
    size_t plaintext_len = 0;

    GopostError err = gopost_crypto_aes_gcm_decrypt(
        ctx, ciphertext_and_tag, 32, key, 32, nonce, 12,
        nullptr, 0, &plaintext, &plaintext_len
    );

#if defined(GOPOST_USE_OPENSSL)
    EXPECT_EQ(err, GOPOST_ERROR_CRYPTO) << "Tampered tag must fail verification";
    EXPECT_EQ(plaintext, nullptr);
#else
    EXPECT_TRUE(err == GOPOST_ERROR_CRYPTO || err == GOPOST_OK) << "No backend may return CRYPTO or stub";
    if (plaintext) gopost_crypto_secure_free(ctx, plaintext, plaintext_len);
#endif

    gopost_crypto_destroy(ctx);
}

TEST(CryptoTest, DecryptInvalidKeyFailsWithCrypto) {
    GopostCryptoContext* ctx = nullptr;
    ASSERT_EQ(gopost_crypto_create(&ctx), GOPOST_OK);

    uint8_t key[32];
    memcpy(key, kAesGcmKey, 32);
    key[0] ^= 0x01;  /* Wrong key */

    uint8_t* plaintext = nullptr;
    size_t plaintext_len = 0;

    GopostError err = gopost_crypto_aes_gcm_decrypt(
        ctx, kAesGcmCiphertextAndTag, 32, key, 32, kAesGcmNonce, 12,
        nullptr, 0, &plaintext, &plaintext_len
    );

#if defined(GOPOST_USE_OPENSSL)
    EXPECT_EQ(err, GOPOST_ERROR_CRYPTO) << "Wrong key must fail verification";
    EXPECT_EQ(plaintext, nullptr);
#else
    if (plaintext) gopost_crypto_secure_free(ctx, plaintext, plaintext_len);
#endif

    gopost_crypto_destroy(ctx);
}

TEST(CryptoTest, DecryptAndSecureFree) {
    GopostCryptoContext* ctx = nullptr;
    gopost_crypto_create(&ctx);

#if defined(GOPOST_USE_OPENSSL)
    uint8_t* plaintext = nullptr;
    size_t plaintext_len = 0;
    GopostError err = gopost_crypto_aes_gcm_decrypt(
        ctx, kAesGcmCiphertextAndTag, sizeof(kAesGcmCiphertextAndTag),
        kAesGcmKey, sizeof(kAesGcmKey), kAesGcmNonce, sizeof(kAesGcmNonce),
        nullptr, 0, &plaintext, &plaintext_len
    );
    ASSERT_EQ(err, GOPOST_OK);
    ASSERT_NE(plaintext, nullptr);
    ASSERT_EQ(plaintext_len, 16u);
#else
    uint8_t ciphertext[32];
    memset(ciphertext, 0xAB, 32);
    uint8_t key[32] = {};
    uint8_t nonce[12] = {};
    uint8_t* plaintext = nullptr;
    size_t plaintext_len = 0;
    GopostError err = gopost_crypto_aes_gcm_decrypt(
        ctx, ciphertext, 32, key, 32, nonce, 12,
        nullptr, 0, &plaintext, &plaintext_len
    );
    if (err != GOPOST_OK) {
        gopost_crypto_destroy(ctx);
        return;
    }
#endif

    err = gopost_crypto_secure_free(ctx, plaintext, plaintext_len);
    EXPECT_EQ(err, GOPOST_OK);

    /* secure_free with same ptr again is undefined (already freed); test null/size 0 is no-op */
    err = gopost_crypto_secure_free(ctx, nullptr, 0);
    EXPECT_EQ(err, GOPOST_OK);

    gopost_crypto_destroy(ctx);
}

TEST(CryptoTest, SecureFreeNullNoOp) {
    GopostCryptoContext* ctx = nullptr;
    gopost_crypto_create(&ctx);
    EXPECT_EQ(gopost_crypto_secure_free(ctx, nullptr, 0), GOPOST_OK);
    EXPECT_EQ(gopost_crypto_secure_free(ctx, nullptr, 100), GOPOST_OK);
    gopost_crypto_destroy(ctx);
}

#if defined(GOPOST_USE_OPENSSL)
TEST(CryptoTest, RsaOaepUnwrapNullFails) {
    GopostCryptoContext* ctx = nullptr;
    gopost_crypto_create(&ctx);
    uint8_t* out = nullptr;
    size_t out_len = 0;
    uint8_t dummy[1] = {0};
    EXPECT_EQ(gopost_crypto_rsa_oaep_unwrap(ctx, nullptr, 0, dummy, 1, &out, &out_len), GOPOST_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(gopost_crypto_rsa_oaep_unwrap(ctx, dummy, 1, nullptr, 0, &out, &out_len), GOPOST_ERROR_INVALID_ARGUMENT);
    gopost_crypto_destroy(ctx);
}

TEST(CryptoTest, RsaOaepUnwrapWithoutCtxFails) {
    uint8_t wrapped[1] = {0};
    uint8_t key[1] = {0};
    uint8_t* out = nullptr;
    size_t out_len = 0;
    EXPECT_EQ(gopost_crypto_rsa_oaep_unwrap(nullptr, wrapped, 1, key, 1, &out, &out_len), GOPOST_ERROR_NOT_INITIALIZED);
}
#endif
