# HESIA CoreLib (v0.2.0)

HESIA CoreLib is the open source systems library for HESIA-style embedded security, inference-control, and post-quantum cryptography.

## Features

- **Stable C ABI**: Long-lived firmware and host integrations.
- **Post-Quantum Cryptography**: ML-KEM and ML-DSA via `liboqs`.
- **Lightweight Cryptography**: Native, high-performance ASCON-128 and Ed25519 for embedded systems.
- **Maintenance & Audit**: Built-in APIs for firmware health, tamper detection, and security metadata.
- **Embedded Guardrails**: Watchdog and Inference-Guard primitives for AI-enabled robotics.
- **ARM Optimized**: Explicit support for ARM Aarch64 and Cortex-M targets.
- **HESIA CLI**: Tooling for maintenance, project scaffolding, and key management.

## Project Structure

- `include/hesia/core.h`: Unified security API.
- `src/algorithms/`: Native implementations of lightweight primitives.
- `tools/hesia-cli/`: Maintenance and developer utility.
- `scripts/`: Cross-compilation and project initialization helpers.

## Getting Started

### Initialize a new PQC project

```bash
bash scripts/init-project.sh my-pqc-app ./my-pqc-app
```

### Build for ARM64

```bash
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=scripts/arm64-toolchain.cmake
cmake --build build-arm
```

### Use the HESIA CLI

```bash
# Get system maintenance info
hesia-cli info

# Generate post-quantum keys
hesia-cli keygen mldsa65
```

## Security Posture

HESIA CoreLib prioritizes fail-closed behavior. When hardware acceleration or providers are missing, it falls back to native, constant-time implementations optimized for embedded trust.

## Layout

- `include/hesia/core.h`: stable C ABI.
- `include/hesia/core.hpp`: header-only C++ wrapper.
- `src/hesia_core.c`: portable implementation.
- `bindings/`: Python, Rust, Swift, and Ada surfaces.
- `examples/`: minimal C and Python examples.
- `tests/`: portable unit tests.
- `third_party/dependencies.lock.json`: pinned OpenSSL, liboqs, CMake, and WiX versions and SHA-256 digests. Everything else under `third_party/` is fetched, never committed.
- `scripts/`: native Windows, Linux, MSI, and tarball packaging workflows.

## Build

### Third-party dependencies

OpenSSL, liboqs, and the Windows build tools are not committed. Their versions, download URLs, and SHA-256 digests are pinned in `third_party/dependencies.lock.json`. The fetch scripts download them, check the digests, and extract them under `third_party/`, which git ignores:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\fetch-deps.ps1
```

```sh
bash scripts/fetch-deps.sh
```

`build-deps-windows.ps1` and `build-deps-linux.sh` call these scripts themselves, so run them on their own only to prepare the sources ahead of time.

### Build commands

Provider-enabled Windows build:

```powershell
cd open_source\hesia-core
powershell -ExecutionPolicy Bypass -File scripts\build-deps-windows.ps1
powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -SkipDeps
powershell -ExecutionPolicy Bypass -File scripts\package-msi.ps1 -SkipBuild
```

Provider-enabled Linux build:

```sh
cd open_source/hesia-core
bash scripts/build-linux.sh
bash scripts/package-linux.sh
sudo bash scripts/install-linux.sh --prefix /usr/local --tarball dist/hesia-core-0.1.0-linux-$(uname -m).tar.gz
```

Portable provider-free baseline:

```sh
cc -Iinclude -c src/hesia_core.c -o hesia_core.o
ar rcs libhesia_core.a hesia_core.o
cc -Iinclude tests/test_core.c libhesia_core.a -o test_core
./test_core
```

With CMake:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Disable providers explicitly for constrained smoke tests:

```sh
cmake -S . -B build-lite -DHESIA_CORE_WITH_OPENSSL=OFF -DHESIA_CORE_WITH_LIBOQS=OFF
```

## Documentation

The project-level bilingual handover documents live in:

- `../../docs/HESIA_CORELIB_EN.md`
- `../../docs/HESIA_CORELIB_FR.md`
- `../../docs/HESIA_CORELIB_EN.pdf`
- `../../docs/HESIA_CORELIB_FR.pdf`

## Security posture

HESIA CoreLib is an integration layer, not a replacement for formal cryptographic review. Production use must keep provider versions pinned, run the provider test suite on the target platform, and validate policy behavior under the exact deployment profile.
