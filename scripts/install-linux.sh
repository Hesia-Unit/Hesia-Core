#!/usr/bin/env bash
set -euo pipefail

PREFIX="/usr/local"
URL=""
TARBALL=""

while [ $# -gt 0 ]; do
  case "$1" in
    --prefix)
      PREFIX="$2"
      shift 2
      ;;
    --url)
      URL="$2"
      shift 2
      ;;
    --tarball)
      TARBALL="$2"
      shift 2
      ;;
    *)
      echo "Usage: $0 [--prefix /usr/local] [--url https://...tar.gz | --tarball file.tar.gz]" >&2
      exit 2
      ;;
  esac
done

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ -n "$URL" ]; then
  command -v curl >/dev/null 2>&1 || { echo "curl is required" >&2; exit 1; }
  TARBALL="${TMP}/hesia-core.tar.gz"
  curl -L --fail "$URL" -o "$TARBALL"
elif [ -z "$TARBALL" ]; then
  ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
  "${ROOT}/scripts/package-linux.sh"
  ARCH="$(uname -m)"
  TARBALL="${ROOT}/dist/hesia-core-0.1.0-linux-${ARCH}.tar.gz"
fi

tar -xzf "$TARBALL" -C "$TMP"
PKG_ROOT="$(find "$TMP" -maxdepth 1 -type d -name 'hesia-core-*' | head -n 1)"
if [ -z "$PKG_ROOT" ]; then
  echo "Invalid HESIA CoreLib package" >&2
  exit 1
fi

install -d "$PREFIX/include" "$PREFIX/lib" "$PREFIX/share/hesia-core"
cp -R "$PKG_ROOT/include/hesia" "$PREFIX/include/"
cp "$PKG_ROOT/lib/"* "$PREFIX/lib/"
cp -R "$PKG_ROOT/docs" "$PREFIX/share/hesia-core/"
cp -R "$PKG_ROOT/bindings" "$PREFIX/share/hesia-core/"
cp "$PKG_ROOT/README.md" "$PKG_ROOT/LICENSE" "$PKG_ROOT/NOTICE" "$PKG_ROOT/SECURITY.md" "$PREFIX/share/hesia-core/"

if command -v ldconfig >/dev/null 2>&1 && [ "$(id -u)" -eq 0 ]; then
  ldconfig
fi

echo "HESIA CoreLib installed under ${PREFIX}"
