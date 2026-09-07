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
mkdir -p mystart/metal
touch mystart/metal/reset.s        mystart/metal/reset_i386.s  mystart/metal/reset_arm32.s \
      mystart/metal/reset_arm64.s  mystart/metal/reset_amd64_lm.s mystart/metal/reset_cortexm.s \
      mystart/metal/metal_start.cpp mystart/metal/mc_mport.cpp \
      mystart/metal/mc_metal_libgcc.cpp \
      mystart/metal/metal.ld       mystart/metal/metal_i386.ld mystart/metal/metal_arm32.ld \
      mystart/metal/metal_arm64.ld mystart/metal/metal_stm32.ld
printf '.text\n.global _start\n_start:\n'  > boot.s

pass=0
fail=0

# splat emits the compile line first; the run line (if any) follows
compile_line() { "$duck" splat "$@" 2>/dev/null | head -1; }

# stdout OR stderr
line_any() { "$duck" splat "$@" 2>&1 | head -1; }

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
# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
echo "[--kernel: the Phase 5 kernel-module object]"
# the codegen set, per arch. these are the flags that make a .ko object legal; a missing one is not
# a style problem, it is a corrupted userspace FP state or a link failure inside kbuild.
want "kernel amd64 kills the vector units" "$(compile_line compile t.cpp --kernel --x86 -o out)" \
     "-mno-sse" "-mno-mmx" "-mno-80387" "-mno-3dnow" "-mno-red-zone" "-mcmodel=kernel"
want "kernel amd64 kills the C++ runtime hooks" "$(compile_line compile t.cpp --kernel --x86 -o out)" \
     "-fno-threadsafe-statics" "-fno-use-cxa-atexit" "-fno-common" "-fno-asynchronous-unwind-tables" "-fno-pie"
want "kernel defines the four macros" "$(compile_line compile t.cpp --kernel --x86 -o out)" \
     "-DMICRON_PORT_KERNEL" "-DMICRON_NO_SIMD" "-DMICRON_NO_FP" "-DMICRON_NO_TLS"
want "kernel arm64 uses -mgeneral-regs-only" "$(compile_line compile t.cpp --kernel --arm64 -o out)" "-mgeneral-regs-only"
# armv7's -mgeneral-regs-only rejects a float in a DECLARATION, not merely in codegen; soft-float is
# what the real arm32 kernel builds with. Getting this wrong is 426 errors in numerics.hpp alone.
want    "kernel armv7 uses soft-float"      "$(compile_line compile t.cpp --kernel --arm -o out)" "-mfloat-abi=soft"
wantnot "kernel armv7 avoids -mgeneral-regs-only" "$(compile_line compile t.cpp --kernel --arm -o out)" "-mgeneral-regs-only"
# --kernel forces a real object: no LTO bytecode, and no crt however hard you ask for one
want    "kernel is a real object" "$(compile_line compile t.cpp --kernel --x86 -o out)" "-c" "-fno-lto"
wantnot "kernel links no crt"     "$(compile_line compile t.cpp --kernel --x86 --start /nope -o out)" "start.s" "start.cpp"
# and it is not a program
must_fail "duck run --kernel"     run t.cpp --kernel
must_fail "duck test --kernel"    test t.cpp --kernel
must_fail "duck emulate --kernel" emulate t.cpp --kernel --arm64
must_fail "--kernel with --direct" compile t.cpp --kernel --direct

echo "[--metal: the Phase 6 bare-metal image]"

# the same "no vector unit, no FPU" set as --kernel...
want "metal amd64 kills the vector units" "$(compile_line build t.cpp --metal --x86 --start mystart -o out)" \
     "-mno-sse" "-mno-mmx" "-mno-80387" "-mno-3dnow" "-mno-red-zone"
# ...but NOT the kernel's own address model: -mcmodel=kernel is a Linux-kernel choice and means
# nothing on a board
wantnot "metal does not use -mcmodel=kernel" "$(compile_line build t.cpp --metal --x86 --start mystart -o out)" "-mcmodel=kernel"
want "metal defines the four macros" "$(compile_line build t.cpp --metal --x86 --start mystart -o out)" \
     "-DMICRON_PORT_METAL" "-DMICRON_NO_SIMD" "-DMICRON_NO_FP" "-DMICRON_NO_TLS"
want "metal arm64 uses -mgeneral-regs-only" "$(compile_line build t.cpp --metal --arm64 --start mystart -o out)" "-mgeneral-regs-only"
want "metal armv7 uses soft-float"         "$(compile_line build t.cpp --metal --arm --start mystart -o out)" "-mfloat-abi=soft"

# UNLIKE --kernel, IT LINKS -- the reset vector, the entry body, the board hooks, the libgcc shim
# and the layout script. A board has no other linker.
want "metal links the reset vector + crt" "$(compile_line build t.cpp --metal --x86 --start mystart -o out)" \
     "mystart/metal/reset.s" "mystart/metal/metal_start.cpp" "mystart/metal/mc_mport.cpp" "mystart/metal/mc_metal_libgcc.cpp"
