#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NO_PROVIDERS="${NO_PROVIDERS:-0}"

if [ "$NO_PROVIDERS" != "1" ]; then
  "${ROOT}/scripts/build-deps-linux.sh"
fi

ARCH="$(uname -m)"
BUILD="${ROOT}/build/linux-${ARCH}"
BIN="${BUILD}/bin"
LIB="${BUILD}/lib"
mkdir -p "$BIN" "$LIB"

CFLAGS=(-std=c11 -O2 -Wall -Wextra -Wpedantic -fPIC -Iinclude)
LIBS=()

if [ "$NO_PROVIDERS" != "1" ]; then
  DEPS="${ROOT}/third_party/install/linux-${ARCH}"
  OPENSSL="${DEPS}/openssl"
  LIBOQS="${DEPS}/liboqs"
  OPENSSL_LIB="${OPENSSL}/lib/libcrypto.a"
  if [ ! -f "$OPENSSL_LIB" ]; then
    OPENSSL_LIB="${OPENSSL}/lib64/libcrypto.a"
  fi
  CFLAGS+=(-DHESIA_CORE_WITH_OPENSSL -DHESIA_CORE_WITH_LIBOQS -I"${OPENSSL}/include" -I"${LIBOQS}/include")
  LIBS+=("${LIBOQS}/lib/liboqs.a" "$OPENSSL_LIB" -ldl -pthread)
fi

cc "${CFLAGS[@]}" -c src/hesia_core.c -o "${BUILD}/hesia_core.o"
ar rcs "${LIB}/libhesia_core.a" "${BUILD}/hesia_core.o"
cc "${CFLAGS[@]}" -shared src/hesia_core.c -o "${LIB}/libhesia_core.so" "${LIBS[@]}"
cc "${CFLAGS[@]}" tests/test_core.c src/hesia_core.c -o "${BIN}/test_core" "${LIBS[@]}"
"${BIN}/test_core"
if [ "$NO_PROVIDERS" != "1" ]; then
  cc "${CFLAGS[@]}" tests/test_providers.c src/hesia_core.c -o "${BIN}/test_providers" "${LIBS[@]}"
  "${BIN}/test_providers"
fi

echo "Linux build output: ${BUILD}"
