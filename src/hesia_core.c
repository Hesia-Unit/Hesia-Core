#include "hesia/core.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <bcrypt.h>
#elif defined(__linux__)
#  include <errno.h>
#  include <fcntl.h>
#  include <sys/random.h>
#  include <sys/time.h>
#  include <unistd.h>
#else
#  include <errno.h>
#  include <fcntl.h>
#  include <unistd.h>
#endif

#if defined(HESIA_CORE_WITH_OPENSSL)
#  include <openssl/evp.h>
#  include <openssl/opensslv.h>
#  include <openssl/rand.h>
#endif

#if defined(HESIA_CORE_WITH_LIBOQS)
#  include <oqs/oqs.h>
#endif

#include "algorithms/ascon.h"
#include "algorithms/ed25519.h"

static hesia_status_t hsc_fail(
    hesia_error_t *error,
    hesia_status_t status,
    const char *subsystem,
    const char *message) {
    hesia_error_set(error, status, subsystem, message);
    return status;
}

static void hsc_copy_string(char *dst, size_t dst_len, const char *src) {
    size_t copy_len;

    if (dst == NULL || dst_len == 0u) {
        return;
    }
    if (src == NULL) {
        src = "";
    }

    copy_len = strlen(src);
    if (copy_len >= dst_len) {
        copy_len = dst_len - 1u;
    }
    if (copy_len > 0u) {
        memcpy(dst, src, copy_len);
    }
    dst[copy_len] = '\0';
}

static uint64_t hsc_now_ms(void) {
#if defined(_WIN32)
    return (uint64_t)GetTickCount64();
#elif defined(CLOCK_MONOTONIC)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return ((uint64_t)ts.tv_sec * 1000u) + ((uint64_t)ts.tv_nsec / 1000000u);
    }
    return 0u;
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        return ((uint64_t)tv.tv_sec * 1000u) + ((uint64_t)tv.tv_usec / 1000u);
    }
    return 0u;
#endif
}

static hesia_status_t hsc_random_os(uint8_t *out, size_t len, hesia_error_t *error) {
    if (out == NULL && len > 0u) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "random", "output buffer is null");
    }
    if (len == 0u) {
        return HESIA_STATUS_OK;
    }

#if defined(HESIA_CORE_WITH_OPENSSL)
    if (RAND_bytes(out, (int)len) == 1) {
        return HESIA_STATUS_OK;
    }
    return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "random", "OpenSSL RAND_bytes failed");
#elif defined(_WIN32)
    if (BCryptGenRandom(NULL, out, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0) {
        return HESIA_STATUS_OK;
    }
    return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "random", "Windows BCryptGenRandom failed");
#elif defined(__linux__)
    {
        size_t done = 0u;
        while (done < len) {
            ssize_t rc = getrandom(out + done, len - done, 0);
            if (rc > 0) {
                done += (size_t)rc;
                continue;
            }
            if (rc == -1 && errno == EINTR) {
                continue;
            }
            break;
        }
        if (done == len) {
            return HESIA_STATUS_OK;
        }
    }
#endif

#if !defined(_WIN32)
    {
        int fd = open("/dev/urandom", O_RDONLY);
        size_t done = 0u;
        if (fd < 0) {
            return hsc_fail(error, HESIA_STATUS_IO_ERROR, "random", "cannot open /dev/urandom");
        }
        while (done < len) {
            ssize_t rc = read(fd, out + done, len - done);
            if (rc > 0) {
                done += (size_t)rc;
                continue;
            }
            if (rc == -1 && errno == EINTR) {
                continue;
            }
            close(fd);
            return hsc_fail(error, HESIA_STATUS_IO_ERROR, "random", "cannot read enough random bytes");
        }
        close(fd);
        return HESIA_STATUS_OK;
    }
#else
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "random", "no supported OS random provider");
#endif
}

#if defined(HESIA_CORE_WITH_LIBOQS)
static const char *hsc_oqs_kem_name(hesia_pqc_algorithm_t algorithm) {
    switch (algorithm) {
    case HESIA_PQC_ML_KEM_768:
#  if defined(OQS_KEM_alg_ml_kem_768)
        return OQS_KEM_alg_ml_kem_768;
#  else
        return "ML-KEM-768";
#  endif
    case HESIA_PQC_ML_KEM_1024:
#  if defined(OQS_KEM_alg_ml_kem_1024)
        return OQS_KEM_alg_ml_kem_1024;
#  else
        return "ML-KEM-1024";
#  endif
    default:
        return NULL;
    }
}

