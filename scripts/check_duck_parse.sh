#!/usr/bin/env bash
#  Copyright (c) 2024- David Lucius Severus
#  Distributed under the Boost Software License, Version 1.0.
#
# check_duck_parse.sh [path/to/duck]
set -u

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/.." && pwd)"

duck="${1:-}"
if [ -z "$duck" ]; then
  if [ -x "$root/bin/duck" ]; then duck="$root/bin/duck"; else duck="duck"; fi
fi
command -v "$duck" >/dev/null 2>&1 || { echo "no duck at '$duck'"; exit 2; }

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
cd "$work" || exit 2
mkdir -p inc1 inc2 libs deep/nest
printf 'int main(){return 1;}\n' > t.cpp
printf 'int main(){return 1;}\n' > u.cpp
printf 'int f();\n'              > deep/top.cpp
printf 'int g();\n'              > deep/nest/inner.cpp

# a fake micron crt, so the freestanding checks below never depend on what this host installed
mkdir -p mystart
touch mystart/start.s        mystart/start_i386.s  mystart/start_arm32.s  mystart/start_arm64.s \
      mystart/direct.s       mystart/direct_i386.s mystart/direct_arm32.s mystart/direct_arm64.s \
      mystart/start.cpp      mystart/eh_runtime.cpp
printf '.text\n.global _start\n_start:\n'  > boot.s

pass=0
fail=0

# splat emits the compile line first; the run line (if any) follows
compile_line() { "$duck" splat "$@" 2>/dev/null | head -1; }

ok() { pass=$((pass + 1)); }
no() { fail=$((fail + 1)); printf '  FAIL  %s\n' "$1"; }

# the emitted compile line must contain every needle
want() {
  desc="$1"; shift
  line="$1"; shift
  for needle in "$@"; do
    case "$line" in
      *"$needle"*) ;;
      *) no "$desc -- missing '$needle'"; printf '        got: %s\n' "$line"; return ;;
    esac
  done
  ok
}

same() {
  desc="$1"; a="$2"; b="$3"
  if [ "$a" = "$b" ]; then ok; else
    no "$desc -- orders disagree"
    printf '        A: %s\n        B: %s\n' "$a" "$b"
  fi
}

# the emitted compile line must contain none of the needles
wantnot() {
  desc="$1"; shift
  line="$1"; shift
  for needle in "$@"; do
    case "$line" in
      *"$needle"*) no "$desc -- unexpected '$needle'"; printf '        got: %s\n' "$line"; return ;;
    esac
  done
  ok
}

# a malformed line must not succeed quietly
must_fail() {
  desc="$1"; shift
  if "$duck" splat "$@" >/dev/null 2>&1; then
    no "$desc -- accepted, should have been rejected"
  else
    ok
  fi
}

echo "duck: $duck"
echo

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
echo "[order independence]"
after=$(compile_line  build t.cpp -i inc1 -i inc2 -o out)
before=$(compile_line build -i inc1 -i inc2 -o out t.cpp)
split=$(compile_line  build -i inc1 t.cpp -i inc2 -o out)
want "flags after source"  "$after" " t.cpp " "-Iinc1" "-Iinc2" "-o out/t"
same "before vs after"     "$after" "$before"
same "split vs after"      "$after" "$split"

# the first -i replaces the seeded ./src, later ones accumulate
case "$after" in
  *-I./src*) no "first -i should have replaced the default ./src"; printf '        got: %s\n' "$after" ;;
  *) ok ;;
esac
want "default include when no -i" "$(compile_line build t.cpp)" "-I./src"

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
echo "[output naming]"
same "-o before/after an explicit name" \
     "$(compile_line build t.cpp -o out myname)" "$(compile_line build t.cpp myname -o out)"
want "explicit .bin output" "$(compile_line build t.cpp -o out final.bin)" "-o out/final.bin"
want "extra .o is linked in" "$(compile_line build t.cpp -o out extra.o)" "extra.o"

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
echo "[cross targets]"
for order in "emulate t.cpp --arm64 -i inc1 -o out" "emulate --arm64 -i inc1 -o out t.cpp"; do
  # shellcheck disable=SC2086
  want "emulate: $order" "$(compile_line $order)" "aarch64" " t.cpp " "-Iinc1" "-o out/t"
