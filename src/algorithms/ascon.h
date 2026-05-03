#ifndef HESIA_ALGORITHMS_ASCON_H
#define HESIA_ALGORITHMS_ASCON_H

#include <stdint.h>
#include <stddef.h>

/* ASCON-128 AEAD */
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
