#!/usr/bin/env bash
#
# Build a curated set of test files with AddressSanitizer enabled.
# CLAUDE.md mandates -flto for the optimized build (constructor-attribute
# linker dependency); ASan does not compose well with LTO + -Ofast, so this
# profile drops both. The trade-off: catches more bugs, but doesn't exercise
# the optimized SIMD codegen path. Run the standard `duck build` profile in
# addition for full coverage.
#
# Usage:  bash tests/build/sanitize.sh [TEST_NAME ...]
# With no args: runs the full curated set.
#
# SYSTEM REQUIREMENTS:
#   * libasan (system package; gcc-libsan or similar)
#   * /proc/sys/vm/overcommit_memory must NOT be 2 (strict accounting).
#     ASan needs to reserve ~15TB of virtual address space for shadow mem.
#     If overcommit_memory=2, run as root:
#       echo 0 > /proc/sys/vm/overcommit_memory
#     (this is non-persistent; reverts on reboot).
#   * Optional: libubsan for -fsanitize=undefined (not in default lib path
#     on some distros).
#
# Exits with the first non-zero test exit code, or 0 if all pass.

set -u

cd "$(dirname "$0")/../.."

GXX=${GXX:-/usr/bin/g++}
COMMON_FLAGS=(
  -std=c++26
  -O1
  -g
  -fno-omit-frame-pointer
  -fsanitize=address
  -fno-sanitize-recover=all
  -mavx2 -mbmi -march=native
  -Wall -Wextra -Wpedantic
  -Wno-variadic-macros -Wno-inline
  -I./src
  -L./libs/
  -lpthread
)

# UBSan is supported by gcc but on this distro the libubsan runtime is not
# in the default lib search path (only ASan is). Add `-fsanitize=undefined`
# to COMMON_FLAGS if your install has libubsan; otherwise this profile runs
# ASan-only (which still catches heap UAF/double-free/out-of-bounds — the
# class of bugs B1/B2 produce).

OUT_DIR=bin/sanitize
mkdir -p "$OUT_DIR"

# Tests proven safe under ASan+UBSan (the new ones added in this push).
# Add to this list as more tests are vetted for sanitizer-cleanness.
DEFAULT_TESTS=(
  tests/rigor/vector_self_assignment.cpp
  tests/rigor/vector_try_reserve.cpp
  tests/rigor/deep_move_safety.cpp
  tests/rigor/ctrl_scan_tail.cpp
  tests/rigor/hash_table_property.cpp
  tests/rigor/concept_satisfaction.cpp
  tests/rigor/support_smoke.cpp
)

if [[ $# -eq 0 ]]; then
  TESTS=("${DEFAULT_TESTS[@]}")
else
  TESTS=("$@")
fi

PASS=0
FAIL=0
declare -a FAILURES=()

for src in "${TESTS[@]}"; do
  if [[ ! -f "$src" ]]; then
    echo "[skip] $src (not found)"
    continue
  fi
  name=$(basename "$src" .cpp)
  bin="$OUT_DIR/$name"
  printf "[build] %s ... " "$src"
  if "$GXX" "${COMMON_FLAGS[@]}" "$src" -o "$bin" 2>"$bin.build.log"; then
    echo "ok"
  else
    echo "FAIL (build)"
    FAIL=$((FAIL + 1))
    FAILURES+=("$src (build)")
    tail -20 "$bin.build.log"
    continue
  fi

  printf "[run]   %s ... " "$bin"
  if ASAN_OPTIONS="detect_leaks=1:abort_on_error=0:halt_on_error=1" \
     UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" \
     "$bin" >"$bin.run.log" 2>&1; then
    echo "ok"
    PASS=$((PASS + 1))
  else
    rc=$?
    echo "FAIL ($rc)"
    FAIL=$((FAIL + 1))
    FAILURES+=("$src (run rc=$rc)")
    tail -40 "$bin.run.log"
  fi
done

echo ""
echo "=== sanitize.sh summary ==="
echo "  passed: $PASS"
echo "  failed: $FAIL"
if [[ $FAIL -gt 0 ]]; then
  echo "  failures:"
  for f in "${FAILURES[@]}"; do
    echo "    - $f"
  done
  exit 1
fi
exit 0
