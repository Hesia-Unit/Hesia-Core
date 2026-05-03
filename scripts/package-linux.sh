#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="$(uname -m)"
VERSION="0.1.0"
BUILD="${ROOT}/build/linux-${ARCH}"
STAGE="${ROOT}/dist/linux/hesia-core-${VERSION}-linux-${ARCH}"

"${ROOT}/scripts/build-linux.sh"

rm -rf "$STAGE"
mkdir -p "$STAGE/bin" "$STAGE/lib" "$STAGE/include" "$STAGE/bindings" "$STAGE/docs" "$STAGE/third_party"
cp "${BUILD}/lib/libhesia_core.a" "$STAGE/lib/"
cp "${BUILD}/lib/libhesia_core.so" "$STAGE/lib/"
cp -R "${ROOT}/include/hesia" "$STAGE/include/"
cp -R "${ROOT}/bindings/python" "$STAGE/bindings/"
cp -R "${ROOT}/bindings/rust" "$STAGE/bindings/"
cp -R "${ROOT}/bindings/swift" "$STAGE/bindings/"
cp -R "${ROOT}/bindings/ada" "$STAGE/bindings/"
cp "${ROOT}/README.md" "${ROOT}/LICENSE" "${ROOT}/NOTICE" "${ROOT}/SECURITY.md" "$STAGE/"
cp "${ROOT}/third_party/dependencies.lock.json" "$STAGE/third_party/"
mkdir -p "$STAGE/third_party/install"
cp -R "${ROOT}/third_party/install/linux-${ARCH}" "$STAGE/third_party/install/"
cp "${ROOT}/../../docs/HESIA_CORELIB_EN.md" "${ROOT}/../../docs/HESIA_CORELIB_FR.md" "$STAGE/docs/"
cp "${ROOT}/../../docs/HESIA_CORELIB_EN.pdf" "${ROOT}/../../docs/HESIA_CORELIB_FR.pdf" "$STAGE/docs/"

TARBALL="${ROOT}/dist/hesia-core-${VERSION}-linux-${ARCH}.tar.gz"
tar -czf "$TARBALL" -C "$(dirname "$STAGE")" "$(basename "$STAGE")"
if command -v sha256sum >/dev/null 2>&1; then
  sha256sum "$TARBALL" > "${TARBALL}.sha256"
fi

echo "Linux package ready: ${TARBALL}"
