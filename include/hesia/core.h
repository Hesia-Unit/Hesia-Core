#ifndef HESIA_CORE_H
#define HESIA_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(HESIA_CORE_BUILD_SHARED)
#  if defined(HESIA_CORE_EXPORTS)
#    define HESIA_CORE_API __declspec(dllexport)
#  else
#    define HESIA_CORE_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define HESIA_CORE_API __attribute__((visibility("default")))
#else
#  define HESIA_CORE_API
#endif

#define HESIA_CORE_VERSION_MAJOR 0u
#define HESIA_CORE_VERSION_MINOR 1u
#define HESIA_CORE_VERSION_PATCH 0u

#define HESIA_ERROR_SUBSYSTEM_BYTES 32u
#define HESIA_ERROR_MESSAGE_BYTES 192u

typedef enum hesia_status {
    HESIA_STATUS_OK = 0,
    HESIA_STATUS_INVALID_ARGUMENT = 1,
    HESIA_STATUS_BUFFER_TOO_SMALL = 2,
    HESIA_STATUS_UNSUPPORTED = 3,
    HESIA_STATUS_CRYPTO_ERROR = 4,
    HESIA_STATUS_AUTHENTICATION_FAILED = 5,
    HESIA_STATUS_POLICY_DENIED = 6,
    HESIA_STATUS_WATCHDOG_EXPIRED = 7,
    HESIA_STATUS_INFERENCE_REJECTED = 8,
    HESIA_STATUS_IO_ERROR = 9,
    HESIA_STATUS_INTERNAL_ERROR = 10
} hesia_status_t;

typedef enum hesia_feature {
    HESIA_FEATURE_OS_RANDOM = 1,
    HESIA_FEATURE_OPENSSL_AEAD = 2,
    HESIA_FEATURE_LIBOQS_PQC = 3,
    HESIA_FEATURE_WATCHDOG = 4,
    HESIA_FEATURE_INFERENCE_GUARD = 5
} hesia_feature_t;

typedef enum hesia_aead_algorithm {
    HESIA_AEAD_AES_256_GCM = 1
} hesia_aead_algorithm_t;

typedef enum hesia_pqc_algorithm {
    HESIA_PQC_ML_KEM_768 = 1,
    HESIA_PQC_ML_KEM_1024 = 2,
    HESIA_PQC_ML_DSA_65 = 3,
    HESIA_PQC_ML_DSA_87 = 4
} hesia_pqc_algorithm_t;

typedef enum hesia_inference_state {
    HESIA_INFERENCE_IDLE = 0,
    HESIA_INFERENCE_LOADED = 1,
    HESIA_INFERENCE_RUNNING = 2,
    HESIA_INFERENCE_COMPLETE = 3,
    HESIA_INFERENCE_FAILED = 4
} hesia_inference_state_t;

typedef struct hesia_error {
    hesia_status_t status;
    char subsystem[HESIA_ERROR_SUBSYSTEM_BYTES];
    char message[HESIA_ERROR_MESSAGE_BYTES];
} hesia_error_t;

typedef struct hesia_watchdog {
    uint64_t timeout_ms;
    uint64_t last_heartbeat_ms;
    uint32_t max_missed;
    uint32_t missed;
    int armed;
} hesia_watchdog_t;

typedef struct hesia_model_contract {
    const char *model_id;
    uint32_t model_version;
    size_t max_input_bytes;
    size_t max_output_bytes;
    uint32_t max_latency_ms;
    int require_signed_model;
    const uint8_t *model_digest;
    size_t model_digest_len;
} hesia_model_contract_t;

typedef struct hesia_inference_request {
    const uint8_t *input;
    size_t input_len;
    uint8_t *output;
    size_t output_capacity;
    uint32_t deadline_ms;
} hesia_inference_request_t;

typedef struct hesia_inference_result {
    hesia_inference_state_t state;
    size_t output_len;
    uint32_t latency_ms;
    hesia_status_t status;
} hesia_inference_result_t;

HESIA_CORE_API const char *hesia_version(void);
HESIA_CORE_API const char *hesia_status_string(hesia_status_t status);
HESIA_CORE_API int hesia_feature_available(hesia_feature_t feature);
HESIA_CORE_API const char *hesia_feature_provider_version(hesia_feature_t feature);

HESIA_CORE_API void hesia_secure_zero(void *ptr, size_t len);
HESIA_CORE_API hesia_status_t hesia_random(uint8_t *out, size_t len, hesia_error_t *error);