done
# shellcheck disable=SC2086
run_line=$("$duck" splat emulate --arm64 -i inc1 -o out t.cpp 2>/dev/null | tail -1)
want "emulate run line goes through qemu" "$run_line" "qemu-aarch64-static" "-L " "out/t"

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
echo "[clang profile]"
clang_fast=$(compile_line build t.cpp --clang -Ofast -o out)
want "clang executable and fast mapping" "$clang_fast" "/clang++" "-O3" "-ffast-math" "-flto=thin"
wantnot "clang drops GCC-only optimization flags" "$clang_fast" \
        "-fmodulo-sched" "-fgcse-sm" "-flto=8" "-fext-numeric-literals" "-fconcepts-diagnostics-depth"
want "clang --no-lto" "$(compile_line build t.cpp --clang --no-lto -o out)" "-fno-lto"
wantnot "clang --no-lto drops ThinLTO" "$(compile_line build t.cpp --clang --no-lto -o out)" "-flto=thin"
want "clang arm32 cross driver" "$(compile_line build t.cpp --clang --arm --raw-obj -o out)" \
     "/clang++" "--target=arm-none-linux-gnueabihf" "--gcc-toolchain=/usr/gcc-linaro" \
     "--sysroot=/usr/gcc-linaro/arm-none-linux-gnueabihf/libc"
want "clang arm64 cross driver" "$(compile_line build t.cpp --clang --arm64 --raw-obj -o out)" \
     "/clang++" "--target=aarch64-none-linux-gnu" "--gcc-toolchain=/usr/gcc-linaro-aarch64" \
     "--sysroot=/usr/gcc-linaro-aarch64/aarch64-none-linux-gnu/libc"
want "clang gas follows the target" "$(compile_line build boot.s --clang --arm --raw-obj -o out)" \
     "/clang " "--target=arm-none-linux-gnueabihf" "-march=armv7-a"
clang_k=$(compile_line build t.cpp --clang -k --start mystart -o out)
want "clang freestanding preserves the main ABI" "$clang_k" \
     "-fhosted" "-fno-builtin" "-D__micron_freestanding=1" "-fno-unwind-tables" "-fno-asynchronous-unwind-tables"
wantnot "clang freestanding does not mangle main" "$clang_k" "-ffreestanding"
clang_ke=$(compile_line build t.cpp --clang -ke --start mystart -o out)
want "clang EH keeps unwind tables" "$clang_ke" "-fhosted" "-D__micron_freestanding=1" "-fasynchronous-unwind-tables"
wantnot "clang EH keeps unwind tables" "$clang_ke" "-fno-unwind-tables" "-fno-asynchronous-unwind-tables"
want "clang arm64 links binary128 support" "$(compile_line build t.cpp --clang --arm64 -k --start mystart -o out)" "-lgcc"
must_fail "clang armv7 rejects CFI" build t.cpp --clang --arm --cfi
must_fail "clang rejects static TSAN" build t.cpp --clang --tsan -s

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
echo "[malformed lines are rejected, not absorbed]"
must_fail "unknown flag"          build t.cpp --urign
must_fail "gcc-style glued -I"    build t.cpp -I./inc1
must_fail "glued -i"              build t.cpp -i./inc1
must_fail "two sources"           build t.cpp u.cpp
must_fail "no source at all"      build -i inc1 -o out
must_fail "--recursive on a file" build t.cpp --recursive
must_fail "--recursive on run"    run t.cpp --recursive

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
echo "[-freflection]"
want "emits the flag + its define" "$(compile_line build t.cpp -freflection -o out)" \
     "-freflection" "-std=c++26" "-DMICRON_REFLECTION"
case "$(compile_line build t.cpp -o out)" in
  *-freflection*) no "-freflection leaked into a build that did not ask for it" ;;
  *) ok ;;
esac
# the standard check is deferred to finalize_and_infer, so it must not depend on flag order
must_fail "reflection under c++23"        build t.cpp -freflection --std c++23
must_fail "reflection under c++23 (rev)"  build t.cpp --std c++23 -freflection
must_fail "reflection on arm"             build t.cpp -freflection --arm
must_fail "reflection on arm64"           build t.cpp -freflection --arm64
must_fail "reflection under clang"        build t.cpp -freflection --clang
must_fail "reflection on a C target"      build t.cpp -freflection -c

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
echo "[directory mode]"
flat=$("$duck" splat build deep 2>/dev/null | grep -c 'top\.cpp')
rec=$("$duck"  splat build deep --recursive 2>/dev/null | grep -c 'inner\.cpp')
norec=$("$duck" splat build deep 2>/dev/null | grep -c 'inner\.cpp')
[ "$flat" -eq 1 ]  && ok || no "flat dir mode missed deep/top.cpp"
[ "$rec" -eq 1 ]   && ok || no "--recursive missed deep/nest/inner.cpp"
[ "$norec" -eq 0 ] && ok || no "flat dir mode should NOT descend into deep/nest"

