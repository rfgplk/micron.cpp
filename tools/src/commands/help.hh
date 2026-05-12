#pragma once

#include "io/console.hpp"

template <typename T = void>
void
help(void)
  requires(recipes::__using_gnu)
{
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // banner
  mc::set_color(mc::color::cyan);
  mc::console("duck:  a no-build C / C++ / asm compile driver for the micron toolchain");
  mc::set_color(mc::color::reset);
  mc::console("");
  mc::console("Drives gcc/g++, clang/clang++ or nasm directly. No Makefiles, no CMake;");
  mc::console("recipes are baked in. Source is the only required argument.");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // usage
  mc::set_color(mc::color::yellow);
  mc::console("USAGE");
  mc::set_color(mc::color::reset);
  mc::console("    duck <command> [flags] <source|dir|object> ...");
  mc::console("    duck help | --help | -h");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // commands
  mc::set_color(mc::color::yellow);
  mc::console("COMMANDS");
  mc::set_color(mc::color::reset);
  mc::console("    build      compile + link, serially; waits for each invocation");
  mc::console("    compile    compile, do NOT wait for processes (parallel-ish)");
  mc::console("    link       compile + link without waiting (alias of compile today)");
  mc::console("    debug      shorthand for `build` with the debug recipe (-g, -w, -O0)");
  mc::console("    run        build a single source then exec the resulting binary,");
  mc::console("               replacing the duck process (one source only)");
  mc::console("    emulate    build a single source then exec the resulting binary via qemu-arm-static,");
  mc::console("               replacing the duck process (one source only)");
  mc::console("    test       build sources, run each, print exit codes (CI mode)");
  mc::console("    doctor     build with diagnostic flags (-ftime-report,");
  mc::console("               -ftime-report-details, -fmem-report, -fopt-info,");
  mc::console("               -fopt-info-missed):  useful for compile-time profiling");
  mc::console("    batch      execute a script of duck command lines (one per line)");
  mc::console("    make       scaffold a new project from template          (reserved)");
  mc::console("    recipes    list/select build recipes                     (reserved)");
  mc::console("    help       print this screen");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // sources
  mc::set_color(mc::color::yellow);
  mc::console("SOURCES");
  mc::set_color(mc::color::reset);
  mc::console("    <file>            single source file");
  mc::console("                      C++:  .cpp .cc .cxx .c++ .cp .C .ii");
  mc::console("                      C:    .c  .i");
  mc::console("                      asm:  .s .S .asm .ASM   (auto-selects nasm)");
  mc::console("    <dir>             every C/C++ source matched in the directory");
  mc::console("                      (each file produces its own config and build)");
  mc::console("    <file>.o          additional object linked into the final binary");
  mc::console("    <file>.bin        explicit output target name");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // flags - output
  mc::set_color(mc::color::yellow);
  mc::console("FLAGS:  output");
  mc::set_color(mc::color::reset);
  mc::console("    -o <dir>          output directory (default: bin/, created on demand)");
  mc::console("    --obj             emit a relocatable object   (-c, .o suffix)");
  mc::console("    --asm             emit assembly               (-S, .s suffix)");
  mc::console("    --pp              preprocess only             (-E)");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // flags - toolchain
  mc::set_color(mc::color::yellow);
  mc::console("FLAGS:  toolchain");
  mc::set_color(mc::color::reset);
  mc::console("    --gcc             use gcc / g++                            (default)");
  mc::console("    --clang           use clang / clang++");
  mc::console("    --nasm            use NASM (also implied by .asm/.s sources)");
  mc::console("    -c                treat source as C");
  mc::console("    -cpp              treat source as C++");
  mc::console("    -asm              treat source as NASM assembly");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // flags - architecture
  mc::set_color(mc::color::yellow);
  mc::console("FLAGS:  architecture");
  mc::set_color(mc::color::reset);
  mc::console("    --x86             target x86                                (default)");
  mc::console("    --arm             target ARM via Linaro cross compiler");
  mc::console("                      (armv7-a, NEON, hard-float)");
  mc::console("    -32               32-bit ABI                  (-m32 on x86)");
  mc::console("    -64               64-bit ABI                  (-m64 on x86, default)");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // flags - language standard
  mc::set_color(mc::color::yellow);
  mc::console("FLAGS:  language standard");
  mc::set_color(mc::color::reset);
  mc::console("    --std <suffix>    standard suffix passed to -std=...");
  mc::console("                      C:    c90 c99 c11 c17 c18 c23  (+ iso9899:* + gnuNN)");
  mc::console("                      C++:  c++98 c++03 c++11 c++14 c++17 c++20 c++23 c++26");
  mc::console("                            (+ gnu++NN)");
  mc::console("                      Defaults: -std=c++26 (C++), -std=c11 (C).");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // flags - optimization
  mc::set_color(mc::color::yellow);
  mc::console("FLAGS:  optimization");
  mc::set_color(mc::color::reset);
  mc::console("    -O0 -O1 -O2 -O3   gcc optimization levels");
  mc::console("    -Ofast            aggressive optimization                   (default)");
  mc::console("    -Oz               optimize for size");
  mc::console("    -d, -g            debug build; switches to the debug recipe");
  mc::console("                      and forces -O0 unless an explicit -O*    is given");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // flags - codegen
  mc::set_color(mc::color::yellow);
  mc::console("FLAGS:  codegen / hardening");
  mc::set_color(mc::color::reset);
  mc::console("    -w                extended warnings: Wall Wextra Wpedantic plus a long");
  mc::console("                      list (Wshadow, Wuseless-cast, Wlogical-op, Wcast-*,");
  mc::console("                      Wfloat-*, Wduplicated-*, Wnull-dereference, ...)");
  mc::console("                      and -Werror=missing-field-initializers,return-type");
  mc::console("    -s                static binary    (-static, static-libgcc/-libstdc++)");
  mc::console("    -k                freestanding / kernel build");
  mc::console("                      (-ffreestanding, -nostdlib, -nostdlib++; drops");
  mc::console("                      default -lpthread linkage)");
  mc::console("    -f                force build:  skip include-mtime change detection");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // flags - includes/libs
  mc::set_color(mc::color::yellow);
  mc::console("FLAGS:  includes / libs");
  mc::set_color(mc::color::reset);
  mc::console("    -i <dir>          include search path        (default: ./src)");
  mc::console("    -l <dir>          library search path        (default: ./libs/)");
  mc::console("    --lib <name>      link library by name       (-l<name>)");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // batch file format
  mc::set_color(mc::color::yellow);
  mc::console("BATCHFILE FORMAT  (duck batch <file>)");
  mc::set_color(mc::color::reset);
  mc::console("    Each non-empty, non-comment line is parsed as a duck invocation:");
  mc::console("        <command> <args...>           # 'duck' itself is implicit");
  mc::console("    '#' starts a line comment. Blank lines are ignored. Tokens are");
  mc::console("    whitespace-separated. Lines run sequentially in declaration order.");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // built-in recipes
  mc::set_color(mc::color::yellow);
  mc::console("BUILT-IN RECIPES");
  mc::set_color(mc::color::reset);
  mc::console("    Optimized recipe (default) adds:");
  mc::console("        -mavx2 -mbmi -march=native -flto=8");
  mc::console("        -fmodulo-sched -fmodulo-sched-allow-regmoves");
  mc::console("        -fgcse-sm -fgcse-las");
  mc::console("    Debug recipe adds:");
  mc::console("        -g3 -ggdb3 -gcolumn-info -ginline-points -gstatement-frontiers");
  mc::console("        -march=native");
  mc::console("    Always applied:");
  mc::console("        -fstack-protector-strong -fstack-clash-protection");
  mc::console("        -fstrict-overflow");
  mc::console("        -Wno-variadic-macros -Wno-inline");
  mc::console("        C++: -fext-numeric-literals  -fdiagnostics-color=always");
  mc::console("             -fconcepts-diagnostics-depth=2");
  mc::console("    Doctor adds:");
  mc::console("        -ftime-report -ftime-report-details -fmem-report");
  mc::console("        -fopt-info -fopt-info-missed");
  mc::console("    ARM recipe adds:");
  mc::console("        -march=armv7-a -mfpu=neon -mfloat-abi=hard");
  mc::console("    NASM recipe:");
  mc::console("        -f elf64 -I<include path>");
  mc::console("    Default link libs (unless -k/freestanding):");
  mc::console("        -lpthread");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // examples
  mc::set_color(mc::color::yellow);
  mc::console("EXAMPLES");
  mc::set_color(mc::color::reset);
  mc::console("    duck build tests/rigor/iarray.cpp");
  mc::console("    duck build src/                       # every source under src/");
  mc::console("    duck run examples/hello.cpp           # build + exec in place");
  mc::console("    duck debug src/main.cpp -w");
  mc::console("    duck test tests/rigor/                # CI-style summary");
  mc::console("    duck compile -O3 -s src/server.cpp -o build/");
  mc::console("    duck doctor src/heavy_template.cpp    # compile-time diagnostics");
  mc::console("    duck build src/boot.S --asm -o boot/  # nasm assembly path");
  mc::console("    duck build src/k.cpp -k -s --std c++23  # freestanding static");
  mc::console("    duck batch scripts/build_all.duck     # script of duck commands");
}