want "metal i386 takes the i386 reset vector" "$(compile_line build t.cpp --metal --i386 --start mystart -o out)" "mystart/metal/reset_i386.s"
want "metal arm32 takes the arm32 reset vector" "$(compile_line build t.cpp --metal --arm --start mystart -o out)" "mystart/metal/reset_arm32.s"
want "metal arm64 takes the arm64 reset vector" "$(compile_line build t.cpp --metal --arm64 --start mystart -o out)" "mystart/metal/reset_arm64.s"
# NOT start.cpp: that boots TLS, auxv, atexit, a threadpool and io buffers, none of which exists here
wantnot "metal does not link start.cpp" "$(compile_line build t.cpp --metal --x86 --start mystart -o out)" "mystart/start.cpp" "mystart/start.s"
# the layout script is not optional and is per arch
want "metal amd64 uses metal.ld"  "$(compile_line build t.cpp --metal --x86   --start mystart -o out)" "-T" "mystart/metal/metal.ld"
want "metal i386 uses metal_i386.ld" "$(compile_line build t.cpp --metal --i386 --start mystart -o out)" "mystart/metal/metal_i386.ld"
want "metal arm64 uses metal_arm64.ld" "$(compile_line build t.cpp --metal --arm64 --start mystart -o out)" "mystart/metal/metal_arm64.ld"
want "metal is static" "$(compile_line build t.cpp --metal --x86 --start mystart -o out)" "-static"

# and it is not a host program, and it owns the entry slot
must_fail "duck run --metal"       run t.cpp --metal --start mystart
must_fail "duck test --metal"      test t.cpp --metal --start mystart
must_fail "duck emulate --metal"   emulate t.cpp --metal --arm64 --start mystart
must_fail "--metal with --kernel"  compile t.cpp --metal --kernel
must_fail "--metal with --direct"  compile t.cpp --metal --direct
must_fail "--metal with --mx"      compile t.cpp --metal --mx
# a board port is a second TU by design (weak hooks vs strong), so compiling one alone is allowed
want "metal accepts a lone TU as an object" "$(compile_line compile t.cpp --metal --raw-obj --x86 --start mystart -o out)" "-c" "-DMICRON_PORT_METAL"
must_fail "--metal with --static-pie" build t.cpp --metal --static-pie --start mystart

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# --metal-entry: WHO SWITCHES THE CPU INTO LONG MODE
#
# amd64 has two reset vectors and the difference is not cosmetic. The default is entered by a loader
# in 32-BIT protected mode -- that is what PVH gives, and PVH is the only protocol qemu will use for
# a 64-bit ELF -- so reset.s builds a GDT and an identity map itself. `lm` assumes the board's first
# stage already did, and nothing in this tree can boot that shape, which is exactly why linking it
# is gated.
want "metal-entry default is the pvh trampoline" "$(compile_line build t.cpp --metal --x86 --start mystart -o out)" "mystart/metal/reset.s"
want "metal-entry lm takes the long-mode stub"   "$(compile_line build t.cpp --metal --metal-entry lm --x86 --start mystart -o out)" "mystart/metal/reset_amd64_lm.s"
wantnot "metal-entry lm does not take reset.s"   "$(compile_line build t.cpp --metal --metal-entry lm --x86 --start mystart -o out)" "mystart/metal/reset.s "
want "metal-entry pvh is the default spelled out" "$(compile_line build t.cpp --metal --metal-entry pvh --x86 --start mystart -o out)" "mystart/metal/reset.s"
# both shapes share one layout script
want "metal-entry lm still uses metal.ld" "$(compile_line build t.cpp --metal --metal-entry lm --x86 --start mystart -o out)" "mystart/metal/metal.ld"
must_fail "--metal-entry without --metal"  build t.cpp -k --metal-entry lm --start mystart
must_fail "--metal-entry lm on arm64"      build t.cpp --metal --metal-entry lm --arm64 --start mystart
must_fail "--metal-entry lm on i386"       build t.cpp --metal --metal-entry lm --i386 --start mystart
must_fail "--metal-entry with a bad value" build t.cpp --metal --metal-entry sideways --x86 --start mystart

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# THE BUILT-IN CRT DEFAULT IS PER-CRT
#
# There are two crts and they cannot share a directory: the barebones start/ names
# <micron/port/backends/__syscall.hpp>, which exists only on this branch, and the userland one names
# <micron/syscall.hpp>. scripts/install_start.py:8 has always installed to /usr/src/mc_start_bb for
# that reason -- "don't clobber the userland start" -- while duck defaulted BOTH to /usr/src/mc_start.
# So `duck --metal` with no --start looked in the userland directory for metal/ files that
# install_start.py had put somewhere else, and said "cannot open linker script" about a path that
# looks entirely reasonable.
#
# THE TRAILING SLASH IN THESE ASSERTIONS IS LOAD-BEARING: "/usr/src/mc_start_bb" contains
# "/usr/src/mc_start" as a substring, so a check for the userland path without it passes on the
# barebones one. conf.start_dir is slash-normalised before use, so "/usr/src/mc_start/" is exact.
#
# These deliberately pass no --start, which is the whole point. Whether the directory exists decides
# whether duck prints a command or an error naming the missing file -- and both name the path, which
# is what is being asserted.
want "userland crt defaults to mc_start"    "$(line_any build t.cpp --x86 -k -o out)"  "/usr/src/mc_start/"
wantnot "userland crt is not the bb one"    "$(line_any build t.cpp --x86 -k -o out)"  "/usr/src/mc_start_bb"
want "--mx keeps the userland crt"          "$(line_any build t.cpp --x86 -k --mx -o out)" "/usr/src/mc_start/"
want "--direct keeps the userland crt"      "$(line_any build t.cpp --x86 -k --direct -o out)" "/usr/src/mc_start/"
want "--metal defaults to mc_start_bb"      "$(line_any build t.cpp --x86 --metal -o out)" "/usr/src/mc_start_bb"
want "--metal arm64 too"                    "$(line_any build t.cpp --arm64 --metal -o out)" "/usr/src/mc_start_bb"
want "--metal --cortex-m too"               "$(line_any build t.cpp --cortex-m cortex-m4 --metal -o out)" "/usr/src/mc_start_bb"
# and an explicit path still beats the default, either way round
want "--start beats the bb default"         "$(compile_line build t.cpp --x86 --metal --start mystart -o out)" "mystart/metal/metal.ld"
wantnot "--start really beats it"           "$(compile_line build t.cpp --x86 --metal --start mystart -o out)" "/usr/src/mc_start"

