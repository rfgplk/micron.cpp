#!/usr/bin/env bash
#
# mic-thread validation runner: build + run each threading rigor test across
# {hosted, freestanding (-k), freestanding+EH (-ke)} and assert the rigor
# success contract
#
# RIGOR EXIT CONTRACT (tests/snowball):
#   1   = PASS   (the success sentinel `return 1`)
#   6   = require() failure  (__abort -> sys_exit(6))   -> logic/assert failure
#   139 = SIGSEGV (128+11)                              -> almost always a
#                                                          spawned-thread TLS /
#                                                          stack-canary bug
#   0   = ran to the end without hitting the sentinel   -> fail
#   *   = anything else                                 -> fail
# A mode PASSES iff the binary exits 1.
#
# Only amd64 is built+run here (the host). ARM64/ARM32/i386 columns are written
# behind their __micron_arch_* guards and validated by inspection / qemu when a
# cross toolchain + sysroot exists; they are intentionally NOT exercised here.
#
# Usage:  bash tests/build/freestanding_threads.sh [MODE ...] -- [TEST ...]
#   MODE  one of: hosted k ke           (default: all three)
#   TEST  paths to .cpp                  (default: the curated thread set)
#
# Exits non-zero if any (test,mode) cell fails.

set -u
cd "$(dirname "$0")/../.."

DUCK=./bin/duck
[[ -x "$DUCK" ]] || DUCK=duck   # fall back to PATH, but prefer the freshly-built one

# --- reinstall guard: start/ is consumed from /usr/src/mc_start, not in-tree ---
MC_START=/usr/src/mc_start
if [[ -d start && -d "$MC_START" ]]; then
  newest_local=$(find start -type f -newer "$MC_START/start.cpp" 2>/dev/null | head -1)
  if [[ -n "$newest_local" ]]; then
    echo "WARNING: start/ has edits newer than $MC_START (e.g. $newest_local)."
    echo "         Freestanding (-k/-ke) builds will use the STALE installed copy."
    echo "         Run:  sudo python3 scripts/install_start.py"
    echo ""
  fi
fi

ALL_MODES=(hosted k ke)
MODES=()
TESTS=()
seen_dd=0
for a in "$@"; do
  if [[ "$a" == "--" ]]; then seen_dd=1; continue; fi
  if [[ $seen_dd -eq 0 ]]; then MODES+=("$a"); else TESTS+=("$a"); fi
done
[[ ${#MODES[@]} -eq 0 ]] && MODES=("${ALL_MODES[@]}")

DEFAULT_TESTS=(
  tests/rigor/thread_solo_api.cpp
  tests/rigor/thread_reg.cpp
  tests/rigor/thread_auto.cpp
  tests/rigor/thread_group.cpp
  tests/rigor/thread_void.cpp
  tests/rigor/thread_async.cpp
  tests/rigor/thread_pool.cpp
  tests/rigor/thread_stress.cpp
  tests/rigor/thread_patches.cpp
  tests/core/threads.cpp
)
[[ ${#TESTS[@]} -eq 0 ]] && TESTS=("${DEFAULT_TESTS[@]}")

mode_flag() { case "$1" in hosted) echo "" ;; k) echo "-k" ;; ke) echo "-ke" ;; esac; }

decode() {  # human-readable verdict for a run exit code
  case "$1" in
    1)   echo "PASS" ;;
    6)   echo "require()-fail" ;;
    139) echo "SIGSEGV (TLS/canary?)" ;;
    0)   echo "no success sentinel" ;;
    *)   echo "exit $1" ;;
  esac
}

PASS=0; FAIL=0; declare -a FAILURES=()

for mode in "${MODES[@]}"; do
  flag=$(mode_flag "$mode")
  outdir="bin/ft_$mode"; mkdir -p "$outdir"
  echo "================ MODE: $mode ${flag:+($flag)} ================"
  for src in "${TESTS[@]}"; do
    [[ -f "$src" ]] || { echo "[skip] $src (not found)"; continue; }
    name=$(basename "$src" .cpp)
    bin="$outdir/$name"
    printf "  [build] %-28s ... " "$name"
    if $DUCK build "$src" -o "$outdir" ${flag:+$flag} >"$bin.build.log" 2>&1; then
      echo "ok"
    else
      echo "FAIL (build)"; FAIL=$((FAIL+1)); FAILURES+=("$name [$mode] build")
      grep -iE 'error:|undefined reference' "$bin.build.log" | head -8
      continue
    fi
    printf "  [run]   %-28s ... " "$name"
    "$bin" >"$bin.run.log" 2>&1; rc=$?
    if [[ $rc -eq 1 ]]; then
      echo "PASS"; PASS=$((PASS+1))
    else
      echo "FAIL ($(decode $rc))"; FAIL=$((FAIL+1)); FAILURES+=("$name [$mode] run=$(decode $rc)")
      tail -20 "$bin.run.log"
    fi
  done
  echo ""
done

echo "=== freestanding_threads.sh summary ==="
echo "  passed: $PASS"
echo "  failed: $FAIL"
if [[ $FAIL -gt 0 ]]; then
  printf '    - %s\n' "${FAILURES[@]}"
  exit 1
fi
exit 0
