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

#include "flags.hh"

using string_type = mc::sstring<4096>;

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
batch_optimized(char *file_name, char *out_name, const string_type &compiler, const string_type &bin_dir,
                const string_type &standard)
{
  const string_type optimizations
      = make_flags(gcc::opt_flags::flags::optimize_fast, gcc::x86_flags::flags::mavx2, gcc::x86_flags::flags::mbmi,

                   gcc::x86_flags::flags::march_native);

  const string_type compile_libs = "-lc -lgcc -lgcc_s -lstdc++ -lm";

  const string_type flags_warn_base
      = make_flags(gcc::w_flags::flags::Wall, gcc::w_flags::flags::Wextra, gcc::w_flags::flags::pedantic);

  const string_type flags_warn_extra
      = "-Wno-cpp -Wunused -Wshadow -Wconversion -Wcast-qual -Wconversion-null -Woverlength-strings -Wpointer-arith "
        "-Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla -Wwrite-strings -Wduplicated-cond -Wdouble-promotion "
        "-Wdisabled-optimization -Winline -Wfloat-equal -Wmissing-noreturn -Wpacked -Wnonnull -Wundef -Wtrampolines "
        "-Winline -Winit-self -Wcast-align -Wnarrowing -Wregister -Wmain -Wchanges-meaning -Wsequence-point "
        "-Wattributes";

  const string_type flags_errors_extra = "-Werror=missing-field-initializers -Werror=return-type";

  const string_type flags_warn_ignore = "-Wno-implicit-fallthrough -Wno-sign-conversion -Wno-variadic-macros";

  const string_type flags_extensions
      = "-fstack-protector-strong -fstack-clash-protection -fstrict-overflow -fopenmp "
        "-fext-numeric-literals -ffast-math -flto -fdiagnostics-color=always -fconcepts-diagnostics-depth=2";

  const string_type libs_location = "-L./libs";
  const string_type includes_location = "-Isrc";
  const string_type libs_static = "-static-libstdc++ -static-libgcc";

  string_type command_pre_opt = make_command(compiler, standard, optimizations, flags_warn_base, flags_warn_extra,
                                             flags_warn_ignore, flags_errors_extra, flags_extensions);

  string_type command_post = make_command(compile_libs, includes_location);

  string_type command_release;
  if ( out_name != nullptr )
    command_release
        = make_command(command_pre_opt, file_name, command_post, "-o", bin_dir + "/" + out_name, libs_static);
  else {
    string_type tmp{ file_name };
    auto itr = mc::format::find_reverse(tmp, tmp.end() - 1, ".");
    auto itr_2 = mc::format::find_reverse(tmp, itr, "/") + 1;
    string_type out(itr_2, itr);
    command_release = make_command(command_pre_opt, file_name, command_post, "-o", bin_dir + "/" + string_type(out));
  }
  return command_release;
};

string_type
batch_debug(char *file_name, char *out_name, const string_type &compiler, const string_type &bin_dir,
            const string_type &standard)
{
  const string_type debug_flags = make_flags(gcc::debug_flags::flags::g, gcc::x86_flags::flags::march_native);

  const string_type compile_libs = "-lc -lgcc -lgcc_s -lstdc++ -lm";

  const string_type flags_warn_base
      = make_flags(gcc::w_flags::flags::Wall, gcc::w_flags::flags::Wextra, gcc::w_flags::flags::pedantic);

  const string_type flags_warn_extra
      = "-Wno-cpp -Wunused -Wshadow -Wconversion -Wcast-qual -Wconversion-null -Woverlength-strings -Wpointer-arith "
        "-Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla -Wwrite-strings -Wduplicated-cond -Wdouble-promotion "
        "-Wdisabled-optimization -Winline -Wfloat-equal -Wmissing-noreturn -Wpacked -Wnonnull -Wundef -Wtrampolines "
        "-Winline -Winit-self -Wcast-align -Wnarrowing -Wregister -Wmain -Wchanges-meaning -Wsequence-point "
        "-Wattributes";

  const string_type flags_errors_extra = "-Werror=missing-field-initializers -Werror=return-type";

  const string_type flags_warn_ignore = "-Wno-implicit-fallthrough -Wno-sign-conversion -Wno-variadic-macros";

  const string_type flags_extensions
      = "-fstack-protector-strong -fstack-clash-protection -fstrict-overflow -fopenmp "
        "-fext-numeric-literals -ffast-math -flto -fdiagnostics-color=always -fconcepts-diagnostics-depth=2";

  const string_type libs_location = "-L./libs";
  const string_type includes_location = "-Isrc";
  const string_type libs_static = "-static-libstdc++ -static-libgcc";

  string_type command_pre_debug = make_command(compiler, standard, debug_flags, flags_warn_base, flags_warn_extra,
                                               flags_warn_ignore, flags_errors_extra, flags_extensions);

  string_type command_post = make_command(compile_libs, includes_location);

  string_type command_debug;
  if ( out_name != nullptr )
    command_debug = make_command(command_pre_debug, file_name, command_post, "-o", bin_dir + "/" + out_name);
  else {
    string_type tmp{ file_name };
    auto itr = mc::format::find_reverse(tmp, tmp.end() - 1, ".");
    auto itr_2 = mc::format::find_reverse(tmp, itr, "/") + 1;
    string_type out(itr, itr_2);
    command_debug = make_command(command_pre_debug, file_name, command_post, "-o", bin_dir + "/" + string_type(out));
  }
  return command_debug;
};
