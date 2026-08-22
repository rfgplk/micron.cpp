#pragma once

#include "all_flags.hh"

namespace clang_flags
{
// Driver and optimization spellings used by duck's Clang profile. The complete
// compiler-generated catalog lives beside this file in all_flags.hh.
constexpr const char *target = "--target=";
constexpr const char *gcc_toolchain = "--gcc-toolchain=";
constexpr const char *sysroot = "--sysroot=";
constexpr const char *ld_path = "--ld-path=";

constexpr const char *optimize_zero = "-O0";
constexpr const char *optimize_one = "-O1";
constexpr const char *optimize_two = "-O2";
constexpr const char *optimize_three = "-O3";
constexpr const char *optimize_size = "-Os";
constexpr const char *optimize_tiny = "-Oz";
constexpr const char *optimize_debug = "-Og";
constexpr const char *fast_math = "-ffast-math";
constexpr const char *thin_lto = "-flto=thin";
constexpr const char *hosted_main = "-fhosted";
constexpr const char *no_builtin = "-fno-builtin";
constexpr const char *micron_freestanding = "-D__micron_freestanding=1";
constexpr const char *no_unwind_tables = "-fno-unwind-tables";
constexpr const char *no_async_unwind_tables = "-fno-asynchronous-unwind-tables";
constexpr const char *gcc_runtime = "-lgcc";

constexpr const char *debug_three = "-g3";
constexpr const char *debug_gdb_three = "-ggdb3";
constexpr const char *debug_columns = "-gcolumn-info";

constexpr const char *remarks_passed = "-Rpass=.*";
constexpr const char *remarks_missed = "-Rpass-missed=.*";
constexpr const char *remarks_analysis = "-Rpass-analysis=.*";

constexpr const char *warnings_extra = "-Wunused -Wshadow -Wnull-dereference -Wconversion -Wcast-qual -Woverlength-strings -Wpointer-arith "
                                       "-Wvarargs -Wvla -Wwrite-strings -Wdouble-promotion -Wcast-function-type-strict -Wformat-security "
                                       "-Wmissing-noreturn -Wpacked -Wnonnull -Wundef -Warray-bounds -Wcast-align -Winit-self -Wnarrowing "
                                       "-Wdeprecated-register -Wunsequenced";
}      // namespace clang_flags
