#!/bin/sh
# Build the dlopen fixture modules.
#
#   sh tests/support/dl/build.sh              # native (amd64)
#   sh tests/support/dl/build.sh i386
#   sh tests/support/dl/build.sh arm
#   sh tests/support/dl/build.sh arm64
#
# duck cannot emit a shared object yet (tools/src/recipes/gnu/config.hh:305, "TODO: implement shared
# outs"), so this is a plain sh script rather than a .duck manifest. The suites that need these
# fixtures skip with a printed reason when they are absent, so a tree without them still grades PASS.
#
# The chain is top -> mid -> leaf, wired with -Wl,-soname and a real DT_NEEDED, plus a dependency-free
# `solo`. Built -O0: the tests assert on constructor ordering and cross-module calls, and there is
# nothing here worth letting the optimiser inline away.

set -e
here=$(cd "$(dirname "$0")" && pwd)
arch=${1:-x64}

case "$arch" in
  x64|amd64|x86_64) CC="gcc"; FLAGS="-m64"; out="$here/x64" ;;
  i386|x86)         CC="gcc"; FLAGS="-m32"; out="$here/i386" ;;
  arm|arm32)        CC="/usr/gcc-linaro/bin/arm-none-linux-gnueabihf-gcc"; FLAGS=""; out="$here/arm" ;;
  arm64|aarch64)    CC="/usr/gcc-linaro-aarch64/bin/aarch64-none-linux-gnu-gcc"; FLAGS=""; out="$here/arm64" ;;
  *) echo "unknown arch '$arch' (x64|i386|arm|arm64)" >&2; exit 2 ;;
esac

if ! command -v "$CC" >/dev/null 2>&1 && [ ! -x "$CC" ]; then
  echo "skip: no compiler for $arch ($CC)" >&2
  exit 0
fi

mkdir -p "$out"
common="-shared -fPIC -O0 -g0 -nostdlib -Wl,--no-undefined"

# leaf: no dependencies of its own
$CC $FLAGS $common -Wl,-soname,libmc_dl_leaf.so.1 -o "$out/libmc_dl_leaf.so.1" "$here/dl_leaf.c"

# solo: likewise, but nothing depends on it either
$CC $FLAGS $common -Wl,-soname,libmc_dl_solo.so.1 -o "$out/libmc_dl_solo.so.1" "$here/dl_solo.c"

# mid NEEDs leaf, top NEEDs mid. -rpath '$ORIGIN' so the chain resolves out of this directory
# without touching the system search paths.
$CC $FLAGS $common -Wl,-soname,libmc_dl_mid.so.1 -Wl,-rpath,'$ORIGIN' \
    -o "$out/libmc_dl_mid.so.1" "$here/dl_mid.c" "$out/libmc_dl_leaf.so.1"

$CC $FLAGS $common -Wl,-soname,libmc_dl_top.so.1 -Wl,-rpath,'$ORIGIN' \
    -o "$out/libmc_dl_top.so.1" "$here/dl_top.c" "$out/libmc_dl_mid.so.1"

# dia depends on BOTH leaf and mid, and names leaf first. The link order is the test: it puts the
# shared child ahead of a sibling that needs it in any parent-first teardown walk.
$CC $FLAGS $common -Wl,-soname,libmc_dl_dia.so.1 -Wl,-rpath,'$ORIGIN' \
    -o "$out/libmc_dl_dia.so.1" "$here/dl_dia.c" "$out/libmc_dl_leaf.so.1" "$out/libmc_dl_mid.so.1"

# void: exports NOTHING (local.map hides every definition) but imports four symbols from leaf, so
# its .gnu.hash has one empty bucket and any symcount derived by walking the chains comes out as
# symoffset instead of the real .dynsym length -- which makes every symbolic relocation look out of
# range and fails the load under reloc_mode_t::strict. This is the librxe-rdmav59.so shape,
# reproduced without needing libibverbs to be installed.
#
# --hash-style=gnu is MANDATORY, not decoration: the Linaro arm/arm64 toolchains default to 'both',
# and a present DT_HASH sends count_dynsyms down the nchain path where the bug does not appear. The
# fixture would then pass on two of the four targets for the wrong reason.
$CC $FLAGS $common -Wl,--hash-style=gnu -Wl,--version-script,"$here/local.map" \
    -Wl,-soname,libmc_dl_void.so.1 -Wl,-rpath,'$ORIGIN' \
    -o "$out/libmc_dl_void.so.1" "$here/dl_void.c" "$out/libmc_dl_leaf.so.1"

# the fixture is worthless if the toolchain exported something after all, so assert the shape here
# rather than letting the suite pass vacuously
# only GLOBAL/WEAK definitions matter: STT_SECTION entries are LOCAL, live below symoffset and
# never populate a gnu-hash bucket, and the arm/arm64 toolchains emit two of them
if readelf --dyn-syms -W "$out/libmc_dl_void.so.1" \
     | awk 'NR>3 && NF>=8 && ($5=="GLOBAL"||$5=="WEAK") && $7!="UND"{n++} END{exit !n}'; then
  echo "error: libmc_dl_void.so.1 exports a defined symbol; --version-script did not take" >&2
  exit 1
fi
if ! readelf -dW "$out/libmc_dl_void.so.1" | grep -q GNU_HASH; then
  echo "error: libmc_dl_void.so.1 has no DT_GNU_HASH" >&2; exit 1
fi
if readelf -dW "$out/libmc_dl_void.so.1" | grep -qE '\(HASH\)'; then
  echo "error: libmc_dl_void.so.1 also has DT_HASH; --hash-style=gnu did not take" >&2; exit 1
fi

echo "built $arch fixtures in $out:"
for f in "$out"/*.so.1; do
  printf '  %-28s ' "$(basename "$f")"
  readelf -dW "$f" 2>/dev/null | grep -oE 'NEEDED.*\[[^]]*\]|SONAME.*\[[^]]*\]' | tr '\n' ' '
  echo
done
