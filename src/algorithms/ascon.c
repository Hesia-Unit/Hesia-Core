#include "ascon.h"
#include <string.h>

typedef struct {
  uint64_t x0, x1, x2, x3, x4;
} ascon_state_t;

static inline uint64_t ROR64(uint64_t x, int n) {
  return (x >> n) | (x << (64 - n));
}

static inline void ascon_permutation(ascon_state_t *s, int rounds) {
  for (int i = 12 - rounds; i < 12; ++i) {
    s->x0 ^= (uint64_t)(0xf0u - i * 0x10u) << 32 | (uint64_t)(0x0fu + i);
    s->x0 ^= s->x4; s->x4 ^= s->x3; s->x2 ^= s->x1;
    uint64_t t0 = s->x0; uint64_t t1 = s->x1; uint64_t t2 = s->x2; uint64_t t3 = s->x3; uint64_t t4 = s->x4;
    t0 =~ t0; t1 =~ t1; t2 =~ t2; t3 =~ t3; t4 =~ t4;
    t0 &= s->x1; t1 &= s->x2; t2 &= s->x3; t3 &= s->x4; t4 &= s->x0;
    s->x0 ^= t1; s->x1 ^= t2; s->x2 ^= t3; s->x3 ^= t4; s->x4 ^= t0;
    s->x1 ^= s->x0; s->x0 ^= s->x4; s->x3 ^= s->x2; s->x2 =~ s->x2;
    s->x0 ^= ROR64(s->x0, 19) ^ ROR64(s->x0, 28);
    s->x1 ^= ROR64(s->x1, 61) ^ ROR64(s->x1, 39);
    s->x2 ^= ROR64(s->x2, 1) ^ ROR64(s->x2, 6);
    s->x3 ^= ROR64(s->x3, 10) ^ ROR64(s->x3, 17);
    s->x4 ^= ROR64(s->x4, 7) ^ ROR64(s->x4, 41);
  }
}

static inline uint64_t load64(const uint8_t *p) {
  uint64_t x = 0;
  for (int i = 0; i < 8; ++i) x = (x << 8) | p[i];
  return x;
}

static inline void store64(uint8_t *p, uint64_t x) {
  for (int i = 7; i >= 0; --i) { p[i] = (uint8_t)x; x >>= 8; }
}

int ascon128_encrypt(uint8_t *t, uint8_t *c, const uint8_t *k, const uint8_t *n, const uint8_t *a, size_t alen, const uint8_t *p, size_t plen) {
  ascon_state_t s;
  uint64_t k0 = load64(k);
  uint64_t k1 = load64(k + 8);
  uint64_t n0 = load64(n);
  uint64_t n1 = load64(n + 8);
  s.x0 = 0x80400c0600000000ULL;
  s.x1 = k0; s.x2 = k1; s.x3 = n0; s.x4 = n1;
  ascon_permutation(&s, 12);
  s.x3 ^= k0; s.x4 ^= k1;
  if (alen) {
    while (alen >= 8) {
      s.x0 ^= load64(a);
      ascon_permutation(&s, 6);
      a += 8; alen -= 8;
    }
    for (int i = 0; i < (int)alen; ++i) s.x0 ^= (uint64_t)a[i] << (56 - i * 8);
    s.x0 ^= (uint64_t)0x80 << (56 - alen * 8);
    ascon_permutation(&s, 6);
  }
  s.x4 ^= 1;
  while (plen >= 8) {
    s.x0 ^= load64(p);
    store64(c, s.x0);
    ascon_permutation(&s, 6);
    p += 8; c += 8; plen -= 8;
  }
  for (int i = 0; i < (int)plen; ++i) s.x0 ^= (uint64_t)p[i] << (56 - i * 8);
  s.x0 ^= (uint64_t)0x80 << (56 - plen * 8);
  for (int i = 0; i < (int)plen; ++i) c[i] = (uint8_t)(s.x0 >> (56 - i * 8));
  s.x1 ^= k0; s.x2 ^= k1;
  ascon_permutation(&s, 12);
  s.x3 ^= k0; s.x4 ^= k1;
  store64(t, s.x3);
  store64(t + 8, s.x4);
  return 0;
}

int ascon128_decrypt(uint8_t *p, const uint8_t *k, const uint8_t *n, const uint8_t *a, size_t alen, const uint8_t *c, size_t clen, const uint8_t *t) {
  ascon_state_t s;
  uint64_t k0 = load64(k);
  uint64_t k1 = load64(k + 8);
  uint64_t n0 = load64(n);
  uint64_t n1 = load64(n + 8);
  s.x0 = 0x80400c0600000000ULL;
  s.x1 = k0; s.x2 = k1; s.x3 = n0; s.x4 = n1;
  ascon_permutation(&s, 12);
  s.x3 ^= k0; s.x4 ^= k1;
  if (alen) {
    while (alen >= 8) {
      s.x0 ^= load64(a);
      ascon_permutation(&s, 6);
      a += 8; alen -= 8;
    }
    for (int i = 0; i < (int)alen; ++i) s.x0 ^= (uint64_t)a[i] << (56 - i * 8);
    s.x0 ^= (uint64_t)0x80 << (56 - alen * 8);
    ascon_permutation(&s, 6);
  }
  s.x4 ^= 1;
  while (clen >= 8) {
    uint64_t cx = load64(c);
    store64(p, s.x0 ^ cx);
    s.x0 = cx;
    ascon_permutation(&s, 6);
    p += 8; c += 8; clen -= 8;
  }
  for (int i = 0; i < (int)clen; ++i) {
    uint8_t ci = c[i];
    p[i] = (uint8_t)(s.x0 >> (56 - i * 8)) ^ ci;
    s.x0 &= ~((uint64_t)0xff << (56 - i * 8));
    s.x0 |= (uint64_t)ci << (56 - i * 8);
  }
  s.x0 ^= (uint64_t)0x80 << (56 - clen * 8);
  s.x1 ^= k0; s.x2 ^= k1;
  ascon_permutation(&s, 12);
  s.x3 ^= k0; s.x4 ^= k1;
  if (load64(t) != s.x3 || load64(t + 8) != s.x4) return -1;
  return 0;
}
