#pragma once

#include "io/fsys.hpp"

#include "memory/addr.hpp"

#include "vector/vector.hpp"

#include "flags.hh"
#include "impl.hh"

enum __arch : u32 { x86 = 0, arm };

enum __compilers : u32 { gnucc = 0, clang, nasm };

enum __languages : u32 { cpp = 0, c, lasm };

enum __opt_modes : u32 { optimized = 0, debug };

enum __comp_type : u32 { linked = 0, object, assembly, preprocessed };

constexpr const string_type __compiler_gcc = "/usr/bin/gcc";
constexpr const string_type __compiler_clang = "/usr/bin/clang";
constexpr const string_type __compiler_gpp = "/usr/bin/g++";
constexpr const string_type __compiler_clangpp = "/usr/bin/clang++";
constexpr const string_type __assembler_nasm = "/usr/bin/nasm";

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
  string_type include_path;
  string_type lib_path;
  mc::vector<string_type> bonus_objs;
  mc::vector<string_type> bonus_libs;
  bool warnings;
  bool static_binary;
  u32 compile_type{ __comp_type::linked };
  u32 width{ 64 };
  u32 arch{ __arch::x86 };
  u32 compiler{ __compilers::gnucc };     // gcc by default
  u32 language{ __languages::cpp };       // cpp by default
  u32 mode{ __opt_modes::optimized };     // optimized by default
  gcc::opt_flags::flags opt_mode{ gcc::opt_flags::flag_optimize_fast };
};

// TODO: ugly refactor into a single func

