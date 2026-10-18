#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
THIRD_PARTY="${ROOT}/third_party"
SOURCES="${THIRD_PARTY}/src"
INSTALL="${THIRD_PARTY}/install/linux-$(uname -m)"
OPENSSL_VERSION="3.5.6"
LIBOQS_VERSION="0.15.0"

jobs_count() {
  if command -v nproc >/dev/null 2>&1; then nproc; else echo 2; fi
}

need() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing required command: $1" >&2
    exit 1
  }
}

need cc
need make
need perl
need cmake

bash "${ROOT}/scripts/fetch-deps.sh"

OPENSSL_SOURCE="${SOURCES}/openssl-${OPENSSL_VERSION}"
OPENSSL_PREFIX="${INSTALL}/openssl"
if [ ! -f "${OPENSSL_PREFIX}/lib/libcrypto.a" ] && [ ! -f "${OPENSSL_PREFIX}/lib64/libcrypto.a" ]; then
  (
    cd "$OPENSSL_SOURCE"
    ./Configure linux-generic64 no-shared no-tests no-module --prefix="$OPENSSL_PREFIX" --openssldir="${OPENSSL_PREFIX}/ssl"
    make -j"$(jobs_count)"
    make install_sw
  )
fi

LIBOQS_SOURCE="${SOURCES}/liboqs-${LIBOQS_VERSION}"
LIBOQS_BUILD="${THIRD_PARTY}/build/linux-$(uname -m)/liboqs"
LIBOQS_PREFIX="${INSTALL}/liboqs"
if [ ! -f "${LIBOQS_PREFIX}/lib/liboqs.a" ]; then
  rm -rf "$LIBOQS_BUILD"
  cmake -S "$LIBOQS_SOURCE" -B "$LIBOQS_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$LIBOQS_PREFIX" \
    -DBUILD_SHARED_LIBS=OFF \
    -DOQS_BUILD_ONLY_LIB=ON \
    -DOQS_USE_OPENSSL=OFF \
    -DOQS_MINIMAL_BUILD="KEM_ml_kem_768;KEM_ml_kem_1024;SIG_ml_dsa_65;SIG_ml_dsa_87"
  cmake --build "$LIBOQS_BUILD" --parallel
  cmake --install "$LIBOQS_BUILD"
fi

echo "Dependency install root: ${INSTALL}"
