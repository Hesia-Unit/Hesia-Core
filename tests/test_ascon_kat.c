/* Checks the native Ascon-128 against the official v1.2 test vectors in
   tests/kat/ascon128v12/LWC_AEAD_KAT_128_128.txt, copied unchanged from the
   reference implementation (github.com/ascon/ascon-c, tag v1.2.7, CC0).

   For every vector: encryption must reproduce CT (ciphertext || tag), decryption
   must recover PT, and decryption with a corrupted tag must fail and leave the
   plaintext buffer zeroed. Plain checks rather than assert(), so the test still
   runs in builds that define NDEBUG. */

#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "algorithms/ascon.h"

#define KAT_VECTORS 1089
#define MAX_MSG 32
#define TAG_LEN 16

typedef struct {
    unsigned long count;
    uint8_t key[16];
    uint8_t nonce[16];
    uint8_t pt[MAX_MSG];
    uint8_t ad[MAX_MSG];
    uint8_t ct[MAX_MSG + TAG_LEN];
    size_t key_len;
    size_t nonce_len;
    size_t pt_len;
    size_t ad_len;
    size_t ct_len;
} kat_vector_t;

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_hex(const char *hex, uint8_t *out, size_t capacity, size_t *out_len) {
    size_t n = 0;
    while (hex[0] != '\0') {
        int hi = hex_digit(hex[0]);
        int lo = hex[1] != '\0' ? hex_digit(hex[1]) : -1;
        if (hi < 0 || lo < 0 || n >= capacity) return -1;
        out[n++] = (uint8_t)((hi << 4) | lo);
        hex += 2;
    }
    *out_len = n;
    return 0;
}

static int check_vector(const kat_vector_t *v) {
    uint8_t ct[MAX_MSG];
    uint8_t tag[TAG_LEN];
    uint8_t pt[MAX_MSG];
    uint8_t bad_tag[TAG_LEN];
    size_t i;

    if (v->key_len != 16 || v->nonce_len != 16 || v->ct_len != v->pt_len + TAG_LEN) {
        fprintf(stderr, "Count = %lu: malformed vector\n", v->count);
        return 0;
    }

    ascon128_encrypt(tag, ct, v->key, v->nonce, v->ad, v->ad_len, v->pt, v->pt_len);
    if (memcmp(ct, v->ct, v->pt_len) != 0 || memcmp(tag, v->ct + v->pt_len, TAG_LEN) != 0) {
        fprintf(stderr, "Count = %lu: encryption does not match CT\n", v->count);
        return 0;
    }

    memset(pt, 0xA5, sizeof(pt));
    if (ascon128_decrypt(pt, v->key, v->nonce, v->ad, v->ad_len, v->ct, v->pt_len, v->ct + v->pt_len) != 0 ||
        memcmp(pt, v->pt, v->pt_len) != 0) {
        fprintf(stderr, "Count = %lu: decryption does not recover PT\n", v->count);
        return 0;
    }

    memcpy(bad_tag, v->ct + v->pt_len, TAG_LEN);
    bad_tag[v->count % TAG_LEN] ^= 0x01u;
    memset(pt, 0xA5, sizeof(pt));
    if (ascon128_decrypt(pt, v->key, v->nonce, v->ad, v->ad_len, v->ct, v->pt_len, bad_tag) != -1) {
        fprintf(stderr, "Count = %lu: corrupted tag was accepted\n", v->count);
        return 0;
    }
    for (i = 0; i < v->pt_len; ++i) {
        if (pt[i] != 0u) {
            fprintf(stderr, "Count = %lu: plaintext left in the buffer after a tag failure\n", v->count);
            return 0;
        }
    }
    return 1;
}

static void trim_line_end(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ')) {
        s[--n] = '\0';
    }
}

int main(int argc, char **argv) {
    FILE *f;
    char line[512];
    kat_vector_t v;
    int vectors = 0;
    int passed = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <LWC_AEAD_KAT_128_128.txt>\n", argv[0]);
        return 2;
    }
    f = fopen(argv[1], "r");
    if (f == NULL) {
        perror(argv[1]);
        return 2;
    }

    memset(&v, 0, sizeof(v));
    while (fgets(line, sizeof(line), f) != NULL) {
        char *value = strstr(line, " =");
        int bad = 0;
        trim_line_end(line);
        if (value == NULL) continue;
        *value = '\0';
        value += 2;
        if (*value == ' ') value++;

        if (strcmp(line, "Count") == 0) {
            v.count = strtoul(value, NULL, 10);
        } else if (strcmp(line, "Key") == 0) {
            bad = parse_hex(value, v.key, sizeof(v.key), &v.key_len);
        } else if (strcmp(line, "Nonce") == 0) {
            bad = parse_hex(value, v.nonce, sizeof(v.nonce), &v.nonce_len);
        } else if (strcmp(line, "PT") == 0) {
            bad = parse_hex(value, v.pt, sizeof(v.pt), &v.pt_len);
        } else if (strcmp(line, "AD") == 0) {
            bad = parse_hex(value, v.ad, sizeof(v.ad), &v.ad_len);
        } else if (strcmp(line, "CT") == 0) {
            bad = parse_hex(value, v.ct, sizeof(v.ct), &v.ct_len);
            if (bad == 0) {
                vectors++;
                passed += check_vector(&v);
            }
        }
        if (bad != 0) {
            fprintf(stderr, "Count = %lu: cannot parse %s\n", v.count, line);
            fclose(f);
            return 1;
        }
    }
    fclose(f);

    printf("Ascon-128 v1.2 KAT: %d/%d vectors passed\n", passed, vectors);
    if (vectors != KAT_VECTORS) {
        fprintf(stderr, "expected %d vectors, read %d\n", KAT_VECTORS, vectors);
        return 1;
    }
    return passed == vectors ? 0 : 1;
}