# same basename in two subdirectories collides on bin/<name>
cp deep/top.cpp deep/nest/top.cpp
must_fail "basename collision under --recursive" build deep --recursive

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
echo "[freestanding crt: --start / MICRON_START / --direct]"
# the crt path is --start > $MICRON_START > /usr/src/mc_start, resolved in finalize_and_infer
k=$(compile_line build t.cpp -k --start mystart -o out)
want    "--start relocates the crt"     "$k" "mystart/start.s" "mystart/start.cpp"
wantnot "--start replaces the default"  "$k" "usr/src/mc_start"
want    "-ke adds the eh trampoline"    "$(compile_line build t.cpp -ke --start mystart -o out)" "mystart/eh_runtime.cpp"
same    "a trailing slash is idempotent" "$k" "$(compile_line build t.cpp -k --start mystart/ -o out)"

# the stub is arch- and width-aware; --direct swaps it for the __micron_directc entry
want "-32 takes the i386 stub"    "$(compile_line build t.cpp -k -32     --start mystart -o out)" "mystart/start_i386.s"
want "--arm takes the arm32 stub" "$(compile_line build t.cpp -k --arm   --start mystart -o out)" "mystart/start_arm32.s"
want "--arm64 takes the arm64 stub" "$(compile_line build t.cpp -k --arm64 --start mystart -o out)" "mystart/start_arm64.s"
d=$(compile_line build t.cpp -k --direct --start mystart -o out)
want    "--direct swaps the stub"  "$d" "mystart/direct.s" "mystart/start.cpp"
wantnot "--direct drops start.s"   "$d" "mystart/start.s "
want "--direct -32"     "$(compile_line build t.cpp -k --direct -32     --start mystart -o out)" "mystart/direct_i386.s"
want "--direct --arm"   "$(compile_line build t.cpp -k --direct --arm   --start mystart -o out)" "mystart/direct_arm32.s"
want "--direct --arm64" "$(compile_line build t.cpp -k --direct --arm64 --start mystart -o out)" "mystart/direct_arm64.s"

# MICRON_START is the whole-run fallback; the flag outranks it
want "MICRON_START is honoured" \
     "$(MICRON_START=mystart "$duck" splat build t.cpp -k -o out 2>/dev/null | head -1)" "mystart/start.s"
want "--start outranks MICRON_START" \
     "$(MICRON_START=/nope "$duck" splat build t.cpp -k --start mystart -o out 2>/dev/null | head -1)" "mystart/start.s"
wantnot "MICRON_START stays off hosted lines" \
     "$(MICRON_START=/nope "$duck" splat build t.cpp -o out 2>/dev/null | head -1)" "/nope"

# --start takes a value, so __find_source must step over it rather than build the directory
want "--start does not swallow the source" "$(compile_line build --start mystart -k t.cpp -o out)" " t.cpp "

# the three places a freestanding build links no crt: static-PIE on x86, a non-linking compile,
# and a .s/.asm target (batch_gas links bare). none of them may demand the files
want    "x86 static-PIE links no crt" "$(compile_line build t.cpp -k --static-pie --start /nope -o out)" "-static-pie"
wantnot "x86 static-PIE links no crt" "$(compile_line build t.cpp -k --static-pie --start /nope -o out)" "start.s" "start.cpp"
want    "--raw-obj never links a crt" "$(compile_line build t.cpp -k --raw-obj --start /nope -o out)" " t.cpp "
want    "a .s target links bare"      "$(compile_line build boot.s -k --start /nope -o out)" "boot.s"
# ...but arm has never made the static-PIE exclusion, and that asymmetry is deliberate
want "arm64 static-PIE still links its crt" \
     "$(compile_line build t.cpp -k --arm64 --static-pie --start mystart -o out)" "mystart/start_arm64.s"

must_fail "crt directory does not exist"  build t.cpp -k --start ./nonexistent
must_fail "--start with no value"         build t.cpp -k --start
must_fail "--start followed by a flag"    build t.cpp -k --start -o out
must_fail "--start without -k"            build t.cpp --start mystart
must_fail "--direct without -k"           build t.cpp --direct
must_fail "--direct under x86 static-PIE" build t.cpp -k --direct --static-pie
must_fail "--start but no source"         build --start mystart -k

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
echo
echo "passed: $pass   failed: $fail"
[ "$fail" -eq 0 ]
