#!/usr/bin/env bash
# Builds weapon-forge.so (32-bit) for Linux servers.
# Requires CMake >= 3.15, git and a C++17 compiler with 32-bit support
# (Debian/Ubuntu: sudo apt install cmake git g++-multilib).
#
#   ./build.sh            -> dist/plugins/weapon-forge.so (+ dist/components/)
#   VERSION=1.2.0 ./build.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="${VERSION:-$(head -n1 "$ROOT/VERSION" | tr -d '[:space:]')}"
BUILD="$ROOT/build/linux"
OUT="$ROOT/dist"

echo "==> Configuring weapon-forge $VERSION"
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DWF_VERSION="$VERSION"

echo "==> Building"
cmake --build "$BUILD" --parallel

mkdir -p "$OUT/plugins" "$OUT/components" "$OUT/include"
cp "$BUILD/weapon-forge.so" "$OUT/plugins/weapon-forge.so"
cp "$BUILD/weapon-forge.so" "$OUT/components/weapon-forge.so"
cp "$ROOT/include/weapon-forge.inc" "$OUT/include/weapon-forge.inc"

echo "==> Done: $OUT/plugins/weapon-forge.so (SA-MP) / $OUT/components/weapon-forge.so (open.mp)"
