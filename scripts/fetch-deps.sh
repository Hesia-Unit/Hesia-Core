#!/usr/bin/env bash
# Fetch and verify the third-party sources used by the provider-enabled build.
# Archives land in third_party/downloads/, sources in third_party/src/.
# Versions, URLs and SHA-256 digests must match third_party/dependencies.lock.json.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
THIRD_PARTY="${ROOT}/third_party"
DOWNLOADS="${THIRD_PARTY}/downloads"
SOURCES="${THIRD_PARTY}/src"

OPENSSL_VERSION="3.5.6"
OPENSSL_URL="https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz"
OPENSSL_SHA256="deae7c80cba99c4b4f940ecadb3c3338b13cb77418409238e57d7f31f2a3b736"

LIBOQS_VERSION="0.15.0"
LIBOQS_URL="https://github.com/open-quantum-safe/liboqs/archive/refs/tags/${LIBOQS_VERSION}.tar.gz"
LIBOQS_SHA256="3983f7cd1247f37fb76a040e6fd684894d44a84cecdcfbdb90559b3216684b5c"

need() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing required command: $1" >&2
    exit 1
  }
}

sha256_of() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | cut -d' ' -f1
  else
    shasum -a 256 "$1" | cut -d' ' -f1
  fi
}

fetch() {
  local url="$1"
  local out="$2"
  local expected="$3"
  mkdir -p "$(dirname "$out")"
  if [ ! -f "$out" ]; then
    echo "Downloading ${url}"
    curl -L --fail --retry 3 "$url" -o "${out}.part"
    mv "${out}.part" "$out"
  fi
  local actual
  actual="$(sha256_of "$out")"
  if [ "$actual" != "$expected" ]; then
    echo "SHA-256 mismatch for ${out}" >&2
    echo "  expected ${expected}" >&2
    echo "  actual   ${actual}" >&2
    echo "Delete the file and run this script again." >&2
    exit 1
  fi
}

extract() {
  local archive="$1"
  local dest="$2"
  if [ ! -f "$dest/.hesia-extracted" ]; then
    rm -rf "$dest"
    mkdir -p "$dest"
    tar -xzf "$archive" -C "$dest" --strip-components 1
    touch "$dest/.hesia-extracted"
  fi
}

need curl
need tar
if ! command -v sha256sum >/dev/null 2>&1; then
  need shasum
fi

OPENSSL_ARCHIVE="${DOWNLOADS}/openssl-${OPENSSL_VERSION}.tar.gz"
fetch "$OPENSSL_URL" "$OPENSSL_ARCHIVE" "$OPENSSL_SHA256"
extract "$OPENSSL_ARCHIVE" "${SOURCES}/openssl-${OPENSSL_VERSION}"

LIBOQS_ARCHIVE="${DOWNLOADS}/liboqs-${LIBOQS_VERSION}.tar.gz"
fetch "$LIBOQS_URL" "$LIBOQS_ARCHIVE" "$LIBOQS_SHA256"
extract "$LIBOQS_ARCHIVE" "${SOURCES}/liboqs-${LIBOQS_VERSION}"

echo "Third-party sources ready under ${SOURCES}"
