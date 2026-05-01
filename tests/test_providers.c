#include "hesia/core.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_aead(void) {
    uint8_t key[32];
    uint8_t nonce[12];
    const uint8_t aad[] = {0x48, 0x45, 0x53, 0x49, 0x41};
    const uint8_t plaintext[] = "hesia-core-provider-check";
    uint8_t ciphertext[sizeof(plaintext)];
    uint8_t decrypted[sizeof(plaintext)];
    uint8_t tag[16];
    size_t ciphertext_len = 0;
    size_t decrypted_len = 0;
    hesia_error_t error;

    assert(hesia_feature_available(HESIA_FEATURE_OPENSSL_AEAD) == 1);
    assert(hesia_random(key, sizeof(key), &error) == HESIA_STATUS_OK);
    assert(hesia_random(nonce, sizeof(nonce), &error) == HESIA_STATUS_OK);
    assert(hesia_aead_encrypt(
        HESIA_AEAD_AES_256_GCM,
        key,
        sizeof(key),
        nonce,
        sizeof(nonce),
        aad,
        sizeof(aad),
        plaintext,
        sizeof(plaintext),
        ciphertext,
        sizeof(ciphertext),
        &ciphertext_len,
        tag,
        sizeof(tag),
        &error) == HESIA_STATUS_OK);
    assert(ciphertext_len == sizeof(plaintext));
    assert(hesia_aead_decrypt(
        HESIA_AEAD_AES_256_GCM,
        key,
        sizeof(key),
        nonce,
        sizeof(nonce),
        aad,
        sizeof(aad),
        ciphertext,
        ciphertext_len,
        tag,
        sizeof(tag),
        decrypted,
        sizeof(decrypted),
        &decrypted_len,
        &error) == HESIA_STATUS_OK);
    assert(decrypted_len == sizeof(plaintext));
    assert(memcmp(plaintext, decrypted, sizeof(plaintext)) == 0);
    hesia_secure_zero(key, sizeof(key));
}

static void test_ml_kem(void) {
    size_t public_len = 0;
    size_t secret_len = 0;
    size_t ciphertext_len = 0;
    size_t shared_len = 0;
    uint8_t public_key[4096];
    uint8_t secret_key[8192];
    uint8_t ciphertext[4096];
    uint8_t shared_a[128];
    uint8_t shared_b[128];
    hesia_error_t error;

    assert(hesia_feature_available(HESIA_FEATURE_LIBOQS_PQC) == 1);
    assert(hesia_pqc_public_key_bytes(HESIA_PQC_ML_KEM_1024, &public_len, &error) == HESIA_STATUS_OK);
    assert(hesia_pqc_secret_key_bytes(HESIA_PQC_ML_KEM_1024, &secret_len, &error) == HESIA_STATUS_OK);
    assert(hesia_pqc_ciphertext_bytes(HESIA_PQC_ML_KEM_1024, &ciphertext_len, &error) == HESIA_STATUS_OK);
    assert(hesia_pqc_shared_secret_bytes(HESIA_PQC_ML_KEM_1024, &shared_len, &error) == HESIA_STATUS_OK);
    assert(public_len <= sizeof(public_key));
    assert(secret_len <= sizeof(secret_key));
    assert(ciphertext_len <= sizeof(ciphertext));
    assert(shared_len <= sizeof(shared_a));
    assert(shared_len <= sizeof(shared_b));

    assert(hesia_pqc_keypair(
        HESIA_PQC_ML_KEM_1024,
        public_key,
        public_len,
        secret_key,
        secret_len,
        &error) == HESIA_STATUS_OK);
    assert(hesia_pqc_encapsulate(
        HESIA_PQC_ML_KEM_1024,
        public_key,
        public_len,
        ciphertext,
        ciphertext_len,
        shared_a,
        shared_len,
        &error) == HESIA_STATUS_OK);
    assert(hesia_pqc_decapsulate(
        HESIA_PQC_ML_KEM_1024,
        secret_key,
        secret_len,
        ciphertext,
        ciphertext_len,
        shared_b,
        shared_len,
        &error) == HESIA_STATUS_OK);
    assert(memcmp(shared_a, shared_b, shared_len) == 0);
    hesia_secure_zero(secret_key, secret_len);
    hesia_secure_zero(shared_a, shared_len);
    hesia_secure_zero(shared_b, shared_len);
}

static void test_ml_dsa(void) {
    const uint8_t message[] = "hesia-core-ml-dsa-provider-check";
    size_t public_len = 0;
    size_t secret_len = 0;
    size_t signature_capacity = 0;
    size_t signature_len = 0;
    uint8_t public_key[4096];
    uint8_t secret_key[8192];
    uint8_t signature[8192];
    hesia_error_t error;

    assert(hesia_pqc_public_key_bytes(HESIA_PQC_ML_DSA_87, &public_len, &error) == HESIA_STATUS_OK);
    assert(hesia_pqc_secret_key_bytes(HESIA_PQC_ML_DSA_87, &secret_len, &error) == HESIA_STATUS_OK);
    assert(hesia_pqc_signature_bytes(HESIA_PQC_ML_DSA_87, &signature_capacity, &error) == HESIA_STATUS_OK);
    assert(public_len <= sizeof(public_key));
    assert(secret_len <= sizeof(secret_key));
    assert(signature_capacity <= sizeof(signature));

    assert(hesia_pqc_keypair(
        HESIA_PQC_ML_DSA_87,
        public_key,
        public_len,
        secret_key,
        secret_len,
        &error) == HESIA_STATUS_OK);
    assert(hesia_pqc_sign(
        HESIA_PQC_ML_DSA_87,
        secret_key,
        secret_len,
        message,
        sizeof(message),
        signature,
        signature_capacity,
        &signature_len,
        &error) == HESIA_STATUS_OK);
    assert(hesia_pqc_verify(
        HESIA_PQC_ML_DSA_87,
        public_key,
        public_len,
        message,
        sizeof(message),
        signature,
        signature_len,
        &error) == HESIA_STATUS_OK);
    hesia_secure_zero(secret_key, secret_len);
}

int main(void) {
    printf("HESIA Core %s\n", hesia_version());
    printf("OpenSSL provider: %s\n", hesia_feature_provider_version(HESIA_FEATURE_OPENSSL_AEAD));
    printf("liboqs provider: %s\n", hesia_feature_provider_version(HESIA_FEATURE_LIBOQS_PQC));
    test_aead();
    test_ml_kem();
    test_ml_dsa();
    return 0;
}
