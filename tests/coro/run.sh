#!/usr/bin/env bash
#
# coro validation runner: build + run each tests/coro test (hosted) via duck and
# assert the snowball success contract.

set -u
cd "$(dirname "$0")/../.."

DUCK=./bin/duck
[[ -x "$DUCK" ]] || DUCK=duck   # fall back to PATH, but prefer the in-tree one

TESTS=("$@")
if [[ ${#TESTS[@]} -eq 0 ]]; then
  TESTS=(tests/coro/t_*.cpp)
fi

decode() {  # human-readable verdict for a run exit code
  case "$1" in
    1)   echo "PASS" ;;
    6)   echo "require()-fail" ;;
    139) echo "SIGSEGV" ;;
    0)   echo "no success sentinel" ;;
    *)   echo "exit $1" ;;
  esac
}

outdir="bin/coro"; mkdir -p "$outdir"
PASS=0; FAIL=0; declare -a FAILURES=()

for src in "${TESTS[@]}"; do
  [[ -f "$src" ]] || { echo "[skip] $src (not found)"; continue; }
  name=$(basename "$src" .cpp)
  bin="$outdir/$name"
  printf "  [build] %-24s ... " "$name"
  if $DUCK build "$src" -o "$outdir" >"$bin.build.log" 2>&1; then
    echo "ok"
  else
    echo "FAIL (build)"; FAIL=$((FAIL+1)); FAILURES+=("$name build")
    grep -iE 'error:|undefined reference' "$bin.build.log" | head -8
    continue
  fi
  printf "  [run]   %-24s ... " "$name"
  "$bin" >"$bin.run.log" 2>&1; rc=$?
  if [[ $rc -eq 1 ]]; then
    echo "PASS"; PASS=$((PASS+1))
  else
    echo "FAIL ($(decode $rc))"; FAIL=$((FAIL+1)); FAILURES+=("$name run=$(decode $rc)")
    tail -20 "$bin.run.log"
  fi
done

echo ""
echo "=== tests/coro/run.sh summary ==="
echo "  passed: $PASS"
echo "  failed: $FAIL"
if [[ $FAIL -gt 0 ]]; then
  printf '    - %s\n' "${FAILURES[@]}"
  exit 1
fi
exit 0