static const char *hsc_oqs_sig_name(hesia_pqc_algorithm_t algorithm) {
    switch (algorithm) {
    case HESIA_PQC_ML_DSA_65:
#  if defined(OQS_SIG_alg_ml_dsa_65)
        return OQS_SIG_alg_ml_dsa_65;
#  else
        return "ML-DSA-65";
#  endif
    case HESIA_PQC_ML_DSA_87:
#  if defined(OQS_SIG_alg_ml_dsa_87)
        return OQS_SIG_alg_ml_dsa_87;
#  else
        return "ML-DSA-87";
#  endif
    default:
        return NULL;
    }
}

static int hsc_is_kem(hesia_pqc_algorithm_t algorithm) {
    return algorithm == HESIA_PQC_ML_KEM_768 || algorithm == HESIA_PQC_ML_KEM_1024;
}

static int hsc_is_sig(hesia_pqc_algorithm_t algorithm) {
    return algorithm == HESIA_PQC_ML_DSA_65 || algorithm == HESIA_PQC_ML_DSA_87;
}
#endif

const char *hesia_version(void) {
    return "0.1.0";
}

const char *hesia_status_string(hesia_status_t status) {
    switch (status) {
    case HESIA_STATUS_OK:
        return "ok";
    case HESIA_STATUS_INVALID_ARGUMENT:
        return "invalid argument";
    case HESIA_STATUS_BUFFER_TOO_SMALL:
        return "buffer too small";
    case HESIA_STATUS_UNSUPPORTED:
        return "unsupported";
    case HESIA_STATUS_CRYPTO_ERROR:
        return "cryptographic error";
    case HESIA_STATUS_AUTHENTICATION_FAILED:
        return "authentication failed";
    case HESIA_STATUS_POLICY_DENIED:
        return "policy denied";
    case HESIA_STATUS_WATCHDOG_EXPIRED:
        return "watchdog expired";
    case HESIA_STATUS_INFERENCE_REJECTED:
        return "inference rejected";
    case HESIA_STATUS_IO_ERROR:
        return "io error";
    case HESIA_STATUS_INTERNAL_ERROR:
        return "internal error";
    default:
        return "unknown status";
    }
}

int hesia_feature_available(hesia_feature_t feature) {
    switch (feature) {
    case HESIA_FEATURE_OS_RANDOM:
    case HESIA_FEATURE_WATCHDOG:
    case HESIA_FEATURE_INFERENCE_GUARD:
    case HESIA_FEATURE_NATIVE_LW_CRYPTO:
        return 1;
    case HESIA_FEATURE_OPENSSL_AEAD:
#if defined(HESIA_CORE_WITH_OPENSSL)
        return 1;
#else
        return 0;
#endif
    case HESIA_FEATURE_LIBOQS_PQC:
#if defined(HESIA_CORE_WITH_LIBOQS)
        return 1;
#else
        return 0;
#endif
    default:
        return 0;
    }
}

const char *hesia_feature_provider_version(hesia_feature_t feature) {
    switch (feature) {
    case HESIA_FEATURE_OS_RANDOM:
#if defined(HESIA_CORE_WITH_OPENSSL)
        return OPENSSL_VERSION_TEXT;
#elif defined(_WIN32)
        return "Windows CNG BCrypt";
#elif defined(__linux__)
        return "Linux getrandom";
#else
        return "OS random";
#endif
    case HESIA_FEATURE_OPENSSL_AEAD:
#if defined(HESIA_CORE_WITH_OPENSSL)
        return OPENSSL_VERSION_TEXT;
#else
        return "unavailable";
#endif
    case HESIA_FEATURE_LIBOQS_PQC:
#if defined(HESIA_CORE_WITH_LIBOQS)
#  if defined(OQS_VERSION_TEXT)
        return OQS_VERSION_TEXT;
#  else
        return "liboqs";
#  endif
#else
        return "unavailable";
#endif
    case HESIA_FEATURE_WATCHDOG:
        return "builtin";
    case HESIA_FEATURE_INFERENCE_GUARD:
        return "builtin";
    default:
        return "unknown";
    }
}

void hesia_secure_zero(void *ptr, size_t len) {
    if (ptr == NULL || len == 0u) {
        return;
    }
#if defined(_WIN32)
    SecureZeroMemory(ptr, len);
#elif defined(__STDC_LIB_EXT1__)
    memset_s(ptr, len, 0, len);
#else
    {
        volatile uint8_t *p = (volatile uint8_t *)ptr;
        while (len-- > 0u) {
            *p++ = 0u;
        }
    }
#endif
}

hesia_status_t hesia_random(uint8_t *out, size_t len, hesia_error_t *error) {
    hesia_error_init(error);
    return hsc_random_os(out, len, error);
}

void hesia_error_init(hesia_error_t *error) {
    if (error == NULL) {
        return;
    }
    error->status = HESIA_STATUS_OK;
    error->subsystem[0] = '\0';
    error->message[0] = '\0';
}

