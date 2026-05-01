#include "hesia/core.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static void test_errors(void) {
    hesia_error_t error;
    hesia_error_init(&error);
    assert(error.status == HESIA_STATUS_OK);
    hesia_error_set(&error, HESIA_STATUS_POLICY_DENIED, "policy", "blocked");
    assert(error.status == HESIA_STATUS_POLICY_DENIED);
    assert(strcmp(hesia_error_message(&error), "blocked") == 0);
}

static void test_secure_zero(void) {
    uint8_t secret[4] = {1u, 2u, 3u, 4u};
    hesia_secure_zero(secret, sizeof(secret));
    assert(secret[0] == 0u);
    assert(secret[1] == 0u);
    assert(secret[2] == 0u);
    assert(secret[3] == 0u);
}

static void test_watchdog(void) {
    hesia_watchdog_t watchdog;
    hesia_error_t error;
    assert(hesia_watchdog_init(&watchdog, 1u, 1u, &error) == HESIA_STATUS_OK);
    assert(hesia_watchdog_arm(&watchdog, &error) == HESIA_STATUS_OK);
    watchdog.last_heartbeat_ms = 0u;
    assert(hesia_watchdog_check(&watchdog, &error) == HESIA_STATUS_WATCHDOG_EXPIRED);
    hesia_watchdog_disarm(&watchdog);
    assert(hesia_watchdog_check(&watchdog, &error) == HESIA_STATUS_OK);
}

static void test_inference_guard(void) {
    uint8_t input[8] = {0u};
    uint8_t output[8] = {0u};
    hesia_model_contract_t contract;
    hesia_inference_request_t request;
    hesia_inference_result_t result;
    hesia_error_t error;

    memset(&contract, 0, sizeof(contract));
    memset(&request, 0, sizeof(request));
    contract.model_id = "demo";
    contract.model_version = 1u;
    contract.max_input_bytes = sizeof(input);
    contract.max_output_bytes = sizeof(output);
    contract.max_latency_ms = 50u;
    request.input = input;
    request.input_len = sizeof(input);
    request.output = output;
    request.output_capacity = sizeof(output);
    request.deadline_ms = 10u;

    assert(hesia_inference_validate(&contract, &request, &error) == HESIA_STATUS_OK);
    request.input_len = 128u;
    assert(hesia_inference_validate(&contract, &request, &error) == HESIA_STATUS_INFERENCE_REJECTED);

    assert(hesia_inference_record_result(&result, 4u, 9u, HESIA_STATUS_OK, &error) == HESIA_STATUS_OK);
    assert(result.state == HESIA_INFERENCE_COMPLETE);
    assert(result.output_len == 4u);
}

static void test_optional_crypto_is_fail_closed(void) {
    hesia_error_t error;
    size_t len = 0u;
    uint8_t key[32] = {0u};
    uint8_t nonce[12] = {0u};
    uint8_t plaintext[4] = {1u, 2u, 3u, 4u};
    uint8_t ciphertext[4] = {0u};
    uint8_t tag[16] = {0u};
    hesia_status_t status;

    if (!hesia_feature_available(HESIA_FEATURE_OPENSSL_AEAD)) {
        status = hesia_aead_encrypt(
            HESIA_AEAD_AES_256_GCM,
            key,
            sizeof(key),
            nonce,
            sizeof(nonce),
            NULL,
            0u,
            plaintext,
            sizeof(plaintext),
            ciphertext,
            sizeof(ciphertext),
            &len,
            tag,
            sizeof(tag),
            &error);
        assert(status == HESIA_STATUS_UNSUPPORTED);
    }

    if (!hesia_feature_available(HESIA_FEATURE_LIBOQS_PQC)) {
        status = hesia_pqc_public_key_bytes(HESIA_PQC_ML_KEM_1024, &len, &error);
        assert(status == HESIA_STATUS_UNSUPPORTED);
    }
}

int main(void) {
    assert(strcmp(hesia_version(), "0.1.0") == 0);
    assert(hesia_feature_available(HESIA_FEATURE_WATCHDOG) == 1);
    assert(hesia_feature_available(HESIA_FEATURE_INFERENCE_GUARD) == 1);
    assert(strcmp(hesia_feature_provider_version(HESIA_FEATURE_WATCHDOG), "builtin") == 0);

    test_errors();
    test_secure_zero();
    test_watchdog();
    test_inference_guard();
    test_optional_crypto_is_fail_closed();

    return 0;
}
