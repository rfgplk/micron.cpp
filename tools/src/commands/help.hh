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
  mc::console("               `build parallel` runs all targets concurrently (see -j)");
  mc::console("    compile    compile all targets concurrently, reap + report at exit");
  mc::console("    link       compile + link concurrently (alias of compile today)");
  mc::console("    debug      shorthand for `build` with the debug recipe (-g, -w, -O0)");
  mc::console("    run        build a single source then exec the resulting binary,");
  mc::console("               replacing the duck process (one source only)");
  mc::console("    emulate    build a single source then run it under user-mode qemu (one source only)");
  mc::console("               requires a cross target: --arm (qemu-arm-static) or --arm64");
  mc::console("               (qemu-aarch64-static). exits with the target's exit code");
  mc::console("    test       build sources, run each (under qemu when cross-targeted)");
  mc::console("               `test parallel` builds then runs everything concurrently");
  mc::console("               targets whose cross toolchain or qemu is absent are skipped, not failed");
  mc::console("    doctor     build with diagnostic flags (-ftime-report,");
  mc::console("               -ftime-report-details, -fmem-report, -fopt-info,");
  mc::console("               -fopt-info-missed):  useful for compile-time profiling");
  mc::console("    batch      execute a script of duck command lines (one per line)");
  mc::console("               `batch parallel` pools build/compile/link lines into one");
  mc::console("               concurrent run; other lines execute after the pool drains");
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
  mc::console("                      asm:  .s .S      GNU as via the gcc driver (arch aware,");
  mc::console("                                       links by default; --obj for object only)");
  mc::console("                            .asm .ASM  NASM (always emits an object, .o suffix)");
  mc::console("    <dir>             every C/C++/asm source matched in the directory");
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
  mc::console("    --obj             emit a relocatable object   (-c, .o suffix; LTO bytecode)");
  mc::console("    --raw-obj         emit a real machine-code object (-c -fno-lto): a genuinely");
  mc::console("                      linkable .o for an external/third-party linker");
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
  mc::console("    --x86             target x86/amd64                          (default)");
  mc::console("    --i386            target 32-bit x86         (== --x86 -32; runs natively)");
  mc::console("    --arm             target ARM via Linaro cross compiler");
  mc::console("                      (armv7-a, NEON, hard-float); run via `emulate`/`test`");
  mc::console("    --arm64           target AArch64 via Linaro cross compiler  (armv8-a)");
  mc::console("    --aarch64         alias for --arm64");
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
  mc::console("    --perf            profile:  -O3 (FP-safe, not -Ofast) + -funroll-loops");
  mc::console("    --minsize         profile:  -Os + section gc (implies --gc)");
  mc::console("    --gc              dead-strip unused sections (-ffunction-sections");
  mc::console("                      -fdata-sections, -Wl,--gc-sections); pairs with -k");
  mc::console("    --unroll          -funroll-loops");
  mc::console("    --pgo-gen         instrument for PGO (-fprofile-generate);  hosted only");
  mc::console("    --pgo-use         use PGO data (-fprofile-use -fprofile-correction)");
  mc::console("    --no-eh           -fno-exceptions   (C++ only; explicit opt-in)");
  mc::console("    --no-rtti         -fno-rtti         (C++ only; explicit opt-in)");
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
  mc::console("    --asan            AddressSanitizer (-fsanitize=address,");
  mc::console("                      -fno-omit-frame-pointer); disables -flto");
  mc::console("    --ubsan           UBSanitizer (-fsanitize=undefined); disables -flto;");
  mc::console("                      combines with --asan");
  mc::console("    --tsan            ThreadSanitizer (-fsanitize=thread); disables -flto;");
  mc::console("                      mutually exclusive with --asan; hosted only");
  mc::console("    --lsan            LeakSanitizer (-fsanitize=leak); subsumed by --asan");
  mc::console("    --cfi             control-flow integrity:  x86 -fcf-protection=full,");
  mc::console("                      arm64 -mbranch-protection=standard, arm -mbranch-protection=pac-ret");
  mc::console("    --fortify         -D_FORTIFY_SOURCE=3 (x86) / =2 (arm); needs -O>0, hosted");
  mc::console("    --spall           -fstack-protector-all (replaces the default -strong)");
  mc::console("    --pie             position-independent executable  (-fPIE -pie)");
  mc::console("    --static-pie      static PIE (-static-pie -fPIE);  conflicts with -s");
  mc::console("    --relro           full RELRO (-Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack)");
  mc::console("    --strip           strip symbols at link            (-Wl,--strip-all)");
  mc::console("    --harden          profile:  --cfi --fortify --pie --relro (collapses");
  mc::console("                      to --cfi --relro under -k; drops --pie under -s)");
  mc::console("    -f                force build:  skip include-mtime change detection");
  mc::console("    -j <N>            cap for the parallel commands (default: online cpus)");
  mc::console("");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // flags - includes/libs
  mc::set_color(mc::color::yellow);
  mc::console("FLAGS:  includes / libs");
  mc::set_color(mc::color::reset);
  mc::console("    -i <dir>          include search path        (default: ./src)");
  mc::console("                      may be passed multiple times; the first -i replaces");
  mc::console("                      the default, subsequent -i's accumulate");
  mc::console("    -l <dir>          library search path        (default: ./libs/)");
  mc::console("                      may be passed multiple times; the first -l replaces");
  mc::console("                      the default, subsequent -l's accumulate");
  mc::console("    --lib <name>      link library by name       (-l<name>)");
  mc::console("    --def <N[=V]>     preprocessor define        (-DN[=V]); repeatable");
  mc::console("    -gl               link the libs micron::gfx::gl needs");
  mc::console("                      (libX11, libGL, libwayland-client, libwayland-egl, libEGL)");
  mc::console("    -vk               link the libs micron::gfx::vk needs");
  mc::console("                      (libX11, libwayland-client, libvulkan)");
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
  mc::console("    A '<file>.duck' path is run as a batch implicitly:");
  mc::console("        duck build.duck   ==   duck batch build.duck");
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
  mc::console("    NASM recipe (.asm):");
  mc::console("        -f elf64|elf32 (-64/-32) -I<include path>; output is always a .o");
  mc::console("    GNU as recipe (.s/.S):");
  mc::console("        gcc driver, arch flags only; links unless --obj/--pp");
  mc::console("    Sanitizers (--asan/--ubsan/--tsan/--lsan) drop -flto=8 (incompatible);");
  mc::console("        --asan and --tsan are mutually exclusive");
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
  mc::console("    duck test parallel tests/rigor/ -j 8  # same, 8 jobs at a time");
  mc::console("    duck build parallel src/              # concurrent build of a dir");
  mc::console("    duck batch parallel build.duck        # batchfile, pooled builds");
  mc::console("    duck compile -O3 -s src/server.cpp -o build/");
  mc::console("    duck doctor src/heavy_template.cpp    # compile-time diagnostics");
  mc::console("    duck build src/boot.s -o boot/        # GNU as via gcc driver");
  mc::console("    duck build src/blob.asm               # nasm path, emits boot/blob.o");
  mc::console("    duck build src/x.cpp --asan --ubsan   # sanitized build, no lto");
  mc::console("    duck build src/k.cpp -k -s --std c++23  # freestanding static");
  mc::console("    duck build src/svc.cpp --harden       # full hosted hardening profile");
  mc::console("    duck build --arm64 src/k.cpp -k --gc --cfi  # freestanding, gc'd, PAC/BTI");
  mc::console("    duck build src/hot.cpp --perf         # -O3 + unroll (FP-safe)");
  mc::console("    duck build.duck                       # implicit batch (== duck batch ...)");
  mc::console("    duck batch scripts/build_all.duck     # script of duck commands");
}
