#pragma once

#include "flags.hh"
#include "io/console.hpp"
#include "io/fsys.hpp"
#include "io/ftw.hpp"
#include "linux/process/environ.hpp"
#include "memory/addr.hpp"
#include "vector/vector.hpp"

#include "../../splat.hh"

namespace recipes
{
namespace gnu
{

inline bool
__is_cpp_standard(const string_type &__std)
{
  for ( usize i = 1; i < __std.size(); ++i )
    if ( __std[i - 1] == '+' and __std[i] == '+' ) return true;
  return false;
}

inline bool
__is_cxx26_standard(const string_type &__std)
{
  return __std == gcc::__standard_cxx26 or __std == gcc::__standard_gnuxx26;
}

enum __arch : u32 { x86 = 0, arm, arm64 };

// x86 ISA level
//   base -> -march=x86-64
//   v2   -> -march=x86-64-v2
//   v3   -> -march=x86-64-v3
//   v4   -> -march=x86-64-v4
//   native -> -march=native
enum __isa : u32 { native = 0, base, v2, v3, v4 };

enum __compilers : u32 { gnucc = 0, clang, nasm };

inline const char *
__isa_march(u32 isa)
{
  switch ( isa ) {
  case __isa::base:
    return gcc::x86_flags::get_string_flag(gcc::x86_flags::flags::march_x86_64);
  case __isa::v2:
    return gcc::x86_flags::get_string_flag(gcc::x86_flags::flags::march_x86_64_v2);
  case __isa::v3:
    return gcc::x86_flags::get_string_flag(gcc::x86_flags::flags::march_x86_64_v3);
  case __isa::v4:
    return gcc::x86_flags::get_string_flag(gcc::x86_flags::flags::march_x86_64_v4);
  default:
    return gcc::x86_flags::get_string_flag(gcc::x86_flags::flags::march_native);
  }
}

enum __languages : u32 { cpp = 0, c, lasm, gas };

enum __opt_modes : u32 { optimized = 0, debug };

enum __comp_type : u32 { linked = 0, object, assembly, preprocessed };

constexpr const string_type __compiler_gcc = "/usr/bin/gcc";
constexpr const string_type __compiler_clang = "/usr/bin/clang";
constexpr const string_type __compiler_gpp = "/usr/bin/g++";
constexpr const string_type __compiler_clangpp = "/usr/bin/clang++";
constexpr const string_type __assembler_nasm = "/usr/bin/nasm";

// Fedora cross compiler paths (linaro sourced, not standard!)
constexpr const string_type __compiler_gcc_arm_cross = "/usr/gcc-linaro/bin/arm-none-linux-gnueabihf-gcc";
constexpr const string_type __compiler_gpp_arm_cross = "/usr/gcc-linaro/bin/arm-none-linux-gnueabihf-c++";
// aarch64 (arm64) linaro cross compilers
constexpr const string_type __compiler_gcc_aarch64_cross = "/usr/gcc-linaro-aarch64/bin/aarch64-none-linux-gnu-gcc";
constexpr const string_type __compiler_gpp_aarch64_cross = "/usr/gcc-linaro-aarch64/bin/aarch64-none-linux-gnu-g++";

constexpr const string_type __mc_start_default = "/usr/src/mc_start";

struct std_entry {
  const string_type *full;
  const char *suffix;
};

constexpr std_entry std_table[] = { { gcc::__standard_c90.addr(), gcc::__standard_c90.begin() + 5 },
                                    { gcc::__standard_iso9899_1990.addr(), gcc::__standard_iso9899_1990.begin() + 5 },
                                    { gcc::__standard_iso9899_199409.addr(), gcc::__standard_iso9899_199409.begin() + 5 },
                                    { gcc::__standard_c99.addr(), gcc::__standard_c99.begin() + 5 },
                                    { gcc::__standard_iso9899_1999.addr(), gcc::__standard_iso9899_1999.begin() + 5 },
                                    { gcc::__standard_c11.addr(), gcc::__standard_c11.begin() + 5 },
                                    { gcc::__standard_iso9899_2011.addr(), gcc::__standard_iso9899_2011.begin() + 5 },
                                    { gcc::__standard_c17.addr(), gcc::__standard_c17.begin() + 5 },
                                    { gcc::__standard_c18.addr(), gcc::__standard_c18.begin() + 5 },
                                    { gcc::__standard_iso9899_2017.addr(), gcc::__standard_iso9899_2017.begin() + 5 },
                                    { gcc::__standard_c23.addr(), gcc::__standard_c23.begin() + 5 },
                                    { gcc::__standard_iso9899_2023.addr(), gcc::__standard_iso9899_2023.begin() + 5 },

                                    { gcc::__standard_gnu90.addr(), gcc::__standard_gnu90.begin() + 5 },
                                    { gcc::__standard_gnu89.addr(), gcc::__standard_gnu89.begin() + 5 },
                                    { gcc::__standard_gnu99.addr(), gcc::__standard_gnu99.begin() + 5 },
                                    { gcc::__standard_gnu11.addr(), gcc::__standard_gnu11.begin() + 5 },
                                    { gcc::__standard_gnu17.addr(), gcc::__standard_gnu17.begin() + 5 },
                                    { gcc::__standard_gnu18.addr(), gcc::__standard_gnu18.begin() + 5 },
                                    { gcc::__standard_gnu23.addr(), gcc::__standard_gnu23.begin() + 5 },

                                    { gcc::__standard_cxx98.addr(), gcc::__standard_cxx98.begin() + 5 },
                                    { gcc::__standard_cxx03.addr(), gcc::__standard_cxx03.begin() + 5 },
                                    { gcc::__standard_cxx11.addr(), gcc::__standard_cxx11.begin() + 5 },
                                    { gcc::__standard_cxx14.addr(), gcc::__standard_cxx14.begin() + 5 },
                                    { gcc::__standard_cxx17.addr(), gcc::__standard_cxx17.begin() + 5 },
                                    { gcc::__standard_cxx20.addr(), gcc::__standard_cxx20.begin() + 5 },
                                    { gcc::__standard_cxx23.addr(), gcc::__standard_cxx23.begin() + 5 },
                                    { gcc::__standard_cxx26.addr(), gcc::__standard_cxx26.begin() + 5 },

                                    { gcc::__standard_gnuxx98.addr(), gcc::__standard_gnuxx98.begin() + 5 },
                                    { gcc::__standard_gnuxx03.addr(), gcc::__standard_gnuxx03.begin() + 5 },
                                    { gcc::__standard_gnuxx11.addr(), gcc::__standard_gnuxx11.begin() + 5 },
                                    { gcc::__standard_gnuxx14.addr(), gcc::__standard_gnuxx14.begin() + 5 },
                                    { gcc::__standard_gnuxx17.addr(), gcc::__standard_gnuxx17.begin() + 5 },
                                    { gcc::__standard_gnuxx20.addr(), gcc::__standard_gnuxx20.begin() + 5 },
                                    { gcc::__standard_gnuxx23.addr(), gcc::__standard_gnuxx23.begin() + 5 },
                                    { gcc::__standard_gnuxx26.addr(), gcc::__standard_gnuxx26.begin() + 5 },

                                    { gcc::__standard_c9x.addr(), gcc::__standard_c9x.begin() + 5 },
                                    { gcc::__standard_gnu9x.addr(), gcc::__standard_gnu9x.begin() + 5 },
                                    { gcc::__standard_c1x.addr(), gcc::__standard_c1x.begin() + 5 },
                                    { gcc::__standard_gnu1x.addr(), gcc::__standard_gnu1x.begin() + 5 },
                                    { gcc::__standard_c2x.addr(), gcc::__standard_c2x.begin() + 5 },
                                    { gcc::__standard_gnu2x.addr(), gcc::__standard_gnu2x.begin() + 5 } };

struct config_t {
  string_type compiler_path;
  string_type target;
  string_type target_out;
  string_type bin_dir;
  string_type standard;
  string_type start_dir;      // --start <dir> / MICRON_START; empty until finalize_and_infer resolves it
  mc::vector<string_type> include_path;
  mc::vector<string_type> lib_path;
  mc::vector<string_type> bonus_objs;
  mc::vector<string_type> bonus_libs;
  mc::vector<string_type> defines;
  bool warnings = false;
  bool doctor = false;
  bool static_binary = false;
  bool freestanding = false;
  bool freestanding_eh = false;      // freestanding + the micron C++ exception trampoline
  bool direct = false;               // --direct: link direct*.s (enters __micron_directc, no runtime init)
  bool asan = false;
  bool ubsan = false;
  bool tsan = false;
  bool lsan = false;
  bool cfi = false;          // x86 -fcf-protection=full / arm -mbranch-protection=
  bool fortify = false;      // -D_FORTIFY_SOURCE=2|3
  bool pie = false;
  bool static_pie = false;               // -static-pie -fPIE
  bool relro = false;                    // -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
  bool gc = false;                       // -ffunction-sections -fdata-sections -Wl,--gc-sections
  bool spall = true;                     // -fstack-protector-all
  bool no_ssp = false;                   // --no-ssp: -fno-stack-protector (overrides spall)
  bool no_lto = false;                   // --no-lto: drop the otherwise-default -flto=8
  bool frame_pointer = false;            // --fp: -fno-omit-frame-pointer (perf --call-graph fp)
  bool unroll = false;                   // -funroll-loops
  bool pgo_gen = false;                  // -fprofile-generate
  bool pgo_use = false;                  // -fprofile-use
  bool no_eh = false;                    // -fno-exceptions
  bool no_rtti = false;                  // -fno-rtti
  bool reflection = false;               // -freflection: c++26 p2996 reflection (gcc 16+, c++26 only)
  bool strip = false;                    // -Wl,--strip-all
  bool check_compileability = true;      // check include paths for updates - default true
  bool raw_object = false;               // --raw-obj: a real machine-code object (no LTO) for an external linker
  bool opnames = false;                  // -fno-operator-names: disables support for operator names in the standard (xor and or et al)
  u32 jobs{ 0 };                         // 0 = default to online cpu count when running parallel
  u32 timeout{ 0 };                      // --timeout <sec>
                                         // 0 = wait forever, which is duck's historical behavior
  u32 compile_type{ __comp_type::linked };
  u32 width{ 64 };
  u32 arch{ __arch::x86 };
  u32 isa{ __isa::native };      // x86 only; --isa base|v2|v3|v4|native
  u32 compiler{ __compilers::gnucc };
  u32 language{ __languages::cpp };
  u32 mode{ __opt_modes::optimized };
  gcc::opt_flags::flags opt_mode{ gcc::opt_flags::flags::optimize_fast };
};

inline bool
__is_flag(const char *a)
{
  return a != nullptr and a[0] == '-' and a[1] != '\0';
}

inline bool
__flag_takes_value(const char *a)
{
  return mc::strcmp(a, "-o") == 0 or mc::strcmp(a, "-i") == 0 or mc::strcmp(a, "-l") == 0 or mc::strcmp(a, "--lib") == 0
         or mc::strcmp(a, "--def") == 0 or mc::strcmp(a, "--std") == 0 or mc::strcmp(a, "--isa") == 0 or mc::strcmp(a, "-j") == 0
         or mc::strcmp(a, "--timeout") == 0 or mc::strcmp(a, "--start") == 0;
}

inline int
__find_source(int argc, char **argv)
{
  for ( int i = 0; i < argc; ++i ) {
    if ( __is_flag(argv[i]) ) {
      if ( __flag_takes_value(argv[i]) ) ++i;      // step over its argument too
      continue;
    }
    return i;
  }
  return -1;
}

inline bool
__has_flag(int argc, char **argv, const char *flag)
{
  for ( int i = 0; i < argc; ++i ) {
    if ( __is_flag(argv[i]) ) {
      if ( mc::strcmp(argv[i], flag) == 0 ) return true;
      if ( __flag_takes_value(argv[i]) ) ++i;      // don't match a flag's own argument
    }
  }
  return false;
}

inline bool
__determine_source(config_t &conf, string_type &&str)
{
  auto itr = mc::format::find_reverse(str, str.end() - 1, ".");
  if ( itr == nullptr ) {
    conf.target_out = str;
    return true;
  } else {
    auto sstr = str.substr(itr);
    if ( sstr == ".bin" ) {
      conf.target_out = str;
      return true;
    } else if ( sstr == ".o" ) {
      conf.bonus_objs.emplace_back(str);
      return false;
    }
  }
  // used to be dropped without a word
  mc::cerror("unexpected argument '", str, "': one source per invocation - pass a directory, or use a batchfile");
  return false;
}

inline void
__make_name(config_t &conf)
{
  if ( conf.target.empty() ) return;
  auto itr = mc::format::find_reverse(conf.target, conf.target.end() - 1, ".");
  if ( itr == nullptr )
    itr = conf.target.end();
  else {
    auto sstr = conf.target.substr(itr);
    if ( sstr == ".cpp" or sstr == ".cp" or sstr == ".cc" or sstr == ".cxx" or sstr == ".c++" or sstr == ".ii" or sstr == ".C" )
      conf.language = __languages::cpp;
    else if ( sstr == ".c" or sstr == ".i" )
      conf.language = __languages::c;
    else if ( sstr == ".s" or sstr == ".S" ) {
      // GNU as syntax - assembled (and linked) through the gcc driver, arch/cross aware
      conf.language = __languages::gas;
    } else if ( sstr == ".asm" or sstr == ".ASM" ) {
      conf.language = __languages::lasm;
      conf.compiler = __compilers::nasm;
    }
  }
  auto itr_2 = mc::format::find_reverse(conf.target, itr, "/");
  if ( itr_2 == nullptr )
    itr_2 = conf.target.begin();
  else
    itr_2++;
  conf.target_out = string_type{ itr_2, itr };
}

inline void
finalize_and_infer(config_t &conf, bool user_provided_out, bool user_provided_type, bool user_provided_opt)
{
  if ( conf.mode == __opt_modes::debug && !user_provided_opt ) conf.opt_mode = gcc::opt_flags::flags::optimize_zero;

  conf.target_out.insert(conf.target_out.begin(), conf.bin_dir);

  if ( !user_provided_out ) {
    if ( conf.bonus_objs.empty() ) {
      if ( conf.compile_type == __comp_type::object )
        conf.target_out.append(".o");
      else if ( conf.compile_type == __comp_type::assembly )
        conf.target_out.append(".s");
      else if ( conf.compiler == __compilers::nasm )
        conf.target_out.append(".o");      // nasm always emits an object, never a linked binary
    } else {
      conf.target_out = *conf.bonus_objs.last();
      conf.bonus_objs.pop_back();
    }
  }

  if ( !user_provided_type ) {
    auto itr = mc::format::find_reverse(conf.target_out, conf.target_out.end() - 1, ".");
    if ( itr != nullptr ) {
      auto sstr = conf.target_out.substr(itr);
      if ( sstr == ".o" )
        conf.compile_type = __comp_type::object;
      else if ( sstr == ".bin" )
        conf.compile_type = __comp_type::linked;
      else if ( sstr == ".a" ) {
        // TODO: implement static outs
      } else if ( sstr == ".so" ) {
        // TODO: implement shared outs
      }
    }
  }

  switch ( conf.compiler ) {
  case __compilers::nasm:
    conf.compiler_path = __assembler_nasm;
    break;
  case __compilers::gnucc:
    if ( conf.arch == __arch::x86 ) {
      if ( conf.language == __languages::c or conf.language == __languages::gas ) {
        conf.compiler_path = __compiler_gcc;
        // we'll default to c11 since it's reasonable
        if ( conf.language == __languages::c ) conf.standard = gcc::__standard_c11;
      } else if ( conf.language == __languages::cpp )
        conf.compiler_path = __compiler_gpp;
    } else if ( conf.arch == __arch::arm ) {
      if ( conf.language == __languages::c or conf.language == __languages::gas ) {
        conf.compiler_path = __compiler_gcc_arm_cross;
        if ( conf.language == __languages::c ) conf.standard = gcc::__standard_c11;
      } else if ( conf.language == __languages::cpp )
        conf.compiler_path = __compiler_gpp_arm_cross;
    } else if ( conf.arch == __arch::arm64 ) {
      if ( conf.language == __languages::c or conf.language == __languages::gas ) {
        conf.compiler_path = __compiler_gcc_aarch64_cross;
        if ( conf.language == __languages::c ) conf.standard = gcc::__standard_c11;
      } else if ( conf.language == __languages::cpp )
        conf.compiler_path = __compiler_gpp_aarch64_cross;
    }
    break;
  case __compilers::clang:
    if ( conf.language == __languages::c ) {
      conf.compiler_path = __compiler_clang;
      conf.standard = gcc::__standard_c11;
    } else if ( conf.language == __languages::cpp )
      conf.compiler_path = __compiler_clangpp;
    break;
  }

  if ( conf.reflection ) {
    if ( conf.language != __languages::cpp )
      mc::cerror("-freflection is a C++ flag, but the target is not C++");
    if ( !__is_cxx26_standard(conf.standard) )
      mc::cerror("-freflection requires --std c++26 or gnu++26");
    if ( conf.compiler == __compilers::clang )
      mc::cerror("-freflection is the gcc spelling; clang gates reflection behind a different flag");
  }

  // --start/--direct are freestanding-only knobs. MICRON_START is deliberately NOT validated here:
  // an env var applies to a whole run and must stay inert on the hosted lines of a batchfile
  if ( !conf.start_dir.empty() and !conf.freestanding )
    mc::cerror("--start only means something on a freestanding build - add -k (or -ke)");
  if ( conf.direct and !conf.freestanding )
    mc::cerror("--direct only means something on a freestanding build - add -k (or -ke)");
  if ( conf.direct and conf.static_pie and conf.arch == __arch::x86 )
    mc::cerror("--direct has nothing to swap: a static-PIE x86 freestanding image links no _start stub");

  // the crt location: --start > MICRON_START > the built-in /usr/src/mc_start
  if ( conf.start_dir.empty() ) {
    const char *__start_env = mc::env_get("MICRON_START");
    conf.start_dir = (__start_env != nullptr and *__start_env != '\0') ? string_type{ __start_env } : __mc_start_default;
  }
  // normalized once, for all three sources; batch.hh joins by plain concatenation
  if ( *(conf.start_dir.end() - 1) != '/' ) conf.start_dir.insert(conf.start_dir.end(), '/');
}

// create every missing component of a (trailing-slash) directory path
inline void
__mkdir_p(const string_type &dir)
{
  if ( dir.empty() ) return;
  string_type part;
  for ( usize i = 0; i < dir.size(); ++i ) {
    part.insert(part.end(), dir[i]);
    if ( dir[i] != '/' || part.size() == 1 ) continue;      // "/abs" -> don't mkdir("/")
    if ( !mc::posix::exists(part) ) mc::posix::mkdir(part.c_str(), mc::io::perm_dir_default.to_mode());
  }
  if ( !mc::posix::exists(dir) ) mc::posix::mkdir(dir.c_str(), mc::io::perm_dir_default.to_mode());
}

inline void
parse_config(config_t &conf, int argc, char **argv, int source_index)
{
  // initial name
  __make_name(conf);

  bool user_provided_out = false;
  bool user_provided_type = false;
  bool user_provided_opt = false;
  bool user_provided_include = false;
  bool user_provided_lib = false;

  for ( int i = 0; i < argc; ++i ) {
    if ( i == source_index ) continue;
    if ( mc::strcmp(argv[i], "-d") == 0 or mc::strcmp(argv[i], "-g") == 0 ) {
      conf.mode = __opt_modes::debug;
    } else if ( mc::strcmp(argv[i], "-o") == 0 ) {
      if ( ++i >= argc ) mc::cerror("the -o flag must be followed by a path");
      conf.bin_dir = argv[i];
      if ( conf.bin_dir.empty() ) mc::cerror("bin_dir is empty");
      if ( *(conf.bin_dir.end() - 1) != '/' ) conf.bin_dir.insert(conf.bin_dir.end(), '/');
      // NOTE: no __make_name here
    } else if ( mc::strcmp(argv[i], "-32") == 0 ) {
      conf.width = 32;
    } else if ( mc::strcmp(argv[i], "-64") == 0 ) {
      conf.width = 64;
    } else if ( mc::strcmp(argv[i], "--x86") == 0 ) {
      conf.arch = __arch::x86;
    } else if ( mc::strcmp(argv[i], "--i386") == 0 ) {
      conf.arch = __arch::x86;      // i386 == x86 at 32-bit width; runs natively on an amd64 host
      conf.width = 32;
    } else if ( mc::strcmp(argv[i], "--arm") == 0 ) {
      conf.arch = __arch::arm;
    } else if ( mc::strcmp(argv[i], "--arm64") == 0 or mc::strcmp(argv[i], "--aarch64") == 0 ) {
      conf.arch = __arch::arm64;
    } else if ( mc::strcmp(argv[i], "--isa") == 0 ) {
      if ( ++i >= argc ) mc::cerror("the --isa flag must be followed by one of: base, v2, v3, v4, native");
      if ( mc::strcmp(argv[i], "base") == 0 or mc::strcmp(argv[i], "v1") == 0 )
        conf.isa = __isa::base;
      else if ( mc::strcmp(argv[i], "v2") == 0 )
        conf.isa = __isa::v2;
      else if ( mc::strcmp(argv[i], "v3") == 0 )
        conf.isa = __isa::v3;
      else if ( mc::strcmp(argv[i], "v4") == 0 )
        conf.isa = __isa::v4;
      else if ( mc::strcmp(argv[i], "native") == 0 )
        conf.isa = __isa::native;
      else
        mc::cerror("unknown --isa level (expected base, v2, v3, v4 or native)");
    } else if ( mc::strcmp(argv[i], "-i") == 0 ) {
      if ( ++i >= argc ) mc::cerror("the -i flag must be followed by a path");
      // first user -i replaces the default ./src; subsequent -i flags accumulate
      if ( !user_provided_include ) {
        conf.include_path.clear();
        user_provided_include = true;
      }
      conf.include_path.push_back(argv[i]);
    } else if ( mc::strcmp(argv[i], "-l") == 0 ) {
      if ( ++i >= argc ) mc::cerror("the -l flag must be followed by a path");
      // first user -l replaces the default ./libs/; subsequent -l flags accumulate
      if ( !user_provided_lib ) {
        conf.lib_path.clear();
        user_provided_lib = true;
      }
      conf.lib_path.push_back(argv[i]);
    } else if ( mc::strcmp(argv[i], "--lib") == 0 ) {
      if ( ++i >= argc ) mc::cerror("the --lib flag must be followed by a library name (-l)name");
      conf.bonus_libs.push_back(argv[i]);
    } else if ( mc::strcmp(argv[i], "-w") == 0 ) {
      conf.warnings = true;
    } else if ( mc::strcmp(argv[i], "-s") == 0 ) {
      conf.static_binary = true;
    } else if ( mc::strcmp(argv[i], "-k") == 0 ) {
      // k for "kernel"
      conf.freestanding = true;
    } else if ( mc::strcmp(argv[i], "--eh") == 0 or mc::strcmp(argv[i], "-ke") == 0 ) {
      conf.freestanding = true;
      conf.freestanding_eh = true;
    } else if ( mc::strcmp(argv[i], "--start") == 0 ) {
      if ( ++i >= argc ) mc::cerror("the --start flag must be followed by a path to a micron crt directory");
      // without this a forgotten value silently eats the next flag, and the failure surfaces two
      // layers away as a missing crt at some nonsense path
      if ( __is_flag(argv[i]) ) mc::cerror("the --start flag wants a path, but got the flag '", argv[i], "'");
      conf.start_dir = argv[i];
      if ( conf.start_dir.empty() ) mc::cerror("--start was given an empty path");
    } else if ( mc::strcmp(argv[i], "--direct") == 0 ) {
      // direct*.s enters __micron_directc: TLS + stack + auxv only. no atexit, no threadpool, no io
      // buffers, no .init_array -- global constructors do NOT run. you boot what you need
      conf.direct = true;
    } else if ( mc::strcmp(argv[i], "-f") == 0 ) {
      conf.check_compileability = false;
    } else if ( mc::strcmp(argv[i], "--recursive") == 0 ) {
    } else if ( mc::strcmp(argv[i], "-j") == 0 ) {
      if ( ++i >= argc ) mc::cerror("the -j flag must be followed by a job count");
      u32 jobs = 0;
      for ( const char *p = argv[i]; *p; ++p ) {
        if ( *p < '0' or *p > '9' ) mc::cerror("the -j flag must be followed by a positive integer");
        jobs = jobs * 10 + static_cast<u32>(*p - '0');
      }
      conf.jobs = jobs ? jobs : 1;
    } else if ( mc::strcmp(argv[i], "--timeout") == 0 ) {
      if ( ++i >= argc ) mc::cerror("the --timeout flag must be followed by a duration in seconds");
      u32 secs = 0;
      for ( const char *p = argv[i]; *p; ++p ) {
        if ( *p < '0' or *p > '9' ) mc::cerror("the --timeout flag must be followed by a positive integer (seconds)");
        secs = secs * 10 + static_cast<u32>(*p - '0');
      }
      conf.timeout = secs;
    } else if ( mc::strcmp(argv[i], "--asan") == 0 ) {
      conf.asan = true;
    } else if ( mc::strcmp(argv[i], "--ubsan") == 0 ) {
      conf.ubsan = true;
    } else if ( mc::strcmp(argv[i], "--tsan") == 0 ) {
      conf.tsan = true;
    } else if ( mc::strcmp(argv[i], "--lsan") == 0 ) {
      conf.lsan = true;
    } else if ( mc::strcmp(argv[i], "--cfi") == 0 ) {
      conf.cfi = true;
    } else if ( mc::strcmp(argv[i], "--fortify") == 0 ) {
      conf.fortify = true;
    } else if ( mc::strcmp(argv[i], "--pie") == 0 ) {
      conf.pie = true;
    } else if ( mc::strcmp(argv[i], "--static-pie") == 0 ) {
      conf.static_pie = true;
    } else if ( mc::strcmp(argv[i], "--relro") == 0 ) {
      conf.relro = true;
    } else if ( mc::strcmp(argv[i], "--gc") == 0 ) {
      conf.gc = true;
    } else if ( mc::strcmp(argv[i], "--spall") == 0 ) {
      conf.spall = true;
    } else if ( mc::strcmp(argv[i], "--no-ssp") == 0 ) {
      conf.no_ssp = true;
    } else if ( mc::strcmp(argv[i], "--no-lto") == 0 ) {
      conf.no_lto = true;
    } else if ( mc::strcmp(argv[i], "--fp") == 0 ) {
      conf.frame_pointer = true;
    } else if ( mc::strcmp(argv[i], "--unroll") == 0 ) {
      conf.unroll = true;
    } else if ( mc::strcmp(argv[i], "--pgo-gen") == 0 ) {
      conf.pgo_gen = true;
    } else if ( mc::strcmp(argv[i], "--pgo-use") == 0 ) {
      conf.pgo_use = true;
    } else if ( mc::strcmp(argv[i], "--no-eh") == 0 ) {
      conf.no_eh = true;
    } else if ( mc::strcmp(argv[i], "--no-rtti") == 0 ) {
      conf.no_rtti = true;
    } else if ( mc::strcmp(argv[i], "--strip") == 0 ) {
      conf.strip = true;
    } else if ( mc::strcmp(argv[i], "--opnames") == 0 ) {
      conf.opnames = true;
    } else if ( mc::strcmp(argv[i], "--harden") == 0 ) {
      // full hardening profile
      conf.cfi = true;
      conf.fortify = true;
      conf.pie = true;
      conf.relro = true;
    } else if ( mc::strcmp(argv[i], "--minsize") == 0 ) {
      conf.gc = true;
      if ( !user_provided_opt ) {
        conf.opt_mode = gcc::opt_flags::flags::optimize_size;
        user_provided_opt = true;
      }
    } else if ( mc::strcmp(argv[i], "--perf") == 0 ) {
      conf.unroll = true;
      if ( !user_provided_opt ) {
        conf.opt_mode = gcc::opt_flags::flags::optimize_three;
        user_provided_opt = true;
      }
    } else if ( mc::strcmp(argv[i], "--uring") == 0 ) {
      conf.defines.push_back(string_type{ "MICRON_CORO_URING" });
    } else if ( mc::strcmp(argv[i], "-freflection") == 0 ) {
      conf.reflection = true;
      conf.defines.push_back(string_type{ "MICRON_REFLECTION" });
    } else if ( mc::strcmp(argv[i], "--def") == 0 ) {
      if ( ++i >= argc ) mc::cerror("the --def flag must be followed by NAME or NAME=VALUE");
      conf.defines.push_back(string_type{ argv[i] });
    } else if ( mc::strcmp(argv[i], "--obj") == 0 ) {
      conf.compile_type = __comp_type::object;
      user_provided_type = true;
    } else if ( mc::strcmp(argv[i], "--raw-obj") == 0 ) {
      conf.compile_type = __comp_type::object;
      conf.raw_object = true;
      user_provided_type = true;
    } else if ( mc::strcmp(argv[i], "--asm") == 0 ) {
      conf.compile_type = __comp_type::assembly;
      user_provided_type = true;
    } else if ( mc::strcmp(argv[i], "--pp") == 0 ) {
      conf.compile_type = __comp_type::preprocessed;
      user_provided_type = true;
    } else if ( mc::strcmp(argv[i], "-c") == 0 ) {
      conf.language = __languages::c;
    } else if ( mc::strcmp(argv[i], "-cpp") == 0 ) {
      conf.language = __languages::cpp;
    } else if ( mc::strcmp(argv[i], "-asm") == 0 ) {
      conf.language = __languages::lasm;
      conf.compiler = __compilers::nasm;
    } else if ( mc::strcmp(argv[i], "--gcc") == 0 ) {
      conf.compiler = __compilers::gnucc;
    } else if ( mc::strcmp(argv[i], "--clang") == 0 ) {
      conf.compiler = __compilers::clang;
    } else if ( mc::strcmp(argv[i], "--nasm") == 0 ) {
      conf.language = __languages::lasm;
      conf.compiler = __compilers::nasm;
    } else if ( mc::strcmp(argv[i], "--std") == 0 ) {
      if ( ++i >= argc ) mc::cerror("the --std flag must be followed by a valid standard type");
      for ( const auto &e : std_table ) {
        if ( mc::strcmp(argv[i], e.suffix) == 0 ) {
          conf.standard = *e.full;
        }
      }
    } else if ( mc::strcmp(argv[i], "-O0") == 0 ) {
      conf.opt_mode = gcc::opt_flags::flags::optimize_zero;
      user_provided_opt = true;
    } else if ( mc::strcmp(argv[i], "-O1") == 0 ) {
      conf.opt_mode = gcc::opt_flags::flags::optimize_one;
      user_provided_opt = true;
    } else if ( mc::strcmp(argv[i], "-O2") == 0 ) {
      conf.opt_mode = gcc::opt_flags::flags::optimize_two;
      user_provided_opt = true;
    } else if ( mc::strcmp(argv[i], "-O3") == 0 ) {
      conf.opt_mode = gcc::opt_flags::flags::optimize_three;
      user_provided_opt = true;
    } else if ( mc::strcmp(argv[i], "-Ofast") == 0 ) {
      conf.opt_mode = gcc::opt_flags::flags::optimize_fast;
      // conf.no_ssp = true; // also disable the stack protector
      user_provided_opt = true;
    } else if ( mc::strcmp(argv[i], "-Oz") == 0 ) {
      conf.opt_mode = gcc::opt_flags::flags::optimize_z;
      user_provided_opt = true;
    } else if ( mc::strcmp(argv[i], "-gl") == 0 ) {
      // link with shared libs that micron::gfx::gl host_dso looks up
      // using literal names so it works cross platform
      conf.bonus_libs.push_back(string_type{ ":libX11.so.6" });
      conf.bonus_libs.push_back(string_type{ ":libGL.so.1" });
      conf.bonus_libs.push_back(string_type{ ":libwayland-client.so.0" });
      conf.bonus_libs.push_back(string_type{ ":libwayland-egl.so.1" });
      conf.bonus_libs.push_back(string_type{ ":libEGL.so.1" });
    } else if ( mc::strcmp(argv[i], "-vk") == 0 ) {
      conf.bonus_libs.push_back(string_type{ ":libX11.so.6" });
      conf.bonus_libs.push_back(string_type{ ":libwayland-client.so.0" });
      conf.bonus_libs.push_back(string_type{ ":libvulkan.so.1" });
    } else {
      // NOTE: no longer allow unrecog. flags (gcc rejects them regardless)
      if ( __is_flag(argv[i]) ) mc::cerror("unknown flag '", argv[i], "' - see 'duck help'");
      if ( __determine_source(conf, string_type{ argv[i] }) ) user_provided_out = true;
    }
  }
  // splat only prints the command; it must not build the output tree that command would write into
  if ( !splat::active() ) __mkdir_p(conf.bin_dir);
  finalize_and_infer(conf, user_provided_out, user_provided_type, user_provided_opt);
}

inline bool
__match_source_ext(const string_type &file)
{
  auto itr = mc::format::find_reverse(file, file.end() - 1, ".");
  if ( itr == nullptr ) return false;
  auto sstr = file.substr(itr);
  return sstr == ".cpp" or sstr == ".cc" or sstr == ".cxx" or sstr == ".c++" or sstr == ".c" or sstr == ".C" or sstr == ".i"
         or sstr == ".ii" or sstr == ".cp" or sstr == ".s" or sstr == ".S" or sstr == ".asm" or sstr == ".ASM";
}

inline void
__reject_output_collisions(const mc::vector<config_t> &confs)
{
  for ( usize i = 0; i < confs.size(); ++i )
    for ( usize j = i + 1; j < confs.size(); ++j )
      if ( confs[i].target_out == confs[j].target_out )
        mc::cerror("output collision: '", confs[i].target, "' and '", confs[j].target, "' both build to '", confs[i].target_out,
                   "' - build them into separate -o directories");
}

inline __attribute__((always_inline)) mc::vector<config_t>
parse_argv_build(int argc, char **argv)
{
  const int si = __find_source(argc, argv);
  if ( si < 0 ) mc::cerror("no source file or directory given");
  const bool recursive = __has_flag(argc, argv, "--recursive");

  mc::vector<string_type> sources{};
  if ( (int)mc::posix::get_type_at(argv[si]) == mc::posix::dir ) {
    if ( recursive ) {
      for ( auto &n : mc::io::ftw_files(mc::io::path(argv[si])) ) {
        string_type f{ n.c_str() };
        if ( __match_source_ext(f) ) sources.move_back(mc::move(f));
      }
    } else {
      string_type prefix = argv[si];
      if ( *(prefix.end() - 1) != '/' ) prefix.push_back('/');
      mc::io::dir d(argv[si]);
      auto &files = d.get_children();
      for ( auto &n : files ) {
        string_type f = prefix;
        f.append(string_type{ n.d_name.c_str() });
        if ( __match_source_ext(f) ) sources.move_back(mc::move(f));
      }
    }
  } else {
    if ( recursive ) mc::cerror("--recursive needs a directory, not a single source");
    sources.emplace_back(argv[si]);
  }

  mc::vector<config_t> confs;
  for ( auto &target : sources ) {
    config_t conf{};
    conf.target = target;      // already a full path in every branch
    conf.include_path.push_back("./src");
    conf.lib_path.push_back("./libs/");
    // placeholder, update correctly in parse_config
    conf.standard = gcc::__standard_cxx26;
    conf.compile_type = __comp_type::linked;
    conf.bin_dir = "bin/";

    // parse flags and finalize
    parse_config(conf, argc, argv, si);
    confs.move_back(mc::move(conf));
  }

  __reject_output_collisions(confs);
  return confs;
}

// single file mode
inline __attribute__((always_inline)) config_t
parse_argv_build_single(int argc, char **argv)
{
  const int si = __find_source(argc, argv);
  if ( si < 0 ) mc::cerror("no source file given");
  if ( __has_flag(argc, argv, "--recursive") ) mc::cerror("--recursive applies to a directory build; this command takes one source");

  config_t conf{};
  conf.target = argv[si];
  // placeholder, update correctly in parse_config
  conf.standard = gcc::__standard_cxx26;
  conf.compile_type = __comp_type::linked;
  conf.bin_dir = "bin/";
  conf.include_path.push_back("./src");
  conf.lib_path.push_back("./libs/");

  parse_config(conf, argc, argv, si);
  return conf;
}
};      // namespace gnu
};      // namespace recipes
