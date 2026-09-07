#!/bin/sh
# THE GATE THAT ACTUALLY PROVES THE KERNEL SEAM.
#
# Compiling was never the hard part. Before Phase 5 wrote the kernel backends,
# tests/compiletests/barebones_core.cpp built with -DMICRON_PORT_KERNEL and the full kernel flag set
# compiled CLEANLY and emitted 41 syscall instructions, every one inside abc:: -- __va_reserve_once,
# __get_kernel_memory, __vmap_freeze_at, the sheet release paths. It compiled, it linked, and it
# would have executed `syscall` in ring 0. So "0 errors" is not the assertion; this is.
#
# Two properties, each with a live NEGATIVE CONTROL, because a gate that cannot fail reads green
# forever and is worse than no gate (BAREBONES.md records one that did exactly that):
#
#   1. a --kernel object contains NO syscall instruction.  control: the same source built -k on the
#      linux backend, which has ~30.
#   2. a --kernel object touches NO vector or x87 register. control: the same source at -march=v3
#      (x86) or tests/compiletests/math.cpp built normally (arm64).
#
# usage: sh scripts/check_kernel_object.sh [path-to-duck]
set -eu

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
duck="${1:-}"
if [ -z "$duck" ]; then
  if [ -x "$root/bin/duck" ]; then duck="$root/bin/duck"; else duck="duck"; fi
fi

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

pass=0
fail=0
ok()   { pass=$((pass + 1)); printf '  ok    %s\n' "$1"; }
no()   { fail=$((fail + 1)); printf '  FAIL  %s\n' "$1"; }

obj() { find "$1" -name '*.o' 2>/dev/null | head -1; }

# $1 desc  $2 object  $3 objdump  $4 pattern  $5 expected(zero|nonzero)
count_is() {
  d="$1"; o="$2"; od="$3"; pat="$4"; want="$5"
  if [ ! -f "$o" ]; then no "$d -- no object built"; return; fi
  n=$("$od" -d "$o" | grep -cE "$pat" || true)
  case "$want" in
    zero)    if [ "$n" -eq 0 ]; then ok "$d (0)"; else no "$d -- expected 0, got $n"; fi ;;
    nonzero) if [ "$n" -gt 0 ]; then ok "$d (control: $n)"; else no "$d -- control produced 0, so the check above cannot fail"; fi ;;
  esac
}

SYSCALL='\bsyscall\b|\bsysenter\b|int +\$0x80'
X86VEC='%xmm|%ymm|%zmm|%mm[0-7]|%st\('
A64SVC='\bsvc\b'
A64FP='fmul|fadd|fsub|fdiv|fcvt|fmov|fcmp|ldr[ \t]+[dqs][0-9]|str[ \t]+[dqs][0-9]|\.[0-9]+[bhsd]'

OD64=/usr/gcc-linaro-aarch64/bin/aarch64-none-linux-gnu-objdump

echo "[amd64]"
"$duck" compile tests/compiletests/barebones_kernel.cpp --x86 --isa base -O2 --kernel -o "$out/k" >/dev/null 2>&1
count_is "kernel object has no syscall"        "$(obj "$out/k")" objdump "$SYSCALL" zero
count_is "kernel object has no vector/x87 reg" "$(obj "$out/k")" objdump "$X86VEC"  zero
# controls
"$duck" compile tests/compiletests/port.cpp --x86 --isa base -O2 --raw-obj -k --def MICRON_NO_SIMD -o "$out/cs" >/dev/null 2>&1
count_is "CONTROL: linux backend does syscall"  "$(obj "$out/cs")" objdump "$SYSCALL" nonzero
"$duck" compile tests/compiletests/barebones_core.cpp --x86 --isa v3 -O2 --raw-obj -o "$out/cv" >/dev/null 2>&1
count_is "CONTROL: -march=v3 does use vectors"  "$(obj "$out/cv")" objdump "$X86VEC"  nonzero

if [ -x "$OD64" ]; then
  echo "[arm64]"
  "$duck" compile tests/compiletests/barebones_kernel.cpp --arm64 -O2 --kernel -o "$out/ka" >/dev/null 2>&1
  count_is "kernel object has no svc"            "$(obj "$out/ka")" "$OD64" "$A64SVC" zero
  count_is "kernel object has no FP instruction" "$(obj "$out/ka")" "$OD64" "$A64FP"  zero
  "$duck" compile tests/compiletests/port.cpp --arm64 -O2 --raw-obj -k --def MICRON_NO_SIMD -o "$out/cas" >/dev/null 2>&1
  count_is "CONTROL: linux backend does svc"     "$(obj "$out/cas")" "$OD64" "$A64SVC" nonzero
  "$duck" compile tests/compiletests/math.cpp --arm64 -O2 --raw-obj -o "$out/caf" >/dev/null 2>&1
  count_is "CONTROL: math.cpp does use FP"       "$(obj "$out/caf")" "$OD64" "$A64FP"  nonzero
else
  echo "[arm64] skipped: no cross objdump at $OD64"
fi

echo ""
echo "passed: $pass   failed: $fail"
[ "$fail" -eq 0 ]
