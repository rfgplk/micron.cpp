#!/bin/sh

/usr/bin/g++ -std=c++23 -Ofast -mavx2 -mbmi -march=native -fmodulo-sched -fmodulo-sched-allow-regmoves -fgcse-sm -fgcse-las -m64 -Wall -Wextra -Wpedantic -Wno-variadic-macros -Wno-inline -fstack-protector-strong -fstack-clash-protection -fstrict-overflow -fext-numeric-literals -flto -Wno-variadic-macros -Wno-inline -fdiagnostics-color=always -fconcepts-diagnostics-depth=2 tools/src/main.cc -lm -lpthread -I./src -L./libs/ -o bin/duck