void hesia_error_set(
    hesia_error_t *error,
    hesia_status_t status,
    const char *subsystem,
    const char *message) {
    if (error == NULL) {
        return;
    }
    error->status = status;
    hsc_copy_string(error->subsystem, sizeof(error->subsystem), subsystem);
    hsc_copy_string(error->message, sizeof(error->message), message);
}

const char *hesia_error_message(const hesia_error_t *error) {
    if (error == NULL) {
        return "";
    }
    if (error->message[0] != '\0') {
        return error->message;
    }
    return hesia_status_string(error->status);
}

hesia_status_t hesia_watchdog_init(
    hesia_watchdog_t *watchdog,
    uint64_t timeout_ms,
    uint32_t max_missed,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (watchdog == NULL || timeout_ms == 0u || max_missed == 0u) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "watchdog", "invalid watchdog configuration");
    }
    watchdog->timeout_ms = timeout_ms;
    watchdog->last_heartbeat_ms = 0u;
    watchdog->max_missed = max_missed;
    watchdog->missed = 0u;
    watchdog->armed = 0;
    return HESIA_STATUS_OK;
}

hesia_status_t hesia_watchdog_arm(hesia_watchdog_t *watchdog, hesia_error_t *error) {
    hesia_error_init(error);
    if (watchdog == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "watchdog", "watchdog is null");
    }
    watchdog->last_heartbeat_ms = hsc_now_ms();
    watchdog->missed = 0u;
    watchdog->armed = 1;
    return HESIA_STATUS_OK;
}

hesia_status_t hesia_watchdog_heartbeat(hesia_watchdog_t *watchdog, hesia_error_t *error) {
    hesia_error_init(error);
    if (watchdog == NULL || watchdog->armed == 0) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "watchdog", "watchdog is not armed");
    }
    watchdog->last_heartbeat_ms = hsc_now_ms();
    watchdog->missed = 0u;
    return HESIA_STATUS_OK;
}

hesia_status_t hesia_watchdog_check(hesia_watchdog_t *watchdog, hesia_error_t *error) {
    uint64_t now;
    uint64_t elapsed;

    hesia_error_init(error);
    if (watchdog == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "watchdog", "watchdog is null");
    }
    if (watchdog->armed == 0) {
        return HESIA_STATUS_OK;
    }

    now = hsc_now_ms();
    elapsed = now >= watchdog->last_heartbeat_ms ? now - watchdog->last_heartbeat_ms : UINT64_MAX;
    if (elapsed <= watchdog->timeout_ms) {
        return HESIA_STATUS_OK;
    }

    if (watchdog->missed < UINT32_MAX) {
        watchdog->missed += 1u;
    }
    watchdog->last_heartbeat_ms = now;
    if (watchdog->missed >= watchdog->max_missed) {
        return hsc_fail(error, HESIA_STATUS_WATCHDOG_EXPIRED, "watchdog", "watchdog deadline expired");
    }
    return HESIA_STATUS_OK;
}

void hesia_watchdog_disarm(hesia_watchdog_t *watchdog) {
    if (watchdog == NULL) {
        return;
    }
    watchdog->armed = 0;
}

hesia_status_t hesia_inference_validate(
    const hesia_model_contract_t *contract,
    const hesia_inference_request_t *request,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (contract == NULL || request == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "inference", "contract or request is null");
    }
    if (contract->model_id == NULL || contract->model_id[0] == '\0') {
        return hsc_fail(error, HESIA_STATUS_POLICY_DENIED, "inference", "model id is missing");
    }
    if (contract->max_input_bytes == 0u || contract->max_output_bytes == 0u) {
        return hsc_fail(error, HESIA_STATUS_POLICY_DENIED, "inference", "model limits are not configured");
    }
    if (contract->require_signed_model != 0 &&
        (contract->model_digest == NULL || contract->model_digest_len == 0u)) {
        return hsc_fail(error, HESIA_STATUS_POLICY_DENIED, "inference", "signed model digest is required");
    }
    if (request->input == NULL && request->input_len > 0u) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "inference", "input pointer is null");
    }
    if (request->output == NULL || request->output_capacity == 0u) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "inference", "output buffer is missing");
    }
    if (request->input_len > contract->max_input_bytes) {
        return hsc_fail(error, HESIA_STATUS_INFERENCE_REJECTED, "inference", "input exceeds model contract");
    }
    if (request->output_capacity > contract->max_output_bytes) {
        return hsc_fail(error, HESIA_STATUS_INFERENCE_REJECTED, "inference", "output capacity exceeds model contract");
    }
    if (contract->max_latency_ms > 0u && request->deadline_ms > contract->max_latency_ms) {
        return hsc_fail(error, HESIA_STATUS_INFERENCE_REJECTED, "inference", "deadline exceeds model latency policy");
    }
    return HESIA_STATUS_OK;
}

