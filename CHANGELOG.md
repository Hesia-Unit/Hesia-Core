# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

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

