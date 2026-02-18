#pragma once

#include "control.hpp"
#include "linux/process/exec.hpp"
#include "linux/process/fork.hpp"
#include "linux/process/process.hpp"
#include "std.hpp"
#include "thread/signal.hpp"

#include "chrono.hpp"

#include "array/constarray.hpp"
#include "io/console.hpp"
#include "io/filesystem.hpp"
#include "string/format.hpp"
#include "string/sstring.hpp"

#include "linux/std.hpp"

#include "impl.hh"

#include "flags.hh"

template <typename... Ts>
string_type
make_command(Ts &&...ts)
{
  string_type r;
  ((r += ts, r += ' '), ...);
  if constexpr ( sizeof...(Ts) > 0 )
    r.pop_back();
  return r;
}

template <typename... Fs>
string_type
make_flags(Fs &&...fs)
{
  string_type r;
  ((r += get_string_flag(fs), r += ' '), ...);
  if constexpr ( sizeof...(Fs) > 0 )
    r.pop_back();
  return r;
}

template <typename F>
string_type
make_flags(const mc::constarray<F> flags)
{
  string_type r;
  for ( auto &n : flags ) {
    r += n;
    r += ' ';
  }
  if constexpr ( flags.size() > 1 )
    r.pop_back();
  return r;
}

// these are char* since they come directly from argv
string_type
batch_optimized(const string_type &file_name, const string_type &out_name, const string_type &compiler, const string_type &standard,
                bool disable_warnings)
{
  const string_type optimizations = make_flags(gcc::opt_flags::flags::optimize_three, gcc::x86_flags::flags::mavx2, gcc::x86_flags::flags::mbmi,

                                               gcc::x86_flags::flags::march_native);

  const string_type compile_libs = "-lgcc_s -lm -lpthread";

  const string_type flags_warn_base = make_flags(gcc::w_flags::flags::Wall, gcc::w_flags::flags::Wextra, gcc::w_flags::flags::pedantic);

  string_type flags_warn_extra
      = "-Wno-cpp -Wunused -Wshadow -Wconversion -Wcast-qual -Wconversion-null -Woverlength-strings -Wpointer-arith "
        "-Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla -Wwrite-strings -Wduplicated-cond -Wdouble-promotion "
        "-Wdisabled-optimization -Winline -Wfloat-equal -Wmissing-noreturn -Wpacked -Wnonnull -Wundef -Wtrampolines "
        "-Winline -Winit-self -Wcast-align -Wnarrowing -Wregister -Wmain -Wchanges-meaning -Wsequence-point "
        "-Wattributes";

  const string_type flags_errors_extra = "-Werror=missing-field-initializers -Werror=return-type";

  const string_type flags_warn_ignore = "-Wno-implicit-fallthrough -Wno-sign-conversion -Wno-variadic-macros";

  const string_type flags_extensions = "-fstack-protector-strong -fstack-clash-protection -fstrict-overflow -fopenmp "
                                       "-fext-numeric-literals -ffast-math -flto -fdiagnostics-color=always -fconcepts-diagnostics-depth=2";

  const string_type libs_location = "-L./libs";
  const string_type includes_location = "-Isrc";
  const string_type libs_static = "-static-libstdc++ -static-libgcc";

  string_type command_pre_opt = disable_warnings
                                    ? make_command(compiler, standard, optimizations, flags_warn_base, flags_warn_ignore, flags_extensions)
                                    : make_command(compiler, standard, optimizations, flags_warn_base, flags_warn_extra, flags_warn_ignore,
                                                   flags_errors_extra, flags_extensions);

  string_type command_post = make_command(compile_libs, includes_location);

  return make_command(command_pre_opt, file_name, command_post, "-o", out_name, libs_static);
};