want "metal arm64 is strict-align"       "$(compile_line build t.cpp --metal --arm64 --start mystart -o out)" "-mstrict-align"
want "metal armv7-a forbids unaligned"   "$(compile_line build t.cpp --metal --arm --start mystart -o out)" "-mno-unaligned-access"
wantnot "metal cortex-m does NOT"        "$(compile_line build t.cpp --cortex-m cortex-m4 --metal --start mystart -o out)" "-mno-unaligned-access"
wantnot "metal amd64 has no arm flags"   "$(compile_line build t.cpp --metal --x86 --start mystart -o out)" "-mstrict-align" "-mno-unaligned-access"
# and --kernel does not get them: a module runs with the MMU on and its memory Normal
wantnot "kernel arm64 is not strict-align" "$(compile_line compile t.cpp --kernel --arm64 -o out)" "-mstrict-align"
wantnot "kernel armv7 does not forbid unaligned" "$(compile_line compile t.cpp --kernel --arm -o out)" "-mno-unaligned-access"

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# --cortex-m: ARMv7-M, an ARM SUB-TARGET
want "cortex-m takes the cpu it was given" "$(compile_line build t.cpp --cortex-m cortex-m4 --metal --start mystart -o out)" "-mcpu=cortex-m4" "-mthumb" "-mfloat-abi=soft"
want "cortex-m3 too" "$(compile_line build t.cpp --cortex-m cortex-m3 --metal --start mystart -o out)" "-mcpu=cortex-m3"
wantnot "cortex-m drops the armv7-a trio" "$(compile_line build t.cpp --cortex-m cortex-m4 --metal --start mystart -o out)" "-march=armv7-a" "-mfpu=neon" "-mfloat-abi=hard"
want "cortex-m takes the M-profile reset table" "$(compile_line build t.cpp --cortex-m cortex-m4 --metal --start mystart -o out)" "mystart/metal/reset_cortexm.s"
want "cortex-m takes the two-region layout" "$(compile_line build t.cpp --cortex-m cortex-m4 --metal --start mystart -o out)" "mystart/metal/metal_stm32.ld"
# --arm is untouched by any of this
want "--arm still armv7-a neon hard" "$(compile_line build t.cpp --arm --metal --start mystart -o out)" "-march=armv7-a" "-mfpu=neon"
want "--arm still takes reset_arm32.s" "$(compile_line build t.cpp --arm --metal --start mystart -o out)" "mystart/metal/reset_arm32.s"
must_fail "--cortex-m with --marm"        build t.cpp --cortex-m cortex-m4 --marm --metal --start mystart
must_fail "--cortex-m with --kernel"      compile t.cpp --cortex-m cortex-m3 --kernel
must_fail "--cortex-m with --mtp"         build t.cpp --cortex-m cortex-m4 --mtp soft --metal --start mystart
must_fail "--cortex-m eats no flag"       build t.cpp --cortex-m -o out
must_fail "--cortex-m rejects an arch"    build t.cpp --cortex-m armv7-m --metal --start mystart

echo "passed: $pass   failed: $fail"
[ "$fail" -eq 0 ]
