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
echo "[malformed lines are rejected, not absorbed]"
must_fail "unknown flag"          build t.cpp --urign
must_fail "gcc-style glued -I"    build t.cpp -I./inc1
must_fail "glued -i"              build t.cpp -i./inc1
must_fail "two sources"           build t.cpp u.cpp
must_fail "no source at all"      build -i inc1 -o out
must_fail "--recursive on a file" build t.cpp --recursive
must_fail "--recursive on run"    run t.cpp --recursive

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
echo
echo "passed: $pass   failed: $fail"
[ "$fail" -eq 0 ]