mc::vector<config_t>
parse_argv_build(int argc, char **argv)
{
  // [file_in_name] []
  bool __dir_mode = true;
  mc::vector<mc::sstring<256>> sources{};
  if ( (int)mc::io::get_type_at(argv[0]) == mc::posix::dir ) {
    mc::io::dir d(argv[0]);
    auto &files = d.get_children();
    auto match_ext = [](const mc::sstring<256> &file) -> bool {
      auto itr = mc::format::find_reverse(file, file.end() - 1, ".");
      if ( itr == nullptr )
        itr = file.end();
      else {
        auto sstr = file.substr(itr);
        if ( sstr == ".cpp" or sstr == ".cc" or sstr == ".cxx" or sstr == ".c++" or sstr == ".c" or sstr == ".C" or sstr == ".i"
             or sstr == ".ii" or sstr == ".cp" ) {
          return true;
        }
      }
      return false;
    };

    for ( auto &n : files )
      if ( match_ext(n.a.d_name) )
        sources.emplace_back(n.a.d_name);
  } else {
    sources.emplace_back(argv[0]);
    __dir_mode = false;
  }
  mc::vector<config_t> confs;
  for ( auto &target : sources ) {
    config_t conf{};
    conf.target = target;
    conf.include_path = "./src";
    conf.lib_path = "./libs/";
    if ( __dir_mode ) {
      string_type sstr = argv[0];
      if ( *(sstr.end() - 1) != '/' )
        sstr.push_back('/');
      conf.target.insert(conf.target.begin(), sstr);
    }
    conf.standard = gcc::__standard_cxx23;
    conf.compile_type = __comp_type::linked;
    conf.bin_dir = "bin/";

    auto __determine_source = [&](string_type &&str) {
      auto itr = mc::format::find_reverse(str, str.end() - 1, ".");
      if ( itr == nullptr ) {
        conf.target_out = str;
        return true;     // should be target_out
      } else {
        auto sstr = str.substr(itr);
        if ( sstr == ".bin" ) {
          conf.target_out = str;
          return true;     // should be target_out
        } else if ( sstr == ".o" ) {
          // we added an obj
          conf.bonus_objs.emplace_back(str);
          return false;
        }
      }
      return false;
    };
    auto __make_name = [&]() {
      if ( conf.target.empty() )
        return;
      auto itr = mc::format::find_reverse(conf.target, conf.target.end() - 1, ".");
      if ( itr == nullptr )
        itr = conf.target.end();
      else {
        auto sstr = conf.target.substr(itr);
        if ( sstr == ".cpp" or sstr == ".cp" or sstr == ".cc" or sstr == ".cxx" or sstr == ".c++" or sstr == ".ii" or sstr == "C" )
          conf.language = __languages::cpp;
        else if ( sstr == ".c" or sstr == ".i" ) {
          conf.language = __languages::c;
        } else if ( sstr == ".asm" or sstr == ".ASM" or sstr == ".s" or sstr == ".S" ) {
          // set both
          conf.language = __languages::lasm;
          conf.compiler = __compilers::nasm;
        }
      }
      auto itr_2 = mc::format::find_reverse(conf.target, itr, "/");
      if ( itr_2 == nullptr )
        itr_2 = conf.target.begin();
      else
        itr_2++;
      // conf.target_out = pconf.bin_dir + string_type{ itr_2, itr };
      conf.target_out = string_type{ itr_2, itr };
    };

    __make_name();
    bool user_provided_out = false;
    bool user_provided_type = false;
    bool user_provided_opt = false;
    for ( int i = 1; i < argc; ++i ) {
      if ( mc::strcmp(argv[i], "-d") == 0 or mc::strcmp(argv[i], "-g") == 0 ) {
        conf.mode = __opt_modes::debug;
      } else if ( mc::strcmp(argv[i], "-o") == 0 ) {
        if ( ++i >= argc )
          mc::cerror("the -o flag must be followed by a path");
        conf.bin_dir = argv[i];
        if ( conf.bin_dir.empty() )
          mc::cerror("bin_dir is empty");
        if ( *(conf.bin_dir.end() - 1) != '/' )
          conf.bin_dir.insert(conf.bin_dir.end(), '/');
        __make_name();
      } else if ( mc::strcmp(argv[i], "-32") == 0 ) {
        conf.width = 32;
      } else if ( mc::strcmp(argv[i], "-64") == 0 ) {
        conf.width = 64;
      } else if ( mc::strcmp(argv[i], "--x86") == 0 ) {
        conf.arch = __arch::x86;
      } else if ( mc::strcmp(argv[i], "--arm") == 0 ) {
        conf.arch = __arch::arm;
      } else if ( mc::strcmp(argv[i], "-i") == 0 ) {
        if ( ++i >= argc )
          mc::cerror("the -i flag must be followed by a path");
        conf.include_path = argv[i];
      } else if ( mc::strcmp(argv[i], "-l") == 0 ) {
        if ( ++i >= argc )
          mc::cerror("the -i flag must be followed by a path");
        conf.lib_path = argv[i];
      } else if ( mc::strcmp(argv[i], "--lib") == 0 ) {
        if ( ++i >= argc )
          mc::cerror("the --lib flag must be followed by a library name (-l)name");
        conf.bonus_libs.push_back(argv[i]);
      } else if ( mc::strcmp(argv[i], "-w") == 0 ) {
        conf.warnings = true;
      } else if ( mc::strcmp(argv[i], "-s") == 0 ) {
        conf.static_binary = true;
      } else if ( mc::strcmp(argv[i], "--obj") == 0 ) {
        conf.compile_type = __comp_type::object;
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
        if ( ++i >= argc )
          mc::cerror("the --std flag must be followed by a valid standard type");
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
        user_provided_opt = true;
      } else if ( mc::strcmp(argv[i], "-Oz") == 0 ) {
        conf.opt_mode = gcc::opt_flags::flags::optimize_z;
        user_provided_opt = true;
      } else if ( sources.size() == 1 ) {
        if ( __determine_source(string_type{ argv[i] }) )
          user_provided_out = true;
      }
    }

    if ( conf.mode == __opt_modes::debug and !user_provided_opt )
      conf.opt_mode = gcc::opt_flags::flags::optimize_zero;
    conf.target_out.insert(conf.target_out.begin(), conf.bin_dir);
    if ( !user_provided_out ) {
      if ( conf.bonus_objs.empty() ) {
        if ( conf.compile_type == __comp_type::object )
          conf.target_out.append(".o");
        else if ( conf.compile_type == __comp_type::assembly )
          conf.target_out.append(".s");
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
        else if ( sstr == ".bin" ) {
          conf.compile_type = __comp_type::linked;
        } else if ( sstr == ".a" ) {
          // TODO: implement static outs
        }

        else if ( sstr == ".so" ) {
          // TODO: implement shared outs
        }
      }
    }

    switch ( conf.compiler ) {
    case __compilers::nasm : {
      conf.compiler_path = __assembler_nasm;
      break;
    }
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
    confs.move_back(mc::move(conf));
  }

  return confs;
}

config_t
parse_argv_build_single(int argc, char **argv)
{
  // [file_in_name] []
  config_t conf{};
  conf.target = argv[0];
  conf.standard = gcc::__standard_cxx23;
  conf.compile_type = __comp_type::linked;
  conf.bin_dir = "bin/";
  conf.include_path = "./src";
  conf.lib_path = "./libs/";

  auto __determine_source = [&](string_type &&str) {
    auto itr = mc::format::find_reverse(str, str.end() - 1, ".");
    if ( itr == nullptr ) {
      conf.target_out = str;
      return true;     // should be target_out
    } else {
      auto sstr = str.substr(itr);
      if ( sstr == ".bin" ) {
        conf.target_out = str;
        return true;     // should be target_out
      } else if ( sstr == ".o" ) {
        // we added an obj
        conf.bonus_objs.emplace_back(str);
        return false;
      }
    }
    return false;
  };

  auto __make_name = [&]() {
    if ( conf.target.empty() )
      return;
    auto itr = mc::format::find_reverse(conf.target, conf.target.end() - 1, ".");
    if ( itr == nullptr )
      itr = conf.target.end();
    else {
      auto sstr = conf.target.substr(itr);
      if ( sstr == ".cpp" or sstr == ".cc" or sstr == ".cxx" or sstr == ".c++" )
        conf.language = __languages::cpp;
      else if ( sstr == ".c" or sstr == ".C" ) {
        conf.language = __languages::c;
      }
    }
    auto itr_2 = mc::format::find_reverse(conf.target, itr, "/");
    if ( itr_2 == nullptr )
      itr_2 = conf.target.begin();
    else
      itr_2++;
    conf.target_out = string_type{ itr_2, itr };
  };

  __make_name();

  bool user_provided_out = false;
  bool user_provided_type = false;
  bool user_provided_opt = false;
  for ( int i = 1; i < argc; ++i ) {
    if ( mc::strcmp(argv[i], "-d") == 0 or mc::strcmp(argv[i], "-g") == 0 ) {
      conf.mode = __opt_modes::debug;
    } else if ( mc::strcmp(argv[i], "-o") == 0 ) {
      if ( ++i >= argc )
        mc::cerror("the -o flag must be followed by a path");
      conf.bin_dir = argv[i];
      if ( conf.bin_dir.empty() )
        mc::cerror("bin_dir is empty");
      if ( *(conf.bin_dir.end() - 1) != '/' )
        conf.bin_dir.insert(conf.bin_dir.end(), '/');
      __make_name();
    } else if ( mc::strcmp(argv[i], "-32") == 0 ) {
      conf.width = 32;
    } else if ( mc::strcmp(argv[i], "-64") == 0 ) {
      conf.width = 64;
    } else if ( mc::strcmp(argv[i], "--x86") == 0 ) {
      conf.arch = __arch::x86;
    } else if ( mc::strcmp(argv[i], "--arm") == 0 ) {
      conf.arch = __arch::arm;
    } else if ( mc::strcmp(argv[i], "-i") == 0 ) {
      if ( ++i >= argc )
        mc::cerror("the -i flag must be followed by a path");
      conf.include_path = argv[i];
    } else if ( mc::strcmp(argv[i], "-l") == 0 ) {
      if ( ++i >= argc )
        mc::cerror("the -l flag must be followed by a path");
      conf.lib_path = argv[i];
    } else if ( mc::strcmp(argv[i], "--lib") == 0 ) {
      if ( ++i >= argc )
        mc::cerror("the --lib flag must be followed by a library name (-l)name");
      conf.bonus_libs.push_back(argv[i]);
    } else if ( mc::strcmp(argv[i], "-w") == 0 ) {
      conf.warnings = true;
    } else if ( mc::strcmp(argv[i], "-s") == 0 ) {
      conf.static_binary = true;
    } else if ( mc::strcmp(argv[i], "--obj") == 0 ) {
      conf.compile_type = __comp_type::object;
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
      user_provided_opt = true;
    } else if ( mc::strcmp(argv[i], "-Oz") == 0 ) {
      conf.opt_mode = gcc::opt_flags::flags::optimize_z;
      user_provided_opt = true;
    } else {
      if ( __determine_source(string_type{ argv[i] }) )
        user_provided_out = true;
    }
  }

  if ( conf.mode == __opt_modes::debug and !user_provided_opt )
    conf.opt_mode = gcc::opt_flags::flags::optimize_zero;
  if ( !user_provided_out ) {
    if ( conf.bonus_objs.empty() ) {
      if ( conf.compile_type == __comp_type::object )
        conf.target_out.append(".o");
      else if ( conf.compile_type == __comp_type::assembly )
        conf.target_out.append(".s");
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
      else if ( sstr == ".bin" ) {
        conf.compile_type = __comp_type::linked;
      } else if ( sstr == ".a" ) {
        // TODO: implement static outs
      }

      else if ( sstr == ".so" ) {
        // TODO: implement shared outs
      }
    }
  }

  // if ( conf.target_out.empty() )
  //   __make_name();

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