hesia_status_t hesia_inference_record_result(
    hesia_inference_result_t *result,
    size_t output_len,
    uint32_t latency_ms,
    hesia_status_t status,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (result == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "inference", "result is null");
    }
    result->output_len = output_len;
    result->latency_ms = latency_ms;
    result->status = status;
    result->state = status == HESIA_STATUS_OK ? HESIA_INFERENCE_COMPLETE : HESIA_INFERENCE_FAILED;
    return HESIA_STATUS_OK;
}

hesia_status_t hesia_aead_encrypt(
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
    hesia_error_t *error) {
    hesia_error_init(error);
    if (algorithm == HESIA_AEAD_ASCON_128) {
        if (key == NULL || key_len != 16u || nonce == NULL || nonce_len != 16u ||
            (plaintext == NULL && plaintext_len > 0u) || ciphertext == NULL ||
            ciphertext_len == NULL || tag == NULL || tag_len != 16u) {
            return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "aead", "invalid ASCON arguments");
        }
        if (ciphertext_capacity < plaintext_len) {
            return hsc_fail(error, HESIA_STATUS_BUFFER_TOO_SMALL, "aead", "ciphertext buffer is too small");
        }
        ascon128_encrypt(tag, ciphertext, key, nonce, aad, aad_len, plaintext, plaintext_len);
        *ciphertext_len = plaintext_len;
        return HESIA_STATUS_OK;
    }
    if (algorithm != HESIA_AEAD_AES_256_GCM) {
        return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "aead", "unsupported AEAD algorithm");
    }
    if (key == NULL || key_len != 32u || nonce == NULL || nonce_len == 0u ||
        (plaintext == NULL && plaintext_len > 0u) || ciphertext == NULL ||
        ciphertext_len == NULL || tag == NULL || tag_len == 0u) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "aead", "invalid AEAD encryption arguments");
    }
    if (ciphertext_capacity < plaintext_len) {
        return hsc_fail(error, HESIA_STATUS_BUFFER_TOO_SMALL, "aead", "ciphertext buffer is too small");
    }

#if defined(HESIA_CORE_WITH_OPENSSL)
    {
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        int len = 0;
        int total = 0;
        if (ctx == NULL) {
            return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "aead", "cannot allocate EVP context");
        }
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)nonce_len, NULL) != 1 ||
            EVP_EncryptInit_ex(ctx, NULL, NULL, key, nonce) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "aead", "AES-GCM init failed");
        }
        if (aad != NULL && aad_len > 0u &&
            EVP_EncryptUpdate(ctx, NULL, &len, aad, (int)aad_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "aead", "AES-GCM AAD failed");
        }
        if (plaintext_len > 0u &&
            EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, (int)plaintext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "aead", "AES-GCM encrypt failed");
        }
        total = len;
        if (EVP_EncryptFinal_ex(ctx, ciphertext + total, &len) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, (int)tag_len, tag) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "aead", "AES-GCM finalize failed");
        }
        total += len;
        *ciphertext_len = (size_t)total;
        EVP_CIPHER_CTX_free(ctx);
        return HESIA_STATUS_OK;
    }
#else
    (void)aad;
    (void)aad_len;
    (void)ciphertext;
    (void)ciphertext_len;
    (void)tag;
    (void)tag_len;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "aead", "OpenSSL provider is not compiled in");
#endif
}

hesia_status_t hesia_aead_decrypt(
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
    hesia_error_t *error) {
    hesia_error_init(error);
    if (algorithm == HESIA_AEAD_ASCON_128) {
        if (key == NULL || key_len != 16u || nonce == NULL || nonce_len != 16u ||
            (ciphertext == NULL && ciphertext_len > 0u) || tag == NULL || tag_len != 16u ||
            plaintext == NULL || plaintext_len == NULL) {
            return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "aead", "invalid ASCON arguments");
        }
        if (plaintext_capacity < ciphertext_len) {
            return hsc_fail(error, HESIA_STATUS_BUFFER_TOO_SMALL, "aead", "plaintext buffer is too small");
        }
        if (ascon128_decrypt(plaintext, key, nonce, aad, aad_len, ciphertext, ciphertext_len, tag) != 0) {
            hesia_secure_zero(plaintext, plaintext_capacity);
            return hsc_fail(error, HESIA_STATUS_AUTHENTICATION_FAILED, "aead", "ASCON tag verification failed");
        }
        *plaintext_len = ciphertext_len;
        return HESIA_STATUS_OK;
    }
    if (algorithm != HESIA_AEAD_AES_256_GCM) {
        return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "aead", "unsupported AEAD algorithm");
    }
    if (key == NULL || key_len != 32u || nonce == NULL || nonce_len == 0u ||
        (ciphertext == NULL && ciphertext_len > 0u) || tag == NULL || tag_len == 0u ||
        plaintext == NULL || plaintext_len == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "aead", "invalid AEAD decryption arguments");
    }
    if (plaintext_capacity < ciphertext_len) {
        return hsc_fail(error, HESIA_STATUS_BUFFER_TOO_SMALL, "aead", "plaintext buffer is too small");
    }