HESIA_CORE_API void hesia_error_init(hesia_error_t *error);
HESIA_CORE_API void hesia_error_set(
    hesia_error_t *error,
    hesia_status_t status,
    const char *subsystem,
    const char *message);
HESIA_CORE_API const char *hesia_error_message(const hesia_error_t *error);

HESIA_CORE_API hesia_status_t hesia_watchdog_init(
    hesia_watchdog_t *watchdog,
    uint64_t timeout_ms,
    uint32_t max_missed,
    hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_watchdog_arm(hesia_watchdog_t *watchdog, hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_watchdog_heartbeat(hesia_watchdog_t *watchdog, hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_watchdog_check(hesia_watchdog_t *watchdog, hesia_error_t *error);
HESIA_CORE_API void hesia_watchdog_disarm(hesia_watchdog_t *watchdog);

HESIA_CORE_API hesia_status_t hesia_inference_validate(
    const hesia_model_contract_t *contract,
    const hesia_inference_request_t *request,
    hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_inference_record_result(
    hesia_inference_result_t *result,
    size_t output_len,
    uint32_t latency_ms,
    hesia_status_t status,
    hesia_error_t *error);

HESIA_CORE_API hesia_status_t hesia_aead_encrypt(
    hesia_aead_algorithm_t algorithm,
    const uint8_t *key,
    size_t key_len,
    const uint8_t *nonce,
    size_t nonce_len,
    const uint8_t *aad,
    size_t aad_len,
    const uint8_t *plaintext,
    size_t plaintext_len,
    uint8_t *ciphertext,
    size_t ciphertext_capacity,
    size_t *ciphertext_len,
    uint8_t *tag,
    size_t tag_len,
    hesia_error_t *error);

HESIA_CORE_API hesia_status_t hesia_aead_decrypt(
    hesia_aead_algorithm_t algorithm,
    const uint8_t *key,
    size_t key_len,
    const uint8_t *nonce,
    size_t nonce_len,
    const uint8_t *aad,
    size_t aad_len,
    const uint8_t *ciphertext,
    size_t ciphertext_len,
    const uint8_t *tag,
    size_t tag_len,
    uint8_t *plaintext,
    size_t plaintext_capacity,
    size_t *plaintext_len,
    hesia_error_t *error);

HESIA_CORE_API hesia_status_t hesia_pqc_public_key_bytes(
    hesia_pqc_algorithm_t algorithm,
    size_t *out_len,
    hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_pqc_secret_key_bytes(
    hesia_pqc_algorithm_t algorithm,
    size_t *out_len,
    hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_pqc_ciphertext_bytes(
    hesia_pqc_algorithm_t algorithm,
    size_t *out_len,
    hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_pqc_shared_secret_bytes(
    hesia_pqc_algorithm_t algorithm,
    size_t *out_len,
    hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_pqc_signature_bytes(
    hesia_pqc_algorithm_t algorithm,
    size_t *out_len,
    hesia_error_t *error);

HESIA_CORE_API hesia_status_t hesia_pqc_keypair(
    hesia_pqc_algorithm_t algorithm,
    uint8_t *public_key,
    size_t public_key_len,
    uint8_t *secret_key,
    size_t secret_key_len,
    hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_pqc_encapsulate(
    hesia_pqc_algorithm_t algorithm,
    const uint8_t *public_key,
    size_t public_key_len,
    uint8_t *ciphertext,
    size_t ciphertext_len,
    uint8_t *shared_secret,
    size_t shared_secret_len,
    hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_pqc_decapsulate(
    hesia_pqc_algorithm_t algorithm,
    const uint8_t *secret_key,
    size_t secret_key_len,
    const uint8_t *ciphertext,
    size_t ciphertext_len,
    uint8_t *shared_secret,
    size_t shared_secret_len,
    hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_pqc_sign(
    hesia_pqc_algorithm_t algorithm,
    const uint8_t *secret_key,
    size_t secret_key_len,
    const uint8_t *message,
    size_t message_len,
    uint8_t *signature,
    size_t signature_capacity,
    size_t *signature_len,
    hesia_error_t *error);
HESIA_CORE_API hesia_status_t hesia_pqc_verify(
    hesia_pqc_algorithm_t algorithm,
    const uint8_t *public_key,
    size_t public_key_len,
    const uint8_t *message,
    size_t message_len,
    const uint8_t *signature,
    size_t signature_len,
    hesia_error_t *error);

#ifdef __cplusplus
}
#endif

#endif
