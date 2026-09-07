#!/bin/sh
# THE SECOND TRANSLATION UNIT, ON A BARE-METAL TARGET -- AND THE SET THAT IS ALLOWED TO COLLIDE.
#
# tests/build/errno_odr_second_tu.cpp already links two objects, but both of them include only
# <micron/array.hpp>, which is one of the three umbrellas in the tree that pulls neither
# memory/new.hpp nor hash/ nor string/format.hpp. So it never reached the symbols below.
#
# (E) makes this concrete rather than theoretical. examples/metal/README.md states that the board
# port is a separate TU ON PURPOSE -- weak defaults in one object, strong overrides in another, and
# the linker choosing between them is the entire override mechanism. A board that includes a
# container header in that second TU therefore has to link, and at HEAD it did not:
#
#     multiple definition of `micron::hash64(unsigned char const*, unsigned long, unsigned long)'
#     multiple definition of `micron::format::find(char const*, char const*, char)'
#
# Both were ordinary external-linkage definitions in headers -- the one non-template overload in a
# family of templates, twice. Both are `inline` now. The same class as errno.hpp's __micron_errno
# and except::__write_n, and found the same way: by linking two objects instead of compiling one.
#
# WHAT REMAINS IS DELIBERATE AND IS NAMED HERE. The twelve replacement operator new/delete forms in
# memory/new.hpp are strong definitions by design: that is what displaces libstdc++'s hosted, which
# ISSUES.md relies on. Making them inline would change that, so the constraint stands instead --
# A BOARD TU MUST NOT INCLUDE A CONTAINER, PRINT OR STRING HEADER. board_pc.cpp, board_virt.cpp and
# board_stm32.cpp each include only port/backends/__mport_abi.hpp, and start/kernel/mc_libgcc.cpp:33
# documents the identical constraint for a .ko.
#
# So the assertion is not "it links" -- it does not, and should not. It is that the collision set is
# EXACTLY the twelve. Anything else appearing here is a new non-inline definition in a header, which
# is the defect this file exists to catch; anything missing means someone changed the operators'
# linkage without changing ISSUES.md.
#
# usage: sh tests/build/metal_two_tu_link.sh   (from the repo root)
set -eu

CXX=${CXX:-g++}
OUT=${OUT:-bin/two_tu}
mkdir -p "$OUT"

FLAGS="-std=c++26 -O1 -I./src -I. -ffreestanding -nostdlib -nostdlib++ -fno-exceptions -fno-rtti
       -fno-stack-protector -fno-threadsafe-statics -fno-use-cxa-atexit -fno-common
       -fno-asynchronous-unwind-tables -fno-pie -mno-sse -mno-mmx -mno-80387 -mno-3dnow
       -mno-red-zone -DMICRON_PORT_METAL -DMICRON_NO_SIMD -DMICRON_NO_FP -DMICRON_NO_TLS
       -DMICRON_BB_PORT_POOL=1048576"

# the widest reach a board could plausibly have: containers, the printer, strings, hashing, sorting
cat > "$OUT/tu_a.cpp" <<'EOF'
#include <micron/maps.hpp>
#include <micron/print.hpp>
#include <micron/sort/sort.hpp>
#include <micron/strings.hpp>
#include <micron/vector.hpp>
extern "C" unsigned long tu_b_go(void);
extern "C" unsigned long tu_a_go(void)
{
  micron::vector<u32> v;
  v.push_back(1);
  micron::hopscotch_map<u64, u64> m;
  m[1] = 2;
  micron::string s{ "a" };
  s += "b";
  // odr-use the two that were broken, by name
  const byte b[4] = { 1, 2, 3, 4 };
  const char *t = "xy";
  return v.size() + m.size() + s.size() + micron::hash64(b, 4, 7)
         + static_cast<unsigned long>(micron::format::find(t, t + 2, 'y') != nullptr) + tu_b_go();
}
EOF
sed -e 's/tu_a_go/tu_c_go/; s/extern "C" unsigned long tu_b_go(void);//; s/+ tu_b_go()//' \
    -e 's/tu_c_go/tu_b_go/' "$OUT/tu_a.cpp" > "$OUT/tu_b.cpp"

# shellcheck disable=SC2086
$CXX $FLAGS -c "$OUT/tu_a.cpp" -o "$OUT/tu_a.o"
# shellcheck disable=SC2086
$CXX $FLAGS -c "$OUT/tu_b.cpp" -o "$OUT/tu_b.o"

# shellcheck disable=SC2086
$CXX $FLAGS -r -o "$OUT/joined.o" "$OUT/tu_a.o" "$OUT/tu_b.o" 2> "$OUT/link.err" || true

# LC_ALL=C on both sides. Without it `[` collates differently in the two lists and comm reports
# every bracketed name as both unexpected AND missing -- which reads exactly like a real failure.
got=$(sed -nE "s/.*multiple definition of \`([^']*)'.*/\1/p" "$OUT/link.err" | LC_ALL=C sort -u)

want=$(cat <<'EOF'
operator delete(void*)
operator delete(void*, std::align_val_t)
operator delete(void*, unsigned long)
operator delete(void*, unsigned long, std::align_val_t)
operator delete[](void*)
operator delete[](void*, std::align_val_t)
operator delete[](void*, unsigned long)
operator delete[](void*, unsigned long, std::align_val_t)
operator new(unsigned long)
operator new(unsigned long, std::align_val_t)
operator new[](unsigned long)
operator new[](unsigned long, std::align_val_t)
EOF
)
want=$(printf '%s\n' "$want" | LC_ALL=C sort -u)

if [ "$got" = "$want" ]; then
  echo "  ok    the two-TU collision set is exactly the 12 replacement operators"
  exit 0
fi

echo "  FAIL  the two-TU collision set changed" >&2
echo "  --- unexpected (a new non-inline definition in a header) ---" >&2
printf '%s\n' "$got" > "$OUT/got.txt"; printf '%s\n' "$want" > "$OUT/want.txt"
LC_ALL=C comm -23 "$OUT/got.txt" "$OUT/want.txt" | sed 's/^/          /' >&2
echo "  --- missing (someone changed operator new/delete linkage) ---" >&2
LC_ALL=C comm -13 "$OUT/got.txt" "$OUT/want.txt" | sed 's/^/          /' >&2
exit 1
