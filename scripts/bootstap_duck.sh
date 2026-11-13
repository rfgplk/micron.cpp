#!/bin/sh

set -eu

duck_cxx=${CXX:-/usr/bin/g++}
mkdir -p bin
duck_build_output=$(mktemp bin/.duck.XXXXXXXX)
trap 'rm -f "$duck_build_output"' EXIT HUP INT TERM

case "$duck_cxx" in
*clang++*)
  "$duck_cxx" -std=c++23 -O3 -ffast-math -march=native -m64 -Wall -Wextra -Wpedantic \
    -Wno-variadic-macros -Wno-inline -fstack-protector-strong -fstack-clash-protection \
    -fstrict-overflow -flto=thin -fdiagnostics-color=always tools/src/main.cc -lm -lpthread \
    -I. -I./src -L./libs/ -o "$duck_build_output"
  ;;
*)
  "$duck_cxx" -std=c++23 -Ofast -march=native -fmodulo-sched -fmodulo-sched-allow-regmoves \
    -fgcse-sm -fgcse-las -m64 -Wall -Wextra -Wpedantic -Wno-variadic-macros -Wno-inline \
    -fstack-protector-strong -fstack-clash-protection -fstrict-overflow -fext-numeric-literals \
    -flto=8 -fdiagnostics-color=always -fconcepts-diagnostics-depth=2 tools/src/main.cc -lm \
    -lpthread -I. -I./src -L./libs/ -o "$duck_build_output"
  ;;
esac
mv -f "$duck_build_output" bin/duck
