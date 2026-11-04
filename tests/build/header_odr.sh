#!/usr/bin/env bash
# Header-only multi-TU link/runtime gate; uses source headers and CRT without installing either.
# Usage: CXX=clang++ bash tests/build/header_odr.sh (repeat with CXX=g++).
set -euo pipefail
cd "$(dirname "$0")/../.."
cxx=${CXX:-clang++}
out=$(mktemp -d "${TMPDIR:-/tmp}/micron-header-odr.XXXXXX")
trap 'rm -rf "$out"' EXIT
read -r -a extra_flags <<< "${CXXFLAGS:-}"
common=(-std=c++23 -I. -ffunction-sections -fdata-sections "${extra_flags[@]}")
sources=(first second foreign)

check_run() {
  local expected=$1 binary=$2 rc=0
  "$binary" || rc=$?
  if [[ $rc -ne $expected ]]; then
    printf 'FAIL: %s exited %s, expected %s\n' "$binary" "$rc" "$expected" >&2
    exit 1
  fi
}

for opt in -O0 -O2; do
  for part in "${sources[@]}"; do
    "$cxx" "${common[@]}" "$opt" -c "tests/build/header_odr_$part.cpp" -o "$out/$part.o"
  done
  "$cxx" "$out/first.o" "$out/second.o" "$out/foreign.o" -Wl,--gc-sections -o "$out/hosted"
  check_run 1 "$out/hosted"
  "$cxx" "$out/foreign.o" "$out/second.o" "$out/first.o" -Wl,--gc-sections -o "$out/hosted-reverse"
  check_run 1 "$out/hosted-reverse"
  printf 'PASS %s hosted %s both link orders\n' "$cxx" "$opt"
done

# An identity mismatch must fail independently of successful linking.
"$cxx" "${common[@]}" -O2 -DMICRON_ODR_NEGATIVE_CONTROL -c tests/build/header_odr_second.cpp -o "$out/negative.o"
"$cxx" "$out/first.o" "$out/negative.o" "$out/foreign.o" -o "$out/negative"
check_run 6 "$out/negative"
printf 'PASS %s negative control\n' "$cxx"

freestanding=(-O2 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pie)
for part in "${sources[@]}"; do
  "$cxx" "${common[@]}" "${freestanding[@]}" -c "tests/build/header_odr_$part.cpp" -o "$out/$part.o"
done
"$cxx" "${common[@]}" "${freestanding[@]}" -c start/start.cpp -o "$out/crt.o"
"$cxx" -c start/start.s -o "$out/entry.o"
link=(-nostdlib -static -no-pie -Wl,--gc-sections)
"$cxx" "${link[@]}" "$out/entry.o" "$out/crt.o" "$out/first.o" "$out/second.o" "$out/foreign.o" -o "$out/freestanding"
check_run 1 "$out/freestanding"
"$cxx" "${link[@]}" "$out/foreign.o" "$out/second.o" "$out/first.o" "$out/crt.o" "$out/entry.o" -o "$out/freestanding-reverse"
check_run 1 "$out/freestanding-reverse"
printf 'PASS %s freestanding both CRT link orders\n' "$cxx"

# A program with no io headers must still link/run with absent optional boot/shutdown callbacks.
printf 'extern "C" int main() { return 1; }\n' > "$out/no-io.cpp"
"$cxx" "${common[@]}" "${freestanding[@]}" -c "$out/no-io.cpp" -o "$out/no-io.o"
"$cxx" "${link[@]}" "$out/entry.o" "$out/crt.o" "$out/no-io.o" -o "$out/no-io"
check_run 1 "$out/no-io"
printf 'PASS %s freestanding absent optional io callbacks\n' "$cxx"
