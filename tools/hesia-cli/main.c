#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hesia/core.h"

void print_usage(const char *prog) {
    printf("Usage: %s <command> [args]\n", prog);
    printf("\nCommands:\n");
    printf("  info        Show library and system information\n");
    printf("  init <dir>  Initialize a new HESIA-enabled project\n");
    printf("  keygen <alg> Generate a new keypair\n");
    printf("  version     Show version\n");
}

int cmd_info() {
    hesia_maintenance_info_t info;
    hesia_error_t err;
    printf("HESIA CoreLib version: %s\n", hesia_version());
    printf("Features:\n");
    printf("  OS Random:     %s\n", hesia_feature_available(HESIA_FEATURE_OS_RANDOM) ? "YES" : "NO");
    printf("  OpenSSL AEAD:  %s\n", hesia_feature_available(HESIA_FEATURE_OPENSSL_AEAD) ? "YES" : "NO");
    printf("  LibOQS PQC:    %s\n", hesia_feature_available(HESIA_FEATURE_LIBOQS_PQC) ? "YES" : "NO");
    printf("  Native LW:     %s\n", hesia_feature_available(HESIA_FEATURE_NATIVE_LW_CRYPTO) ? "YES" : "NO");
    
    if (hesia_maintenance_get_info(&info, &err) == HESIA_STATUS_OK) {
        printf("\nMaintenance Info:\n");
        printf("  Firmware:      %u.%u.%u\n", (info.firmware_version_packed >> 16) & 0xFF, (info.firmware_version_packed >> 8) & 0xFF, info.firmware_version_packed & 0xFF);
        printf("  Patch Level:   %u\n", info.security_patch_level);
        printf("  Tamper:        %s\n", info.tamper_detected ? "DETECTED" : "NONE");
    }
    return 0;
}

int cmd_keygen(const char *alg) {
    hesia_error_t err;
    hesia_status_t status;
    size_t pk_len, sk_len;
    uint8_t *pk, *sk;
    hesia_sig_algorithm_t sig_alg;

    if (strcmp(alg, "mldsa65") == 0) sig_alg = HESIA_SIG_ML_DSA_65;
    else if (strcmp(alg, "mldsa87") == 0) sig_alg = HESIA_SIG_ML_DSA_87;
    else {
        fprintf(stderr, "Unknown algorithm: %s\n", alg);
        return 1;
    }

    if (hesia_sig_public_key_bytes(sig_alg, &pk_len, &err) != HESIA_STATUS_OK ||
        hesia_sig_secret_key_bytes(sig_alg, &sk_len, &err) != HESIA_STATUS_OK) {
        fprintf(stderr, "Keygen unavailable: %s\n", hesia_error_message(&err));
        return 1;
    }

    pk = malloc(pk_len);
    sk = malloc(sk_len);

    printf("Generating %s keypair...\n", alg);
    status = hesia_sig_keypair(sig_alg, pk, pk_len, sk, sk_len, &err);
    if (status != HESIA_STATUS_OK) {
        fprintf(stderr, "Keygen failed: %s\n", hesia_error_message(&err));
        return 1;
    }

    printf("Success.\nPublic key (%zu bytes): ", pk_len);
    for(size_t i=0; i < pk_len && i < 16; i++) printf("%02x", pk[i]);
    printf("...\n");

    free(pk); free(sk);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "info") == 0) return cmd_info();
    if (strcmp(argv[1], "version") == 0) { printf("%s\n", hesia_version()); return 0; }
    if (strcmp(argv[1], "keygen") == 0) {
        if (argc < 3) { fprintf(stderr, "Missing algorithm\n"); return 1; }
        return cmd_keygen(argv[2]);
    }
    if (strcmp(argv[1], "init") == 0) {
        printf("Project initialization placeholder for %s\n", argc > 2 ? argv[2] : "current dir");
        return 0;
    }

    print_usage(argv[0]);
    return 1;
}
