#ifndef HESIA_ALGORITHMS_ED25519_H
#define HESIA_ALGORITHMS_ED25519_H

#include <stdint.h>
#include <stddef.h>

void ed25519_keypair(uint8_t *public_key, uint8_t *private_key, const uint8_t *seed);
void ed25519_sign(uint8_t *signature, const uint8_t *message, size_t message_len, const uint8_t *public_key, const uint8_t *private_key);
int ed25519_verify(const uint8_t *signature, const uint8_t *message, size_t message_len, const uint8_t *public_key);

#endif