#if defined(HESIA_CORE_WITH_OPENSSL)
    {
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        int len = 0;
        int total = 0;
        int final_rc = 0;
        if (ctx == NULL) {
            return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "aead", "cannot allocate EVP context");
        }
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)nonce_len, NULL) != 1 ||
            EVP_DecryptInit_ex(ctx, NULL, NULL, key, nonce) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "aead", "AES-GCM init failed");
        }
        if (aad != NULL && aad_len > 0u &&
            EVP_DecryptUpdate(ctx, NULL, &len, aad, (int)aad_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "aead", "AES-GCM AAD failed");
        }
        if (ciphertext_len > 0u &&
            EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, (int)ciphertext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "aead", "AES-GCM decrypt failed");
        }
        total = len;
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, (int)tag_len, (void *)tag) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "aead", "AES-GCM tag setup failed");
        }
        final_rc = EVP_DecryptFinal_ex(ctx, plaintext + total, &len);
        EVP_CIPHER_CTX_free(ctx);
        if (final_rc != 1) {
            hesia_secure_zero(plaintext, plaintext_capacity);
            return hsc_fail(error, HESIA_STATUS_AUTHENTICATION_FAILED, "aead", "AES-GCM tag verification failed");
        }
        total += len;
        *plaintext_len = (size_t)total;
        return HESIA_STATUS_OK;
    }
#else
    (void)aad;
    (void)aad_len;
    (void)plaintext;
    (void)plaintext_len;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "aead", "OpenSSL provider is not compiled in");
#endif
}

hesia_status_t hesia_pqc_public_key_bytes(
    hesia_pqc_algorithm_t algorithm,
    size_t *out_len,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (out_len == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "pqc", "length output is null");
    }
#if defined(HESIA_CORE_WITH_LIBOQS)
    if (hsc_is_kem(algorithm)) {
        OQS_KEM *kem = OQS_KEM_new(hsc_oqs_kem_name(algorithm));
        if (kem == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "KEM algorithm unavailable");
        }
        *out_len = kem->length_public_key;
        OQS_KEM_free(kem);
        return HESIA_STATUS_OK;
    }
    if (hsc_is_sig(algorithm)) {
        OQS_SIG *sig = OQS_SIG_new(hsc_oqs_sig_name(algorithm));
        if (sig == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "signature algorithm unavailable");
        }
        *out_len = sig->length_public_key;
        OQS_SIG_free(sig);
        return HESIA_STATUS_OK;
    }
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "unknown PQC algorithm");
#else
    (void)algorithm;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "liboqs provider is not compiled in");
#endif
}

hesia_status_t hesia_pqc_secret_key_bytes(
    hesia_pqc_algorithm_t algorithm,
    size_t *out_len,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (out_len == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "pqc", "length output is null");
    }
#if defined(HESIA_CORE_WITH_LIBOQS)
    if (hsc_is_kem(algorithm)) {
        OQS_KEM *kem = OQS_KEM_new(hsc_oqs_kem_name(algorithm));
        if (kem == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "KEM algorithm unavailable");
        }
        *out_len = kem->length_secret_key;
        OQS_KEM_free(kem);
        return HESIA_STATUS_OK;
    }
    if (hsc_is_sig(algorithm)) {
        OQS_SIG *sig = OQS_SIG_new(hsc_oqs_sig_name(algorithm));
        if (sig == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "signature algorithm unavailable");
        }
        *out_len = sig->length_secret_key;
        OQS_SIG_free(sig);
        return HESIA_STATUS_OK;
    }
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "unknown PQC algorithm");
#else
    (void)algorithm;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "liboqs provider is not compiled in");
#endif
}

hesia_status_t hesia_pqc_ciphertext_bytes(
    hesia_pqc_algorithm_t algorithm,
    size_t *out_len,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (out_len == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "pqc", "length output is null");
    }
#if defined(HESIA_CORE_WITH_LIBOQS)
    if (!hsc_is_kem(algorithm)) {
        return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "ciphertext length is only valid for KEM algorithms");
    }
    {
        OQS_KEM *kem = OQS_KEM_new(hsc_oqs_kem_name(algorithm));
        if (kem == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "KEM algorithm unavailable");
        }
        *out_len = kem->length_ciphertext;
        OQS_KEM_free(kem);
        return HESIA_STATUS_OK;
    }
#else
    (void)algorithm;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "liboqs provider is not compiled in");
#endif
}

hesia_status_t hesia_pqc_shared_secret_bytes(
    hesia_pqc_algorithm_t algorithm,
    size_t *out_len,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (out_len == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "pqc", "length output is null");
    }
