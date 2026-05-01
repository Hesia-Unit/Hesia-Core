# HESIA CoreLib

[![CI](https://github.com/Hesia-Unit/Hesia-Core/actions/workflows/ci.yml/badge.svg)](https://github.com/Hesia-Unit/Hesia-Core/actions/workflows/ci.yml)
[![License](https://img.shields.io/github/license/Hesia-Unit/Hesia-Core)](LICENSE)
[![Tag](https://img.shields.io/github/v/tag/Hesia-Unit/Hesia-Core)](https://github.com/Hesia-Unit/Hesia-Core/tags)

HESIA CoreLib is a small open source systems library for embedded security and inference-control software.

It provides:

- a stable C ABI for long-lived firmware and host integrations
- optional OpenSSL-backed AES-256-GCM and OS random bytes
- optional liboqs-backed ML-KEM and ML-DSA post-quantum operations
- fail-closed error handling when a provider is not compiled in
- watchdog and inference-guard primitives for embedded AI pipelines
- language surfaces for C, C++, Python, Rust, Swift, and Ada

The library does not invent cryptographic primitives. Symmetric crypto is delegated to OpenSSL and post-quantum crypto is delegated to liboqs. Builds without these providers keep the API available but return `HESIA_STATUS_UNSUPPORTED` for unavailable cryptographic operations.

## Why this repository exists

CoreLib is the public engineering surface for reusable HESIA-style primitives. It is intentionally smaller than the private product runtime and focuses on:

- stable interfaces
- explicit failure semantics
- provider-backed cryptography
- auditable, portable code
- small embedded AI safety primitives

## Repository layout

- `include/hesia/core.h`: stable C ABI
- `include/hesia/core.hpp`: header-only C++ wrapper
- `src/hesia_core.c`: portable implementation
- `bindings/`: Python, Rust, Swift, and Ada surfaces
- `examples/`: minimal C and Python examples
- `tests/`: portable unit tests
- `docs/`: English and French project reference material

## Quick start

Portable baseline:

```sh
cc -Iinclude -c src/hesia_core.c -o hesia_core.o
ar rcs libhesia_core.a hesia_core.o
cc -Iinclude tests/test_core.c libhesia_core.a -o test_core
./test_core
```

With CMake:

```sh
cmake -S . -B build -DHESIA_CORE_BUILD_SHARED=OFF -DHESIA_CORE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Enable optional providers only when they are installed on the build host:

```sh
cmake -S . -B build \
  -DHESIA_CORE_WITH_OPENSSL=ON \
  -DHESIA_CORE_WITH_LIBOQS=ON
```

## Documentation

- [Open Source Reference EN](docs/HESIA_CORELIB_EN.md)
- [Open Source Reference FR](docs/HESIA_CORELIB_FR.md)
- [Roadmap](ROADMAP.md)
- [Changelog](CHANGELOG.md)

## Security posture

HESIA CoreLib is an integration layer, not a replacement for formal cryptographic review. Production use should:

- pin provider versions
- run the test suite on the real target platform
- validate policy behavior under the exact deployment profile
- review optional provider enablement as part of the release process

## Project status

The repository is in early public release. Interfaces are designed to stay small and stable, but maintainers may still refine packaging, bindings, and optional provider coverage during the `0.x` cycle.
