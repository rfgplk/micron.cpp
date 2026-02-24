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
  (([&] {
     if constexpr ( micron::is_class_v<micron::decay_t<Ts>> ) {
       if ( !ts.empty() ) {
         r += ts;
         r += ' ';
       }
     } else {
       if ( micron::strlen(ts) ) {

         r += ts;
         r += ' ';
       }
     }
   }()),
   ...);
  if constexpr ( sizeof...(Ts) > 0 )
    if ( !r.empty() )
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

// x86
string_type
batch_cmp(const config_t &conf)
{
  const string_type main_flags = (conf.mode == __opt_modes::optimized
                                      ? make_flags(conf.opt_mode, gcc::x86_flags::flags::mavx2, gcc::x86_flags::flags::mbmi,

                                                   gcc::x86_flags::flags::march_native, gcc::opt_flags::flags::modulo_sched,
                                                   gcc::opt_flags::flags::modulo_sched_allow_regmoves, gcc::opt_flags::flags::gcse_sm,
                                                   gcc::opt_flags::flags::gcse_las)
                                      : make_flags(conf.opt_mode, gcc::debug_flags::flags::g_three, gcc::debug_flags::flags::ggdb_three,
                                                   gcc::debug_flags::flags::gcolumn_info, gcc::debug_flags::flags::ginline_points,
                                                   gcc::debug_flags::flags::gstatement_frontiers, gcc::x86_flags::flags::march_native));
  const string_type comp_type = (conf.compile_type == __comp_type::object)         ? "-c"
                                : (conf.compile_type == __comp_type::assembly)     ? "-S"
                                : (conf.compile_type == __comp_type::preprocessed) ? "-E"
                                                                                   : "";
  const string_type bin_type = (conf.static_binary) ? make_flags(gcc::linker_flags::flags::static_link) : "";
  const string_type arch_width = (conf.width == 64) ? make_flags(gcc::x86_flags::flags::m64) : make_flags(gcc::x86_flags::flags::m32);
  string_type compile_libs = "-lm -lpthread";
  if ( !conf.bonus_libs.empty() )
    for ( auto &n : conf.bonus_libs ) {
      compile_libs += " -l";
      compile_libs += n;
    }
  const string_type flags_warn_base = make_flags(gcc::w_flags::flags::Wall, gcc::w_flags::flags::Wextra, gcc::w_flags::flags::pedantic);

  const string_type flags_warn_extra = make_flags(
      gcc::w_flags::flags::Wunused, gcc::w_flags::flags::Wshadow, gcc::w_flags::flags::Wuseless_cast, gcc::w_flags::flags::Wlogical_op,
      gcc::w_flags::flags::Wnull_dereference, gcc::w_flags::flags::Wconversion, gcc::w_flags::flags::Wcast_qual,
      gcc::w_flags::flags::Woverlength_strings, gcc::w_flags::flags::Wpointer_arith, gcc::w_flags::flags::Wunused,
      gcc::w_flags::flags::Wvarargs, gcc::w_flags::flags::Wvla, gcc::w_flags::flags::Wwrite_strings, gcc::w_flags::flags::Wduplicated_cond,
      gcc::w_flags::flags::Wduplicated_branches, gcc::w_flags::flags::Wdouble_promotion, gcc::w_flags::flags::Winline,
      gcc::w_flags::flags::Wcast_function_type, gcc::w_flags::flags::Wformat_security, gcc::w_flags::flags::Wfloat_equal,
      gcc::w_flags::flags::Wfloat_conversion, gcc::w_flags::flags::Wmissing_noreturn, gcc::w_flags::flags::Wpacked,
      gcc::w_flags::flags::Wnonnull, gcc::w_flags::flags::Wundef, gcc::w_flags::flags::Wtrampolines, gcc::w_flags::flags::Warray_bounds,
      gcc::w_flags::flags::Wcast_align, gcc::w_flags::flags::Winit_self, gcc::w_flags::flags::Wnarrowing, gcc::w_flags::flags::Wregister,
      gcc::w_flags::flags::Wsequence_point);

  const string_type flags_errors_extra = "-Werror=missing-field-initializers -Werror=return-type";

  const string_type flags_warn_ignore = "-Wno-variadic-macros -Wno-inline";

  const string_type flags_extensions
      = make_flags(gcc::profiling_flags::flags::stack_protector_strong, gcc::profiling_flags::flags::stack_clash_protection,
                   gcc::profiling_flags::flags::strict_overflow, gcc::cpp_flags::flags::ext_numeric_literals, gcc::opt_flags::flags::lto);
  const string_type flags_extensions_supple = "-fdiagnostics-color=always -fconcepts-diagnostics-depth=2";

  const string_type libs_location = "-L" + conf.lib_path;
  const string_type includes_location = "-I" + conf.include_path;
  const string_type libs_static
      = conf.static_binary ? make_flags(gcc::linker_flags::flags::static_libgcc, gcc::linker_flags::flags::static_libstdc_pp) : "";

  string_type command_pre
      = conf.warnings ? make_command(conf.compiler_path, conf.standard, comp_type, main_flags, bin_type, arch_width, flags_warn_base,
                                     flags_warn_extra, flags_warn_ignore, flags_errors_extra, flags_extensions, flags_warn_ignore,
                                     flags_extensions_supple)
                      : make_command(conf.compiler_path, conf.standard, comp_type, main_flags, bin_type, arch_width, flags_warn_base,
                                     flags_warn_ignore, flags_extensions, flags_warn_ignore, flags_extensions_supple);

  string_type command_post = make_command(compile_libs, includes_location, libs_location);

  return make_command(command_pre, conf.target, command_post, "-o", conf.target_out, libs_static);
};

string_type
batch_asm(const config_t &conf)
{
  const string_type main_flags = "-f elf64";
  const string_type includes_location = "-I" + conf.include_path;
  string_type command_pre
      = make_command(conf.compiler_path, main_flags);

  string_type command_post = make_command(includes_location);

  return make_command(command_pre, conf.target, command_post, "-o", conf.target_out);
};

string_type
batch(const config_t &conf){
  if(conf.language == __languages::lasm or conf.compiler == __compilers::nasm)
    return batch_asm(conf);
  else if(conf.language != __languages::lasm or conf.compiler != __compilers::nasm)
    return batch_cmp(conf);
}
