#pragma once

#include "memory/addr.hpp"

#include "flags.hh"
#include "impl.hh"

enum __compilers : u32 { gnucc = 0, clang };

enum __languages : u32 { cpp = 0, c };

enum __opt_modes : u32 { optimized = 0, debug };

constexpr const string_type __compiler_gcc = "/usr/bin/gcc";
constexpr const string_type __compiler_clang = "/usr/bin/clang";
constexpr const string_type __compiler_gpp = "/usr/bin/g++";
constexpr const string_type __compiler_clangpp = "/usr/bin/clang++";

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
  string_type standard;
  bool less_warnings;
  u32 compiler{ __compilers::gnucc };     // gcc by default
  u32 language{ __languages::cpp };       // cpp by default
  u32 mode{ 0 };                          // optimized by default
  gcc::opt_flags::flags opt_mode{ gcc::opt_flags::flag_optimize_fast };
};

constexpr const string_type __bin_dir = "bin/";

config_t
parse_argv_build(int argc, char **argv)
{
  // [file_in_name] []
  config_t conf{};
  conf.target = argv[0];
  conf.standard = gcc::__standard_cxx23;

  auto __make_name = [&]() {
    auto itr = mc::format::find_reverse(conf.target, conf.target.end() - 1, ".");
    auto itr_2 = mc::format::find_reverse(conf.target, itr, "/") + 1;
    conf.target_out = __bin_dir + string_type{ itr_2, itr };
  };

  if ( argc < 2 ) {
    __make_name();
  } else {
    for ( int i = 1; i < argc; ++i ) {
      if ( mc::strcmp(argv[i], "-d") == 0 or mc::strcmp(argv[i], "-g") == 0 ) {
        conf.mode = __opt_modes::debug;
      } else if ( mc::strcmp(argv[i], "-nw") == 0 ) {
        conf.less_warnings = true;
      } else if ( mc::strcmp(argv[i], "--gcc") == 0 ) {
        conf.compiler = __compilers::gnucc;
      } else if ( mc::strcmp(argv[i], "--clang") == 0 ) {
        conf.compiler = __compilers::clang;
      } else if ( mc::strcmp(argv[i], "--std") == 0 ) {
        if ( ++i >= argc )
          mc::cerror("the --std flag must be followed by a valid standard type");
        for ( const auto &e : std_table ) {
          if ( mc::strcmp(argv[i], e.suffix) == 0 ) {
            conf.standard = *e.full;
          }
        }
      } else if ( mc::strcmp(argv[i], "-O0") == 0 ) {
        conf.opt_mode = gcc::opt_flags::flags::optimize_zero;
      } else if ( mc::strcmp(argv[i], "-O1") == 0 ) {
        conf.opt_mode = gcc::opt_flags::flags::optimize_one;
      } else if ( mc::strcmp(argv[i], "-O2") == 0 ) {
        conf.opt_mode = gcc::opt_flags::flags::optimize_two;
      } else if ( mc::strcmp(argv[i], "-O3") == 0 ) {
        conf.opt_mode = gcc::opt_flags::flags::optimize_three;
      } else if ( mc::strcmp(argv[i], "-Ofast") == 0 ) {
        conf.opt_mode = gcc::opt_flags::flags::optimize_fast;
      } else if ( mc::strcmp(argv[i], "-Oz") == 0 ) {
        conf.opt_mode = gcc::opt_flags::flags::optimize_z;
      } else {
        conf.target_out = __bin_dir + string_type{ argv[i] };
      }
    }
  }
  if ( conf.mode == __opt_modes::debug )
    conf.opt_mode = gcc::opt_flags::flags::optimize_debug;

  if ( conf.target_out.empty() )
    __make_name();
  switch ( conf.compiler ) {
  case __compilers::gnucc : {
    if ( conf.language == __languages::c )
      conf.compiler_path = __compiler_gcc;
    else if ( conf.language == __languages::cpp )
      conf.compiler_path = __compiler_gpp;
    break;
  }
  case __compilers::clang : {
    if ( conf.language == __languages::c )
      conf.compiler_path = __compiler_clang;
    else if ( conf.language == __languages::cpp )
      conf.compiler_path = __compiler_clangpp;
    break;
  }
  };
  return conf;
}