#if defined(HESIA_CORE_WITH_LIBOQS)
    if (!hsc_is_kem(algorithm)) {
        return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "shared secret length is only valid for KEM algorithms");
    }
    {
        OQS_KEM *kem = OQS_KEM_new(hsc_oqs_kem_name(algorithm));
        if (kem == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "KEM algorithm unavailable");
        }
        *out_len = kem->length_shared_secret;
        OQS_KEM_free(kem);
        return HESIA_STATUS_OK;
    }
#else
    (void)algorithm;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "liboqs provider is not compiled in");
#endif
}

hesia_status_t hesia_pqc_signature_bytes(
    hesia_pqc_algorithm_t algorithm,
    size_t *out_len,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (out_len == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "pqc", "length output is null");
    }
#if defined(HESIA_CORE_WITH_LIBOQS)
    if (!hsc_is_sig(algorithm)) {
        return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "signature length is only valid for signature algorithms");
    }
    {
        OQS_SIG *sig = OQS_SIG_new(hsc_oqs_sig_name(algorithm));
        if (sig == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "signature algorithm unavailable");
        }
        *out_len = sig->length_signature;
        OQS_SIG_free(sig);
        return HESIA_STATUS_OK;
    }
#else
    (void)algorithm;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "liboqs provider is not compiled in");
#endif
}

hesia_status_t hesia_pqc_keypair(
    hesia_pqc_algorithm_t algorithm,
    uint8_t *public_key,
    size_t public_key_len,
    uint8_t *secret_key,
    size_t secret_key_len,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (public_key == NULL || secret_key == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "pqc", "key output buffer is null");
    }
#if defined(HESIA_CORE_WITH_LIBOQS)
    if (hsc_is_kem(algorithm)) {
        OQS_KEM *kem = OQS_KEM_new(hsc_oqs_kem_name(algorithm));
        OQS_STATUS rc;
        if (kem == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "KEM algorithm unavailable");
        }
        if (public_key_len < kem->length_public_key || secret_key_len < kem->length_secret_key) {
            OQS_KEM_free(kem);
            return hsc_fail(error, HESIA_STATUS_BUFFER_TOO_SMALL, "pqc", "KEM key buffer too small");
        }
        rc = OQS_KEM_keypair(kem, public_key, secret_key);
        OQS_KEM_free(kem);
        return rc == OQS_SUCCESS ? HESIA_STATUS_OK :
            hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "pqc", "KEM keypair failed");
    }
    if (hsc_is_sig(algorithm)) {
        OQS_SIG *sig = OQS_SIG_new(hsc_oqs_sig_name(algorithm));
        OQS_STATUS rc;
        if (sig == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "signature algorithm unavailable");
        }
        if (public_key_len < sig->length_public_key || secret_key_len < sig->length_secret_key) {
            OQS_SIG_free(sig);
            return hsc_fail(error, HESIA_STATUS_BUFFER_TOO_SMALL, "pqc", "signature key buffer too small");
        }
        rc = OQS_SIG_keypair(sig, public_key, secret_key);
        OQS_SIG_free(sig);
        return rc == OQS_SUCCESS ? HESIA_STATUS_OK :
            hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "pqc", "signature keypair failed");
    }
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "unknown PQC algorithm");
#else
    (void)algorithm;
    (void)public_key_len;
    (void)secret_key_len;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "liboqs provider is not compiled in");
#endif
}

hesia_status_t hesia_pqc_encapsulate(
    hesia_pqc_algorithm_t algorithm,
    const uint8_t *public_key,
    size_t public_key_len,
    uint8_t *ciphertext,
    size_t ciphertext_len,
    uint8_t *shared_secret,
    size_t shared_secret_len,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (public_key == NULL || ciphertext == NULL || shared_secret == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "pqc", "KEM argument is null");
    }
#if defined(HESIA_CORE_WITH_LIBOQS)
    {
        OQS_KEM *kem = OQS_KEM_new(hsc_oqs_kem_name(algorithm));
        OQS_STATUS rc;
        if (kem == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "KEM algorithm unavailable");
        }
        if (public_key_len < kem->length_public_key || ciphertext_len < kem->length_ciphertext ||
            shared_secret_len < kem->length_shared_secret) {
            OQS_KEM_free(kem);
            return hsc_fail(error, HESIA_STATUS_BUFFER_TOO_SMALL, "pqc", "KEM buffer too small");
        }
        rc = OQS_KEM_encaps(kem, ciphertext, shared_secret, public_key);
        OQS_KEM_free(kem);
        return rc == OQS_SUCCESS ? HESIA_STATUS_OK :
            hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "pqc", "KEM encapsulation failed");
    }
#else
    (void)algorithm;
    (void)public_key_len;
    (void)ciphertext_len;
    (void)shared_secret_len;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "liboqs provider is not compiled in");
