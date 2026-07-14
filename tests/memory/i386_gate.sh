#!/bin/sh
# i386 (x86, 32-bit) compile gate for the memory battery.
#
# Hosted -m32 needs glibc-devel.i686 (absent on this host) and the
# freestanding 32-bit runtime has pre-existing link/start issues unrelated
# to mem*, so i386 coverage is compile+codegen only: it catches 64-bit-GPR
# misuse in inline asm, constraint errors, and register-pressure failures.
# Runtime coverage of 32-bit width semantics comes from the armv7 qemu line.
#
# usage: sh tests/memory/i386_gate.sh   (from the repo root; exit 0 = pass)
set -e
FLAGS="-std=c++26 -Ofast -m32 -march=x86-64 -ffreestanding -fno-exceptions -fno-rtti -fext-numeric-literals -Wall -Wextra"
for f in tests/memory/mem_exhaustive_small.cpp \
         tests/memory/mem_overlap_exhaustive.cpp \
         tests/memory/mem_fill_patterns.cpp \
         tests/memory/mem_large_nt.cpp \
         tests/memory/mem_constexpr.cpp \
         tests/memory/free/cabi_shims_freestanding.cpp; do
  echo "i386 gate: $f"
  g++ $FLAGS -c "$f" -I./src -o /dev/null
done
echo "i386 compile gate: PASS"
