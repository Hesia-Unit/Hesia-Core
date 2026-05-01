# HESIA CoreLib

HESIA CoreLib is the open source systems library for HESIA-style embedded security and inference-control software.

It provides:

- a stable C ABI for long-lived firmware and host integrations
- optional OpenSSL-backed AES-256-GCM and OS random bytes
- optional liboqs-backed ML-KEM and ML-DSA post-quantum operations
- fail-closed error handling when a provider is not compiled in
- watchdog and inference-guard primitives for embedded AI pipelines
- language surfaces for C, C++, Python, Rust, Swift, and Ada

The library does not invent cryptographic primitives. Symmetric crypto is delegated to OpenSSL and post-quantum crypto is delegated to liboqs. Builds without these providers keep the API available but return `HESIA_STATUS_UNSUPPORTED` for the unavailable cryptographic operations.

## Layout

- `include/hesia/core.h`: stable C ABI.
- `include/hesia/core.hpp`: header-only C++ wrapper.
- `src/hesia_core.c`: portable implementation.
- `bindings/`: Python, Rust, Swift, and Ada surfaces.
- `examples/`: minimal C and Python examples.
- `tests/`: portable unit tests.

## Build

Portable baseline:

```sh
cc -Iinclude -c src/hesia_core.c -o hesia_core.o
ar rcs libhesia_core.a hesia_core.o
cc -Iinclude tests/test_core.c libhesia_core.a -o test_core
./test_core
```

With CMake:

```sh
cmake -S . -B build -DHESIA_CORE_WITH_OPENSSL=ON -DHESIA_CORE_WITH_LIBOQS=ON
cmake --build build
ctest --test-dir build
```

## Documentation

The project-level bilingual handover documents live in:

- `../../docs/HESIA_CORELIB_EN.md`
- `../../docs/HESIA_CORELIB_FR.md`
- `../../docs/HESIA_CORELIB_EN.pdf`
- `../../docs/HESIA_CORELIB_FR.pdf`

## Security posture

HESIA CoreLib is an integration layer, not a replacement for formal cryptographic review. Production use must pin provider versions, run the test suite on the target platform, and validate policy behavior under the exact deployment profile.