#endif
}

hesia_status_t hesia_pqc_decapsulate(
    hesia_pqc_algorithm_t algorithm,
    const uint8_t *secret_key,
    size_t secret_key_len,
    const uint8_t *ciphertext,
    size_t ciphertext_len,
    uint8_t *shared_secret,
    size_t shared_secret_len,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (secret_key == NULL || ciphertext == NULL || shared_secret == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "pqc", "KEM argument is null");
    }
#if defined(HESIA_CORE_WITH_LIBOQS)
    {
        OQS_KEM *kem = OQS_KEM_new(hsc_oqs_kem_name(algorithm));
        OQS_STATUS rc;
        if (kem == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "KEM algorithm unavailable");
        }
        if (secret_key_len < kem->length_secret_key || ciphertext_len < kem->length_ciphertext ||
            shared_secret_len < kem->length_shared_secret) {
            OQS_KEM_free(kem);
            return hsc_fail(error, HESIA_STATUS_BUFFER_TOO_SMALL, "pqc", "KEM buffer too small");
        }
        rc = OQS_KEM_decaps(kem, shared_secret, ciphertext, secret_key);
        OQS_KEM_free(kem);
        return rc == OQS_SUCCESS ? HESIA_STATUS_OK :
            hsc_fail(error, HESIA_STATUS_AUTHENTICATION_FAILED, "pqc", "KEM decapsulation failed");
    }
#else
    (void)algorithm;
    (void)secret_key_len;
    (void)ciphertext_len;
    (void)shared_secret_len;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "liboqs provider is not compiled in");
#endif
}

hesia_status_t hesia_pqc_sign(
    hesia_pqc_algorithm_t algorithm,
    const uint8_t *secret_key,
    size_t secret_key_len,
    const uint8_t *message,
    size_t message_len,
    uint8_t *signature,
    size_t signature_capacity,
    size_t *signature_len,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (secret_key == NULL || (message == NULL && message_len > 0u) ||
        signature == NULL || signature_len == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "pqc", "signature argument is null");
    }
#if defined(HESIA_CORE_WITH_LIBOQS)
    {
        OQS_SIG *sig = OQS_SIG_new(hsc_oqs_sig_name(algorithm));
        OQS_STATUS rc;
        if (sig == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "signature algorithm unavailable");
        }
        if (secret_key_len < sig->length_secret_key || signature_capacity < sig->length_signature) {
            OQS_SIG_free(sig);
            return hsc_fail(error, HESIA_STATUS_BUFFER_TOO_SMALL, "pqc", "signature buffer too small");
        }
        rc = OQS_SIG_sign(sig, signature, signature_len, message, message_len, secret_key);
        OQS_SIG_free(sig);
        return rc == OQS_SUCCESS ? HESIA_STATUS_OK :
            hsc_fail(error, HESIA_STATUS_CRYPTO_ERROR, "pqc", "signing failed");
    }
#else
    (void)algorithm;
    (void)secret_key_len;
    (void)signature_capacity;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "liboqs provider is not compiled in");
#endif
}

hesia_status_t hesia_pqc_verify(
    hesia_pqc_algorithm_t algorithm,
    const uint8_t *public_key,
    size_t public_key_len,
    const uint8_t *message,
    size_t message_len,
    const uint8_t *signature,
    size_t signature_len,
    hesia_error_t *error) {
    hesia_error_init(error);
    if (public_key == NULL || (message == NULL && message_len > 0u) || signature == NULL) {
        return hsc_fail(error, HESIA_STATUS_INVALID_ARGUMENT, "pqc", "verify argument is null");
    }
#if defined(HESIA_CORE_WITH_LIBOQS)
    {
        OQS_SIG *sig = OQS_SIG_new(hsc_oqs_sig_name(algorithm));
        OQS_STATUS rc;
        if (sig == NULL) {
            return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "signature algorithm unavailable");
        }
        if (public_key_len < sig->length_public_key || signature_len > sig->length_signature) {
            OQS_SIG_free(sig);
            return hsc_fail(error, HESIA_STATUS_BUFFER_TOO_SMALL, "pqc", "verify buffer size is invalid");
        }
        rc = OQS_SIG_verify(sig, message, message_len, signature, signature_len, public_key);
        OQS_SIG_free(sig);
        return rc == OQS_SUCCESS ? HESIA_STATUS_OK :
            hsc_fail(error, HESIA_STATUS_AUTHENTICATION_FAILED, "pqc", "signature verification failed");
    }
#else
    (void)algorithm;
    (void)public_key_len;
    (void)signature_len;
    return hsc_fail(error, HESIA_STATUS_UNSUPPORTED, "pqc", "liboqs provider is not compiled in");
#endif
}

