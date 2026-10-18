#ifndef HESIA_ALGORITHMS_ASCON_H
#define HESIA_ALGORITHMS_ASCON_H

#include <stdint.h>
#include <stddef.h>

/* Ascon-128 v1.2 AEAD, the CAESAR and NIST LWC finalist version: 128-bit key,
   nonce and tag, 64-bit rate. It is not the NIST SP 800-232 Ascon-AEAD128, which
   uses a different IV, rate and byte order. tests/test_ascon_kat.c checks it
   against the official test vectors.

   ascon128_decrypt returns -1 and zeroes the clen bytes at p when the tag does
   not match. */
int ascon128_encrypt(
    uint8_t *t,
    uint8_t *c,
    const uint8_t *k,
    const uint8_t *n,
    const uint8_t *a,
    size_t alen,
    const uint8_t *p,
    size_t plen);

int ascon128_decrypt(
    uint8_t *p,
    const uint8_t *k,
    const uint8_t *n,
    const uint8_t *a,
    size_t alen,
    const uint8_t *c,
    size_t clen,
    const uint8_t *t);

#endif
