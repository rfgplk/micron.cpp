#!/usr/bin/env bash
#
# Build a curated set of test files with AddressSanitizer enabled.
# CLAUDE.md mandates -flto for the optimized build (constructor-attribute
# linker dependency): without it the global stdout-stream initializer is culled
# and the very first print throws except::memory_error at startup. ASan does not
# compose with LTO + -Ofast, but LTO + -O1 is fine, so this profile keeps -flto
# and only drops -Ofast. The trade-off: catches more bugs, but doesn't exercise
# the fully optimized SIMD codegen path. Run the standard `duck build` profile in
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
  -flto
  -fno-omit-frame-pointer
  -fsanitize=address
  -fno-sanitize-recover=all
  -march=native
  -Wall -Wextra -Wpedantic
  -Wno-variadic-macros -Wno-inline
  -DRIGOR_ITERS=1500
  -I./src
  -L./libs/
  -lpthread
)

# UBSan is supported by gcc but on this distro the libubsan runtime is not
# in the default lib search path (only ASan is). Add `-fsanitize=undefined`
# to COMMON_FLAGS if your install has libubsan; otherwise this profile runs
# ASan-only (which still catches heap UAF/double-free/out-of-bounds — the
# class of bugs B1/B2 produce).
#
# ######################## RESOLVED 2026-08-11 #############################
# The limitation recorded here on 2026-08-04 — a TU pulling src/std.hpp produced
# an ASan binary that DID NOT REPORT — is fixed. src/memory/new.hpp defined all
# twelve global replacement operators with no gate at all, and an executable's
# own definitions interpose over libasan's, so the sanitizer's allocator was
# bypassed and the reports went with it. They are now behind
# `#if !defined(__micron_sanitizer_owns_heap)`, matching what the C malloc family
# (malloc-c.hpp:29) and __abc_allocator (__abc.hpp:33) already did.
# micron::__alloc/__free (allocation/__internal.hpp) needed the same term — they
# are reached from alloc.hpp without going through operator new, and leaving them
# on the arena while ASan hands out libc blocks is a real allocator mismatch that
# malloc.hpp:227 would swallow silently.
#
#   $ nm -C <asan build>  | grep 'operator new(unsigned long)'   ->  U   (libasan's)
#   $ nm -C <plain build> | grep 'operator new(unsigned long)'   ->  T   (micron's)
#
# A green run here now means sanitizer-clean for heap double-free and
# use-after-free. It does NOT yet cover container out-of-bounds:
# serial_allocator.hpp:32 rounds every request to a 4096-byte granularity, so an
# overflow past a container's logical end stays inside its own malloc block and
# never touches a redzone. See ISSUES.md.
# ##########################################################################

OUT_DIR=bin/sanitize
mkdir -p "$OUT_DIR"

# Tests proven safe under ASan+UBSan (the new ones added in this push).
# Add to this list as more tests are vetted for sanitizer-cleanness.
DEFAULT_TESTS=(
  # number <-> text conversion layer. the write side allocates through hstring in the porcelain
  # (to_fixed / format / format_value) and the parse side is heap-free, so these are cheap to run
  # here and they cover the fixed.hpp / decimal.hpp rewrite.
  tests/rigor/rigor_format_write.cpp
  tests/rigor/rigor_conversions_chars.cpp
  tests/rigor/rigor_conversions_wide.cpp
  tests/rigor/rigor_format_float.cpp
  tests/rigor/rigor_format_int.cpp
  tests/rigor/rigor_format_buf.cpp
  tests/rigor/rigor_format_engine.cpp
  tests/rigor/rigor_format_parse.cpp
  tests/rigor/format_values.cpp
  tests/rigor/string_format_extensive.cpp
  tests/rigor/conversions_constexpr.cpp
  tests/rigor/vector_self_assignment.cpp
  tests/rigor/vector_try_reserve.cpp
  tests/rigor/deep_move_safety.cpp
  tests/rigor/ctrl_scan_tail.cpp
  tests/rigor/hash_table_property.cpp
  tests/rigor/concept_satisfaction.cpp
  tests/rigor/support_smoke.cpp
  tests/rigor/rigor_sstring.cpp
  tests/rigor/rigor_string.cpp
  tests/rigor/rigor_istring.cpp
  tests/rigor/rigor_rope.cpp
  tests/rigor/rigor_unistring.cpp
  tests/rigor/rigor_vector.cpp
  tests/rigor/rigor_svector.cpp
  tests/rigor/rigor_fvector.cpp
  tests/rigor/rigor_ivector.cpp
  tests/rigor/rigor_pvector.cpp
  tests/rigor/rigor_convector.cpp
  tests/rigor/rigor_circle_vector.cpp
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
  # RIGOR EXIT CONTRACT (tests/snowball, tools/src/recipes/gnu/qemu.hh): a test PASSES by
  # returning 1, the success sentinel. Shell convention is inverted from that -- `if "$bin"`
  # scores exit 0 as success, which is precisely the "ran off the end of main without reaching
  # the sentinel" FAILURE. Grade on rc == 1, the way tests/build/freestanding_threads.sh does.
  #
  # WARNING: rc == 1 collides head-on with the sanitizers' OWN default exit code (common_flags
  # exitcode=1). With abort_on_error=0 a clean ASan report exits 1 and would grade as a PASS in
  # the one script whose entire job is catching those reports. Move them off 1 explicitly.
  # LeakSanitizer already defaults to 23, but pin it too rather than rely on that.
  SAN_RC=77
  ASAN_OPTIONS="detect_leaks=1:abort_on_error=0:halt_on_error=1:exitcode=$SAN_RC" \
    UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:exitcode=$SAN_RC" \
    LSAN_OPTIONS="exitcode=$SAN_RC" \
    "$bin" >"$bin.run.log" 2>&1
  rc=$?
  # second line of defence: a recovering sanitizer, or one whose exitcode the runtime ignored,
  # still writes its report. A test that "passed" with a report in its log did not pass.
  san_report=""
  if grep -qE '(ERROR|WARNING): (Address|Leak|Memory|Thread|Undefined)Sanitizer|runtime error:|SUMMARY: .*Sanitizer' \
       "$bin.run.log" 2>/dev/null; then
    san_report="sanitizer report in log"
  fi
  if [[ $rc -eq 1 && -z $san_report ]]; then
    echo "ok"
    PASS=$((PASS + 1))
  else
    case $rc in
      1)             why="$san_report" ;;
      6)             why="require()-fail" ;;
      0)             why="no success sentinel" ;;
      "$SAN_RC")     why="sanitizer report (exit $rc)" ;;
      23)            why="LeakSanitizer (exit 23)" ;;
      139)           why="SIGSEGV" ;;
      134)           why="SIGABRT (sanitizer?)" ;;
      *)             why="exit $rc${san_report:+ + $san_report}" ;;
    esac
    echo "FAIL ($why)"
    FAIL=$((FAIL + 1))
    FAILURES+=("$src (run $why)")
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
