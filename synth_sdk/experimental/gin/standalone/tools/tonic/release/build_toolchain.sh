#!/usr/bin/env bash
# GNU binutils2.38/GCC8.5.0 bfin-elf toolchain with verified source tarballs.
set -euo pipefail

WS="${GIN_BUILD_ROOT:-/workspace}"
PREFIX="${BFIN_PREFIX:-$WS/.toolchain}"
CACHE="${BFIN_CACHE:-$WS/.cache/bfin-src}"
LOGS="$WS/local-results/logs"
JOBS="${JOBS:-$(nproc)}"
TARGET="bfin-elf"

BINUTILS_VERSION="2.38"
BINUTILS_SHA256="e316477a914f567eccc34d5d29785b8b0f5a10208d36bbacedcc39048ecfe024"
GCC_VERSION="8.5.0"
GCC_SHA256="d308841a511bb830a6100397b0042db24ce11f642dab6ea6ee44842e5325ed50"

mkdir -p "$PREFIX" "$CACHE" "$LOGS"
export PATH="$PREFIX/bin:$PATH"

fetch() {
  local url="$1" out="$2" sha="$3"
  if [[ ! -f "$out" ]]; then
    echo "fetching $url"
    curl -fL --retry 4 --retry-delay 2 -o "$out.tmp" "$url"
    mv "$out.tmp" "$out"
  fi
  echo "$sha  $out" | sha256sum -c -
}

fetch "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz" \
      "$CACHE/binutils-${BINUTILS_VERSION}.tar.xz" "$BINUTILS_SHA256"
fetch "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz" \
      "$CACHE/gcc-${GCC_VERSION}.tar.xz" "$GCC_SHA256"

BINUTILS_CONFIG=(--target="$TARGET" --prefix="$PREFIX"
  --disable-nls --disable-werror --disable-gdb --disable-sim)
GCC_CONFIG=(--target="$TARGET" --prefix="$PREFIX"
  --enable-languages=c --without-headers --with-newlib
  --disable-nls --disable-shared --disable-threads
  --disable-libssp --disable-libquadmath --disable-libgomp
  --disable-libatomic --disable-multilib)

if [[ ! -x "$PREFIX/bin/${TARGET}-as" ]]; then
  rm -rf "$CACHE/binutils-${BINUTILS_VERSION}" "$CACHE/build-binutils"
  tar -C "$CACHE" -xf "$CACHE/binutils-${BINUTILS_VERSION}.tar.xz"
  mkdir -p "$CACHE/build-binutils"
  pushd "$CACHE/build-binutils" >/dev/null
  ../binutils-${BINUTILS_VERSION}/configure "${BINUTILS_CONFIG[@]}"
  make -j"$JOBS"
  make install
  popd >/dev/null
fi

if [[ ! -x "$PREFIX/bin/${TARGET}-gcc" ]]; then
  rm -rf "$CACHE/gcc-${GCC_VERSION}" "$CACHE/build-gcc"
  tar -C "$CACHE" -xf "$CACHE/gcc-${GCC_VERSION}.tar.xz"
  mkdir -p "$CACHE/build-gcc"
  pushd "$CACHE/build-gcc" >/dev/null
  ../gcc-${GCC_VERSION}/configure "${GCC_CONFIG[@]}"
  make -j"$JOBS" all-gcc
  make install-gcc
  make -j"$JOBS" all-target-libgcc
  make install-target-libgcc
  popd >/dev/null
fi

for t in as ld objdump readelf size nm gcc objcopy; do
  command -v "$TARGET-$t" >/dev/null || { echo "missing $TARGET-$t" >&2; exit 1; }
done

OUT="$WS/local-results/toolchain-versions.txt"
{
  echo "# GIN Blackfin toolchain ($(date -u +%Y-%m-%dT%H:%M:%SZ))"
  echo "target triplet: $("$TARGET-gcc" -dumpmachine)"
  echo "binutils source: binutils-${BINUTILS_VERSION}.tar.xz sha256=${BINUTILS_SHA256}"
  echo "gcc source:      gcc-${GCC_VERSION}.tar.xz sha256=${GCC_SHA256}"
  echo "binutils configure: ${BINUTILS_CONFIG[*]}"
  echo "gcc configure:      ${GCC_CONFIG[*]}"
  echo
  for t in gcc as ld objdump readelf nm size objcopy; do
    echo "== $TARGET-$t --version"; "$TARGET-$t" --version | head -n 1
  done
  echo
  echo "== $TARGET-gcc -v"; "$TARGET-gcc" -v 2>&1
  echo
  echo "== libgcc: $("$TARGET-gcc" -print-libgcc-file-name)"
  echo
  echo "== GCC default CPU (NOT used by GIN; every build passes -mcpu explicitly)"
  echo 'int x;' | "$TARGET-gcc" -E -dM -x c - | grep -E '__ADSPBF|__SILICON|__WORKAROUND|__bfin__|__BFIN' | sort
  echo
  echo "== Predefines with -mcpu=bf524-any (compiler capability reference)"
  echo 'int x;' | "$TARGET-gcc" -mcpu=bf524-any -E -dM -x c - | grep -E '__ADSPBF|__SILICON|__WORKAROUND|__bfin__|__BFIN' | sort
  echo
  echo "== GCC target options with -mcpu=bf524-any"
  "$TARGET-gcc" -mcpu=bf524-any -Q --help=target 2>&1
  echo
  echo "== Assembler Blackfin options"
  "$TARGET-as" --help 2>&1 | sed -n '/Blackfin/,$p'
} > "$OUT"
cat "$OUT" | head -n 20
echo "Blackfin toolchain ready at $PREFIX"
