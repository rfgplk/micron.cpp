#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cxx=${CXX:-g++}
out=$(mktemp -d "${TMPDIR:-/tmp}/micron-fft-odr.XXXXXX")
trap 'rm -rf "$out"' EXIT
flags=(-std=c++23 -O2 -march=x86-64 -I. -ffunction-sections -fdata-sections)
check() {
  local rc=0
  "$1" || rc=$?
  [[ $rc == 1 ]] || { printf 'FAIL: FFT ODR exit %s\n' "$rc"; exit 1; }
}
for mode in hosted freestanding; do
  extra=()
  if [[ $mode == freestanding ]]; then extra=(-ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pie); fi
  for part in first second; do "$cxx" "${flags[@]}" "${extra[@]}" -c "tests/build/fft_odr_$part.cpp" -o "$out/$part.o"; done
  link=(-Wl,--gc-sections)
  if [[ $mode == freestanding ]]; then
    "$cxx" "${flags[@]}" "${extra[@]}" -c start/start.cpp -o "$out/crt.o"
    "$cxx" -c start/start.s -o "$out/start.o"
    link+=(-nostdlib -static -no-pie "$out/crt.o" "$out/start.o")
  fi
  "$cxx" "$out/first.o" "$out/second.o" "${link[@]}" -o "$out/test"
  check "$out/test"
  "$cxx" "$out/second.o" "$out/first.o" "${link[@]}" -o "$out/test-reverse"
  check "$out/test-reverse"
  printf 'PASS %s FFT %s both link orders\n' "$cxx" "$mode"
done
