# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Fixed

- The native Ascon-128 added the round constant to the wrong state word, so its output
  was not Ascon-128. It now matches all 1,089 official v1.2 test vectors, which `ctest`
  and `make test` run.
- Ascon-128 decryption compares the tag in constant time and zeroes the plaintext
  buffer when the tag does not match.
- The `hesia_sig_*` functions cast the signature algorithm to a PQC algorithm, but the two
  enums number ML-DSA differently: `HESIA_SIG_ML_DSA_65` reached ML-KEM-1024 and
  `HESIA_SIG_ML_DSA_87` reached ML-DSA-65. They now map explicitly and reject other values.
- The Makefile and the portable build commands in the README now compile
  `src/algorithms/ascon.c`; linking failed without it.

### Removed

- `HESIA_SIG_ED25519` and the native Ed25519 code. It was a placeholder: keys were derived
  by XOR, signatures were constant bytes, and verification accepted every signature.
  Value `1` of `hesia_sig_algorithm_t` stays reserved, and `hesia-cli keygen ed25519` is gone.

## [0.1.0] - 2026-05-01

### Added

- initial public release of the portable C core
- optional OpenSSL and liboqs provider surfaces
- C++, Python, Rust, Swift, and Ada bindings
- unit tests and minimal examples
- bilingual reference documentation