hesia_status_t hesia_sig_public_key_bytes(hesia_sig_algorithm_t algorithm, size_t *out_len, hesia_error_t *error) {
    hesia_error_init(error);
    if (algorithm == HESIA_SIG_ED25519) {
        if (out_len == NULL) return HESIA_STATUS_INVALID_ARGUMENT;
        *out_len = 32; return HESIA_STATUS_OK;
    }
    return hesia_pqc_public_key_bytes((hesia_pqc_algorithm_t)algorithm, out_len, error);
}

hesia_status_t hesia_sig_secret_key_bytes(hesia_sig_algorithm_t algorithm, size_t *out_len, hesia_error_t *error) {
    hesia_error_init(error);
    if (algorithm == HESIA_SIG_ED25519) {
        if (out_len == NULL) return HESIA_STATUS_INVALID_ARGUMENT;
        *out_len = 64; return HESIA_STATUS_OK;
    }
    return hesia_pqc_secret_key_bytes((hesia_pqc_algorithm_t)algorithm, out_len, error);
}

hesia_status_t hesia_sig_signature_bytes(hesia_sig_algorithm_t algorithm, size_t *out_len, hesia_error_t *error) {
    hesia_error_init(error);
    if (algorithm == HESIA_SIG_ED25519) {
        if (out_len == NULL) return HESIA_STATUS_INVALID_ARGUMENT;
        *out_len = 64; return HESIA_STATUS_OK;
    }
    return hesia_pqc_signature_bytes((hesia_pqc_algorithm_t)algorithm, out_len, error);
}

hesia_status_t hesia_sig_keypair(hesia_sig_algorithm_t algorithm, uint8_t *public_key, size_t public_key_len, uint8_t *secret_key, size_t secret_key_len, hesia_error_t *error) {
    hesia_error_init(error);
    if (algorithm == HESIA_SIG_ED25519) {
        uint8_t seed[32];
        if (public_key_len < 32 || secret_key_len < 64) return HESIA_STATUS_BUFFER_TOO_SMALL;
        hesia_random(seed, 32, error);
        ed25519_keypair(public_key, secret_key, seed);
        return HESIA_STATUS_OK;
    }
    return hesia_pqc_keypair((hesia_pqc_algorithm_t)algorithm, public_key, public_key_len, secret_key, secret_key_len, error);
}

hesia_status_t hesia_sig_sign(hesia_sig_algorithm_t algorithm, const uint8_t *secret_key, size_t secret_key_len, const uint8_t *message, size_t message_len, uint8_t *signature, size_t signature_capacity, size_t *signature_len, hesia_error_t *error) {
    hesia_error_init(error);
    if (algorithm == HESIA_SIG_ED25519) {
        uint8_t public_key[32];
        if (signature_capacity < 64) return HESIA_STATUS_BUFFER_TOO_SMALL;
        /* Convention: secret_key is seed (32) + public_key (32) */
        if (secret_key_len < 64) return HESIA_STATUS_INVALID_ARGUMENT;
        memcpy(public_key, secret_key + 32, 32); 
        ed25519_sign(signature, message, message_len, public_key, secret_key);
        *signature_len = 64;
        return HESIA_STATUS_OK;
    }
    return hesia_pqc_sign((hesia_pqc_algorithm_t)algorithm, secret_key, secret_key_len, message, message_len, signature, signature_capacity, signature_len, error);
}

hesia_status_t hesia_sig_verify(hesia_sig_algorithm_t algorithm, const uint8_t *public_key, size_t public_key_len, const uint8_t *message, size_t message_len, const uint8_t *signature, size_t signature_len, hesia_error_t *error) {
    hesia_error_init(error);
    if (algorithm == HESIA_SIG_ED25519) {
        if (public_key_len != 32 || signature_len != 64) return HESIA_STATUS_INVALID_ARGUMENT;
        if (ed25519_verify(signature, message, message_len, public_key)) return HESIA_STATUS_OK;
        return HESIA_STATUS_AUTHENTICATION_FAILED;
    }
    return hesia_pqc_verify((hesia_pqc_algorithm_t)algorithm, public_key, public_key_len, message, message_len, signature, signature_len, error);
}

hesia_status_t hesia_maintenance_get_info(hesia_maintenance_info_t *info, hesia_error_t *error) {
    hesia_error_init(error);
    if (info == NULL) return HESIA_STATUS_INVALID_ARGUMENT;
    info->firmware_version_packed = (HESIA_CORE_VERSION_MAJOR << 16) | (HESIA_CORE_VERSION_MINOR << 8) | HESIA_CORE_VERSION_PATCH;
    info->security_patch_level = 20260503;
    info->last_maintenance_unix = (uint64_t)time(NULL);
    info->tamper_detected = 0;
    info->debug_mode_active = 0;
    return HESIA_STATUS_OK;
}