string_type
batch_debug(const string_type &file_name, const string_type &out_name, const string_type &compiler, const string_type &standard,
            bool disable_warnings)
{
  const string_type debug_flags = make_flags(gcc::debug_flags::flags::g, gcc::x86_flags::flags::march_native);

  const string_type compile_libs = "-lc -lgcc -lgcc_s -lstdc++ -lm";

  const string_type flags_warn_base = make_flags(gcc::w_flags::flags::Wall, gcc::w_flags::flags::Wextra, gcc::w_flags::flags::pedantic);

  const string_type flags_warn_extra
      = "-Wno-cpp -Wunused -Wshadow -Wconversion -Wcast-qual -Wconversion-null -Woverlength-strings -Wpointer-arith "
        "-Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla -Wwrite-strings -Wduplicated-cond -Wdouble-promotion "
        "-Wdisabled-optimization -Winline -Wfloat-equal -Wmissing-noreturn -Wpacked -Wnonnull -Wundef -Wtrampolines "
        "-Winline -Winit-self -Wcast-align -Wnarrowing -Wregister -Wmain -Wchanges-meaning -Wsequence-point "
        "-Wattributes";

  const string_type flags_errors_extra = "-Werror=missing-field-initializers -Werror=return-type";

  const string_type flags_warn_ignore = "-Wno-implicit-fallthrough -Wno-sign-conversion -Wno-variadic-macros";

  const string_type flags_extensions = "-fstack-protector-strong -fstack-clash-protection -fstrict-overflow -fopenmp "
                                       "-fext-numeric-literals -ffast-math -flto -fdiagnostics-color=always -fconcepts-diagnostics-depth=2";

  const string_type libs_location = "-L./libs";
  const string_type includes_location = "-Isrc";
  const string_type libs_static = "-static-libstdc++ -static-libgcc";

  string_type command_pre_debug = disable_warnings
                                      ? make_command(compiler, standard, debug_flags, flags_warn_base, flags_warn_ignore, flags_extensions)
                                      : make_command(compiler, standard, debug_flags, flags_warn_base, flags_warn_extra, flags_warn_ignore,
                                                     flags_errors_extra, flags_extensions);

  string_type command_post = make_command(compile_libs, includes_location);

  return make_command(command_pre_debug, file_name, command_post, "-o", out_name);
};

string_type
batch(const config_t &conf)
{

  const string_type main_flags
      = (conf.mode == __opt_modes::optimized
             ? make_flags(conf.opt_mode, gcc::x86_flags::flags::mavx2, gcc::x86_flags::flags::mbmi,

                          gcc::x86_flags::flags::march_native)
             : make_flags(conf.opt_mode, gcc::debug_flags::flags::g, gcc::x86_flags::flags::mavx2, gcc::x86_flags::flags::mbmi,

                          gcc::x86_flags::flags::march_native));

  const string_type compile_libs = "-lgcc_s -lm -lpthread";

  const string_type flags_warn_base = make_flags(gcc::w_flags::flags::Wall, gcc::w_flags::flags::Wextra, gcc::w_flags::flags::pedantic);

  string_type flags_warn_extra
      = "-Wno-cpp -Wunused -Wshadow -Wconversion -Wcast-qual -Wconversion-null -Woverlength-strings -Wpointer-arith "
        "-Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla -Wwrite-strings -Wduplicated-cond -Wdouble-promotion "
        "-Wdisabled-optimization -Winline -Wfloat-equal -Wmissing-noreturn -Wpacked -Wnonnull -Wundef -Wtrampolines "
        "-Winline -Winit-self -Wcast-align -Wnarrowing -Wregister -Wmain -Wchanges-meaning -Wsequence-point "
        "-Wattributes";

  const string_type flags_errors_extra = "-Werror=missing-field-initializers -Werror=return-type";

  const string_type flags_warn_ignore = "-Wno-implicit-fallthrough -Wno-sign-conversion -Wno-variadic-macros";

  const string_type flags_extensions = "-fstack-protector-strong -fstack-clash-protection -fstrict-overflow -fopenmp "
                                       "-fext-numeric-literals -ffast-math -flto -fdiagnostics-color=always -fconcepts-diagnostics-depth=2";

  const string_type libs_location = "-L./libs";
  const string_type includes_location = "-Isrc";
  const string_type libs_static = "-static-libstdc++ -static-libgcc";

  string_type command_pre
      = conf.less_warnings ? make_command(conf.compiler_path, conf.standard, main_flags, flags_warn_base, flags_warn_ignore, flags_extensions)
                           : make_command(conf.compiler_path, conf.standard, main_flags, flags_warn_base, flags_warn_extra, flags_warn_ignore,
                                          flags_errors_extra, flags_extensions);

  string_type command_post = make_command(compile_libs, includes_location);

  return make_command(command_pre, conf.target, command_post, "-o", conf.target_out, libs_static);
};
