#pragma once

#include "control.hpp"
#include "linux/process/exec.hpp"
#include "linux/process/fork.hpp"
#include "linux/process/process.hpp"
#include "linux/process/signals.hpp"
#include "std.hpp"

#include "chrono.hpp"

#include "array/constarray.hpp"
#include "io/console.hpp"
#include "io/filesystem.hpp"
#include "string/format.hpp"
#include "string/sstring.hpp"

#include "linux/std.hpp"

#include "flags.hh"

#include "config.hh"

namespace recipes
{
namespace gnu
{
template<typename... Ts>
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
    if ( !r.empty() ) r.pop_back();
  return r;
}

template<typename... Fs>
string_type
make_flags(Fs &&...fs)
{
  string_type r;
  ((r += get_string_flag(fs), r += ' '), ...);
  if constexpr ( sizeof...(Fs) > 0 ) r.pop_back();
  return r;
}

template<typename F>
string_type
make_flags(const mc::constarray<F> flags)
{
  string_type r;
  for ( auto &n : flags ) {
    r += n;
    r += ' ';
  }
  if constexpr ( flags.size() > 1 ) r.pop_back();
  return r;
}

struct composed_t {
  string_type sanitize;
  string_type extensions;
  string_type link_tail;
};

inline void
__compose_add(string_type &dst, const char *s)
{
  if ( s == nullptr or *s == '\0' ) return;
  if ( !dst.empty() ) dst += ' ';
  dst += s;
}

inline composed_t
compose(const config_t &conf, bool linking)
{
  composed_t c;
  const bool cpp = __is_cpp_standard(conf.standard);
  const bool fs = conf.freestanding;
  const bool sanitized = conf.asan or conf.ubsan or conf.tsan or conf.lsan;

  if ( conf.asan and conf.tsan ) mc::cerror("--asan and --tsan are mutually exclusive");
  if ( fs and (conf.tsan or conf.lsan) ) mc::cerror("--tsan/--lsan need a hosted runtime; not valid under -k");
  if ( conf.asan ) __compose_add(c.sanitize, "-fsanitize=address -fno-omit-frame-pointer");
  if ( conf.ubsan ) __compose_add(c.sanitize, "-fsanitize=undefined");
  if ( conf.tsan ) __compose_add(c.sanitize, "-fsanitize=thread -fno-omit-frame-pointer");
  if ( conf.lsan and !conf.asan ) __compose_add(c.sanitize, "-fsanitize=leak");      // asan already covers leak

  c.extensions = fs ? (cpp ? make_flags(gcc::cpp_flags::flags::ext_numeric_literals) : string_type{})
                    : (cpp ? make_flags(conf.spall ? gcc::profiling_flags::flags::stack_protector_all
                                                   : gcc::profiling_flags::flags::stack_protector_strong,
                                        gcc::profiling_flags::flags::stack_clash_protection, gcc::profiling_flags::flags::strict_overflow,
                                        gcc::cpp_flags::flags::ext_numeric_literals)
                           : make_flags(conf.spall ? gcc::profiling_flags::flags::stack_protector_all
                                                   : gcc::profiling_flags::flags::stack_protector_strong,
                                        gcc::profiling_flags::flags::stack_clash_protection, gcc::profiling_flags::flags::strict_overflow));
  // LTO on by default except under sanitizers, freestanding EH, or a raw object
  if ( !sanitized and !conf.freestanding_eh and !conf.raw_object ) {
    if ( !c.extensions.empty() ) c.extensions += ' ';
    c.extensions += make_flags(gcc::opt_flags::flags::lto_eight);
  }
  if ( conf.raw_object ) __compose_add(c.extensions, "-fno-lto");      // belt-and-suspenders: force a non-LTO object

  // more safeties
  if ( conf.cfi ) {
    if ( conf.arch == __arch::x86 )
      __compose_add(c.extensions, "-fcf-protection=full");      // Intel CET
    else if ( conf.arch == __arch::arm64 )
      __compose_add(c.extensions, "-mbranch-protection=standard");      // PAC + BTI
    else
      __compose_add(c.extensions, "-mbranch-protection=pac-ret");      // arm32
  }
  // FORTIFY is hosted only
  if ( conf.fortify and !fs and !sanitized and conf.opt_mode != gcc::opt_flags::flags::optimize_zero )
    __compose_add(c.extensions, conf.arch == __arch::x86 ? "-D_FORTIFY_SOURCE=3" : "-D_FORTIFY_SOURCE=2");
  if ( (conf.pie and !fs) or conf.static_pie ) __compose_add(c.extensions, "-fPIE");      // static-PIE needs -fPIE even under -k
  if ( conf.gc ) __compose_add(c.extensions, "-ffunction-sections -fdata-sections");
  if ( conf.unroll ) __compose_add(c.extensions, "-funroll-loops");
  if ( conf.pgo_gen and conf.pgo_use ) mc::cerror("--pgo-gen and --pgo-use are mutually exclusive");
  if ( (conf.pgo_gen or conf.pgo_use) and fs ) mc::cerror("PGO needs a hosted runtime; not valid under -k");
  if ( conf.pgo_gen ) __compose_add(c.extensions, "-fprofile-generate");
  if ( conf.pgo_use ) __compose_add(c.extensions, "-fprofile-use -fprofile-correction -Wno-missing-profile");
  if ( conf.no_eh and cpp ) __compose_add(c.extensions, "-fno-exceptions");      // explicit only; hosted micron uses EH
  if ( conf.no_rtti and cpp ) __compose_add(c.extensions, "-fno-rtti");

  // links
  if ( linking ) {
    if ( (conf.pie or conf.static_pie) and conf.static_binary ) mc::cerror("--pie/--static-pie conflicts with -s (static)");
    if ( conf.static_pie ) {
      __compose_add(c.link_tail, "-static-pie");
      // under -k the image has no _start
      if ( fs ) __compose_add(c.link_tail, "-nostartfiles -Wl,-e,entry -Wl,--no-dynamic-linker");
    } else if ( conf.pie and !fs )
      __compose_add(c.link_tail, "-pie");
    if ( conf.relro ) __compose_add(c.link_tail, "-Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack");
    if ( conf.gc ) __compose_add(c.link_tail, "-Wl,--gc-sections");
    if ( conf.strip ) __compose_add(c.link_tail, "-Wl,--strip-all");
    if ( conf.tsan and conf.static_binary ) __compose_add(c.link_tail, "-static-libtsan");
  }

  return c;
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
  // -c/-S/-E never reach the linker: link inputs and link flags must stay out of those commands
  const bool linking = (conf.compile_type == __comp_type::linked);
  const string_type bin_type = (conf.static_binary and linking) ? make_flags(gcc::linker_flags::flags::static_link) : "";
  string_type freestanding;
  if ( conf.freestanding ) {
    freestanding = linking ? make_flags(gcc::c_flags::flags::freestanding, gcc::linker_flags::flags::nostdlib,
                                        gcc::linker_flags::flags::nostdlib_pp, gcc::profiling_flags::flags::nostack_protector)
                           : make_flags(gcc::c_flags::flags::freestanding, gcc::profiling_flags::flags::nostack_protector);
    if ( conf.freestanding_eh ) {
      // real C++ exceptions
      freestanding += " -fexceptions -frtti -fasynchronous-unwind-tables -D__micron_eh";
      if ( linking ) freestanding += " -Wl,--eh-frame-hdr";      // emit PT_GNU_EH_FRAME so find_fde works
    } else {
      freestanding += " ";
      freestanding += make_flags(gcc::profiling_flags::flags::no_exceptions, gcc::cpp_flags::flags::no_rtti);
    }
  }
  // -Wno-odr placed after -flto=8 so it actually disables Wodr
  const string_type freestanding_post
      = (conf.freestanding) ? make_flags(gcc::w_flags::flags::Wno_odr, gcc::w_flags::flags::Wno_lto_type_mismatch) : "";
  // the eh trampoline must live in its own TU (separate from throwing code)
  // width-aware _start: a -32 freestanding link must use the i386 crt, not the amd64 one
  string_type startup_objs;
  // a static-PIE freestanding image has no _start
  if ( conf.freestanding and linking and !conf.static_pie ) {
    startup_objs = (conf.width == 32) ? "/usr/src/mc_start/start_i386.s " : "/usr/src/mc_start/start.s ";
    startup_objs += "/usr/src/mc_start/start.cpp";
    if ( conf.freestanding_eh ) startup_objs += " /usr/src/mc_start/eh_runtime.cpp";
  }
  const string_type arch_width = (conf.width == 64) ? make_flags(gcc::x86_flags::flags::m64) : make_flags(gcc::x86_flags::flags::m32);
  string_type compile_libs = (conf.freestanding or !linking) ? "" : "-lpthread";
  if ( linking and !conf.bonus_libs.empty() )
    for ( auto &n : conf.bonus_libs ) {
      compile_libs += " -l";
      compile_libs += n;
    }
  string_type compile_objs = "";
  if ( !conf.bonus_objs.empty() ) {
    for ( auto &n : conf.bonus_objs ) {
      compile_objs += n;
      compile_objs += " ";
    }
    compile_objs.pop_back();
  }
  const string_type flags_warn_base = make_flags(gcc::w_flags::flags::Wall, gcc::w_flags::flags::Wextra, gcc::w_flags::flags::pedantic);

  // no more useless cast + floats
  const string_type flags_warn_extra = make_flags(
      gcc::w_flags::flags::Wunused, gcc::w_flags::flags::Wshadow, gcc::w_flags::flags::Wlogical_op, gcc::w_flags::flags::Wnull_dereference,
      gcc::w_flags::flags::Wconversion, gcc::w_flags::flags::Wcast_qual, gcc::w_flags::flags::Woverlength_strings,
      gcc::w_flags::flags::Wpointer_arith, gcc::w_flags::flags::Wunused, gcc::w_flags::flags::Wvarargs, gcc::w_flags::flags::Wvla,
      gcc::w_flags::flags::Wwrite_strings, gcc::w_flags::flags::Wduplicated_cond, gcc::w_flags::flags::Wduplicated_branches,
      gcc::w_flags::flags::Wdouble_promotion, gcc::w_flags::flags::Winline, gcc::w_flags::flags::Wcast_function_type,
      gcc::w_flags::flags::Wformat_security, gcc::w_flags::flags::Wmissing_noreturn, gcc::w_flags::flags::Wpacked,
      gcc::w_flags::flags::Wnonnull, gcc::w_flags::flags::Wundef, gcc::w_flags::flags::Wtrampolines, gcc::w_flags::flags::Warray_bounds,
      gcc::w_flags::flags::Wcast_align, gcc::w_flags::flags::Winit_self, gcc::w_flags::flags::Wnarrowing, gcc::w_flags::flags::Wregister,
      gcc::w_flags::flags::Wsequence_point);

  const string_type flags_errors_extra = "-Werror=missing-field-initializers -Werror=return-type";

  const string_type flags_warn_ignore = "-Wno-variadic-macros -Wno-inline";

  // sanitizer + hardening + perf + size + language fragment, shared across the three arch builders
  const composed_t __cz = compose(conf, linking);
  const string_type &flags_sanitize = __cz.sanitize;
  const string_type &flags_extensions = __cz.extensions;
  const string_type flags_extensions_supple
      = conf.doctor ? (__is_cpp_standard(conf.standard)
                           ? "-fdiagnostics-color=always -fconcepts-diagnostics-depth=2 -ftime-report -ftime-report-details -fmem-report "
                             "-fopt-info -fopt-info-missed"
                           : "-ftime-report -ftime-report-details -fmem-report -fopt-info -fopt-info-missed")
                    : (__is_cpp_standard(conf.standard) ? "-fdiagnostics-color=always -fconcepts-diagnostics-depth=2" : "");

  string_type libs_location;
  if ( linking )
    for ( const auto &p : conf.lib_path ) {
      libs_location += "-L";
      libs_location += p;
      libs_location += ' ';
    }
  if ( !libs_location.empty() ) libs_location.pop_back();
  string_type defines_flags;
  for ( const auto &p : conf.defines ) {
    defines_flags += "-D";
    defines_flags += p;
    defines_flags += ' ';
  }
  if ( !defines_flags.empty() ) defines_flags.pop_back();
  string_type includes_location;
  for ( const auto &p : conf.include_path ) {
    includes_location += "-I";
    includes_location += p;
    includes_location += ' ';
  }
  if ( !includes_location.empty() ) includes_location.pop_back();
  const string_type libs_static = (conf.static_binary and linking)
                                      ? make_flags(gcc::linker_flags::flags::static_libgcc, gcc::linker_flags::flags::static_libstdc_pp)
                                      : "";

  string_type command_pre = conf.warnings ? make_command(conf.compiler_path, conf.standard, comp_type, main_flags, flags_sanitize, bin_type,
                                                         freestanding, arch_width, flags_warn_base, flags_warn_extra, flags_warn_ignore,
                                                         flags_errors_extra, flags_extensions, freestanding_post, flags_extensions_supple)
                                          : make_command(conf.compiler_path, conf.standard, comp_type, main_flags, flags_sanitize, bin_type,
                                                         freestanding, arch_width, flags_warn_base, flags_warn_ignore, flags_extensions,
                                                         freestanding_post, flags_extensions_supple);

  string_type command_post = make_command(defines_flags, compile_libs, includes_location, libs_location);

  if ( conf.freestanding )
    return make_command(command_pre, conf.target, startup_objs, command_post, compile_objs, "-o", conf.target_out, libs_static,
                        __cz.link_tail);
  else
    return make_command(command_pre, conf.target, command_post, compile_objs, "-o", conf.target_out, libs_static, __cz.link_tail);
};

// armv7
string_type
batch_cmp_armv7(const config_t &conf)
{
  const string_type main_flags = (conf.mode == __opt_modes::optimized
                                      ? make_flags(conf.opt_mode, gcc::arm_flags::flags::march_armv7_a, gcc::arm_flags::flags::mfpu_neon,
                                                   gcc::arm_flags::flags::mfloat_abi_hard, gcc::opt_flags::flags::modulo_sched,
                                                   gcc::opt_flags::flags::modulo_sched_allow_regmoves, gcc::opt_flags::flags::gcse_sm,
                                                   gcc::opt_flags::flags::gcse_las)
                                      : make_flags(conf.opt_mode, gcc::debug_flags::flags::g_three, gcc::debug_flags::flags::ggdb_three,
                                                   gcc::debug_flags::flags::gcolumn_info, gcc::debug_flags::flags::ginline_points,
                                                   gcc::debug_flags::flags::gstatement_frontiers, gcc::arm_flags::flags::march_armv7_a,
                                                   gcc::arm_flags::flags::mfpu_neon, gcc::arm_flags::flags::mfloat_abi_hard));
  const string_type comp_type = (conf.compile_type == __comp_type::object)         ? "-c"
                                : (conf.compile_type == __comp_type::assembly)     ? "-S"
                                : (conf.compile_type == __comp_type::preprocessed) ? "-E"
                                                                                   : "";
  // -c/-S/-E never reach the linker: link inputs and link flags must stay out of those commands
  const bool linking = (conf.compile_type == __comp_type::linked);
  const string_type bin_type = (conf.static_binary and linking) ? make_flags(gcc::linker_flags::flags::static_link) : "";
  string_type freestanding;
  if ( conf.freestanding ) {
    freestanding = linking ? make_flags(gcc::c_flags::flags::freestanding, gcc::linker_flags::flags::nostdlib,
                                        gcc::linker_flags::flags::nostdlib_pp, gcc::profiling_flags::flags::nostack_protector)
                           : make_flags(gcc::c_flags::flags::freestanding, gcc::profiling_flags::flags::nostack_protector);
    if ( conf.freestanding_eh ) {
      // arm32 uses ARM EHABI
      freestanding += " -fexceptions -frtti -funwind-tables -D__micron_eh";
    } else {
      freestanding += " ";
      freestanding += make_flags(gcc::profiling_flags::flags::no_exceptions, gcc::cpp_flags::flags::no_rtti);
    }
  }
  // -Wno-odr placed after -flto=8 so it disable Wodr
  const string_type freestanding_post
      = (conf.freestanding) ? make_flags(gcc::w_flags::flags::Wno_odr, gcc::w_flags::flags::Wno_lto_type_mismatch) : "";
  const string_type startup_objs
      = (conf.freestanding and linking)
            ? (conf.freestanding_eh ? "/usr/src/mc_start/start_arm32.s /usr/src/mc_start/start.cpp /usr/src/mc_start/eh_runtime.cpp"
                                    : "/usr/src/mc_start/start_arm32.s /usr/src/mc_start/start.cpp")
            : "";
  string_type compile_libs = (conf.freestanding or !linking) ? "" : "-lpthread";
  if ( linking and !conf.bonus_libs.empty() )
    for ( auto &n : conf.bonus_libs ) {
      compile_libs += " -l";
      compile_libs += n;
    }
  string_type compile_objs = "";
  if ( !conf.bonus_objs.empty() ) {
    for ( auto &n : conf.bonus_objs ) {
      compile_objs += n;
      compile_objs += " ";
    }
    compile_objs.pop_back();
  }
  const string_type flags_warn_base = make_flags(gcc::w_flags::flags::Wall, gcc::w_flags::flags::Wextra, gcc::w_flags::flags::pedantic);

  // no more useless cast + floats
  const string_type flags_warn_extra = make_flags(
      gcc::w_flags::flags::Wunused, gcc::w_flags::flags::Wshadow, gcc::w_flags::flags::Wlogical_op, gcc::w_flags::flags::Wnull_dereference,
      gcc::w_flags::flags::Wconversion, gcc::w_flags::flags::Wcast_qual, gcc::w_flags::flags::Woverlength_strings,
      gcc::w_flags::flags::Wpointer_arith, gcc::w_flags::flags::Wunused, gcc::w_flags::flags::Wvarargs, gcc::w_flags::flags::Wvla,
      gcc::w_flags::flags::Wwrite_strings, gcc::w_flags::flags::Wduplicated_cond, gcc::w_flags::flags::Wduplicated_branches,
      gcc::w_flags::flags::Wdouble_promotion, gcc::w_flags::flags::Winline, gcc::w_flags::flags::Wcast_function_type,
      gcc::w_flags::flags::Wformat_security, gcc::w_flags::flags::Wmissing_noreturn, gcc::w_flags::flags::Wpacked,
      gcc::w_flags::flags::Wnonnull, gcc::w_flags::flags::Wundef, gcc::w_flags::flags::Wtrampolines, gcc::w_flags::flags::Warray_bounds,
      gcc::w_flags::flags::Wcast_align, gcc::w_flags::flags::Winit_self, gcc::w_flags::flags::Wnarrowing, gcc::w_flags::flags::Wregister,
      gcc::w_flags::flags::Wsequence_point);

  const string_type flags_errors_extra = "-Werror=missing-field-initializers -Werror=return-type";

  const string_type flags_warn_ignore = "-Wno-variadic-macros -Wno-inline";

  // sanitizer + hardening + perf + size + language
  const composed_t __cz = compose(conf, linking);
  const string_type &flags_sanitize = __cz.sanitize;
  const string_type &flags_extensions = __cz.extensions;

  const string_type flags_extensions_supple

      = conf.doctor ? (__is_cpp_standard(conf.standard)
                           ? "-fdiagnostics-color=always -fconcepts-diagnostics-depth=2 -ftime-report -ftime-report-details -fmem-report "
                             "-fopt-info -fopt-info-missed"
                           : "-ftime-report -ftime-report-details -fmem-report -fopt-info -fopt-info-missed")
                    : (__is_cpp_standard(conf.standard) ? "-fdiagnostics-color=always -fconcepts-diagnostics-depth=2" : "");

  string_type libs_location;
  if ( linking )
    for ( const auto &p : conf.lib_path ) {
      libs_location += "-L";
      libs_location += p;
      libs_location += ' ';
    }
  if ( !libs_location.empty() ) libs_location.pop_back();
  string_type defines_flags;
  for ( const auto &p : conf.defines ) {
    defines_flags += "-D";
    defines_flags += p;
    defines_flags += ' ';
  }
  if ( !defines_flags.empty() ) defines_flags.pop_back();
  string_type includes_location;
  for ( const auto &p : conf.include_path ) {
    includes_location += "-I";
    includes_location += p;
    includes_location += ' ';
  }
  if ( !includes_location.empty() ) includes_location.pop_back();
  const string_type libs_static = (conf.static_binary and linking)
                                      ? make_flags(gcc::linker_flags::flags::static_libgcc, gcc::linker_flags::flags::static_libstdc_pp)
                                      : "";

  string_type command_pre
      = conf.warnings ? make_command(conf.compiler_path, conf.standard, comp_type, main_flags, flags_sanitize, bin_type, freestanding,
                                     flags_warn_base, flags_warn_extra, flags_warn_ignore, flags_errors_extra, flags_extensions,
                                     freestanding_post, flags_extensions_supple)
                      : make_command(conf.compiler_path, conf.standard, comp_type, main_flags, flags_sanitize, bin_type, freestanding,
                                     flags_warn_base, flags_warn_ignore, flags_extensions, freestanding_post, flags_extensions_supple);

  string_type command_post = make_command(defines_flags, compile_libs, includes_location, libs_location);

  if ( conf.freestanding )
    return make_command(command_pre, conf.target, startup_objs, command_post, compile_objs, "-o", conf.target_out, libs_static,
                        __cz.link_tail);
  else
    return make_command(command_pre, conf.target, command_post, compile_objs, "-o", conf.target_out, libs_static, __cz.link_tail);
};

string_type
batch_cmp_aarch64(const config_t &conf)
{
  const string_type main_flags = (conf.mode == __opt_modes::optimized
                                      ? make_flags(conf.opt_mode, gcc::arm_flags::flags::march_armv8_a, gcc::opt_flags::flags::modulo_sched,
                                                   gcc::opt_flags::flags::modulo_sched_allow_regmoves, gcc::opt_flags::flags::gcse_sm,
                                                   gcc::opt_flags::flags::gcse_las)
                                      : make_flags(conf.opt_mode, gcc::debug_flags::flags::g_three, gcc::debug_flags::flags::ggdb_three,
                                                   gcc::debug_flags::flags::gcolumn_info, gcc::debug_flags::flags::ginline_points,
                                                   gcc::debug_flags::flags::gstatement_frontiers, gcc::arm_flags::flags::march_armv8_a));
  const string_type comp_type = (conf.compile_type == __comp_type::object)         ? "-c"
                                : (conf.compile_type == __comp_type::assembly)     ? "-S"
                                : (conf.compile_type == __comp_type::preprocessed) ? "-E"
                                                                                   : "";
  // -c/-S/-E never reach the linker: link inputs and link flags must stay out of those commands
  const bool linking = (conf.compile_type == __comp_type::linked);
  const string_type bin_type = (conf.static_binary and linking) ? make_flags(gcc::linker_flags::flags::static_link) : "";
  string_type freestanding;
  if ( conf.freestanding ) {
    freestanding = linking ? make_flags(gcc::c_flags::flags::freestanding, gcc::linker_flags::flags::nostdlib,
                                        gcc::linker_flags::flags::nostdlib_pp, gcc::profiling_flags::flags::nostack_protector)
                           : make_flags(gcc::c_flags::flags::freestanding, gcc::profiling_flags::flags::nostack_protector);
    // mic-thread futex/mutex use __atomic builtins; default -moutline-atomics emits __aarch64_cas*/__aarch64_ldadd*
    // libgcc helpers that don't resolve under -nostdlib. force them inline for EVERY freestanding aarch64 build.
    freestanding += " -mno-outline-atomics";
    if ( conf.freestanding_eh ) {
      // arm64 uses zero-cost DWARF
      freestanding += " -fexceptions -frtti -fasynchronous-unwind-tables -D__micron_eh";
      if ( linking ) freestanding += " -Wl,--eh-frame-hdr";
    } else {
      freestanding += " ";
      freestanding += make_flags(gcc::profiling_flags::flags::no_exceptions, gcc::cpp_flags::flags::no_rtti);
    }
  }
  const string_type freestanding_post
      = (conf.freestanding) ? make_flags(gcc::w_flags::flags::Wno_odr, gcc::w_flags::flags::Wno_lto_type_mismatch) : "";
  const string_type startup_objs
      = (conf.freestanding and linking)
            ? (conf.freestanding_eh ? "/usr/src/mc_start/start_arm64.s /usr/src/mc_start/start.cpp /usr/src/mc_start/eh_runtime.cpp"
                                    : "/usr/src/mc_start/start_arm64.s /usr/src/mc_start/start.cpp")
            : "";
  string_type compile_libs = (conf.freestanding or !linking) ? "" : "-lpthread";
  if ( linking and !conf.bonus_libs.empty() )
    for ( auto &n : conf.bonus_libs ) {
      compile_libs += " -l";
      compile_libs += n;
    }
  string_type compile_objs = "";
  if ( !conf.bonus_objs.empty() ) {
    for ( auto &n : conf.bonus_objs ) {
      compile_objs += n;
      compile_objs += " ";
    }
    compile_objs.pop_back();
  }
  const string_type flags_warn_base = make_flags(gcc::w_flags::flags::Wall, gcc::w_flags::flags::Wextra, gcc::w_flags::flags::pedantic);

  const string_type flags_warn_extra = make_flags(
      gcc::w_flags::flags::Wunused, gcc::w_flags::flags::Wshadow, gcc::w_flags::flags::Wlogical_op, gcc::w_flags::flags::Wnull_dereference,
      gcc::w_flags::flags::Wconversion, gcc::w_flags::flags::Wcast_qual, gcc::w_flags::flags::Woverlength_strings,
      gcc::w_flags::flags::Wpointer_arith, gcc::w_flags::flags::Wunused, gcc::w_flags::flags::Wvarargs, gcc::w_flags::flags::Wvla,
      gcc::w_flags::flags::Wwrite_strings, gcc::w_flags::flags::Wduplicated_cond, gcc::w_flags::flags::Wduplicated_branches,
      gcc::w_flags::flags::Wdouble_promotion, gcc::w_flags::flags::Winline, gcc::w_flags::flags::Wcast_function_type,
      gcc::w_flags::flags::Wformat_security, gcc::w_flags::flags::Wmissing_noreturn, gcc::w_flags::flags::Wpacked,
      gcc::w_flags::flags::Wnonnull, gcc::w_flags::flags::Wundef, gcc::w_flags::flags::Wtrampolines, gcc::w_flags::flags::Warray_bounds,
      gcc::w_flags::flags::Wcast_align, gcc::w_flags::flags::Winit_self, gcc::w_flags::flags::Wnarrowing, gcc::w_flags::flags::Wregister,
      gcc::w_flags::flags::Wsequence_point);

  const string_type flags_errors_extra = "-Werror=missing-field-initializers -Werror=return-type";

  const string_type flags_warn_ignore = "-Wno-variadic-macros -Wno-inline";

  // sanitizer + hardening + perf + size + language fragment, shared across the three arch builders
  const composed_t __cz = compose(conf, linking);
  const string_type &flags_sanitize = __cz.sanitize;
  const string_type &flags_extensions = __cz.extensions;

  const string_type flags_extensions_supple
      = conf.doctor ? (__is_cpp_standard(conf.standard)
                           ? "-fdiagnostics-color=always -fconcepts-diagnostics-depth=2 -ftime-report -ftime-report-details -fmem-report "
                             "-fopt-info -fopt-info-missed"
                           : "-ftime-report -ftime-report-details -fmem-report -fopt-info -fopt-info-missed")
                    : (__is_cpp_standard(conf.standard) ? "-fdiagnostics-color=always -fconcepts-diagnostics-depth=2" : "");

  string_type libs_location;
  if ( linking )
    for ( const auto &p : conf.lib_path ) {
      libs_location += "-L";
      libs_location += p;
      libs_location += ' ';
    }
  if ( !libs_location.empty() ) libs_location.pop_back();
  string_type defines_flags;
  for ( const auto &p : conf.defines ) {
    defines_flags += "-D";
    defines_flags += p;
    defines_flags += ' ';
  }
  if ( !defines_flags.empty() ) defines_flags.pop_back();
  string_type includes_location;
  for ( const auto &p : conf.include_path ) {
    includes_location += "-I";
    includes_location += p;
    includes_location += ' ';
  }
  if ( !includes_location.empty() ) includes_location.pop_back();
  const string_type libs_static = (conf.static_binary and linking)
                                      ? make_flags(gcc::linker_flags::flags::static_libgcc, gcc::linker_flags::flags::static_libstdc_pp)
                                      : "";

  string_type command_pre
      = conf.warnings ? make_command(conf.compiler_path, conf.standard, comp_type, main_flags, flags_sanitize, bin_type, freestanding,
                                     flags_warn_base, flags_warn_extra, flags_warn_ignore, flags_errors_extra, flags_extensions,
                                     freestanding_post, flags_extensions_supple)
                      : make_command(conf.compiler_path, conf.standard, comp_type, main_flags, flags_sanitize, bin_type, freestanding,
                                     flags_warn_base, flags_warn_ignore, flags_extensions, freestanding_post, flags_extensions_supple);

  string_type command_post = make_command(defines_flags, compile_libs, includes_location, libs_location);

  if ( conf.freestanding )
    return make_command(command_pre, conf.target, startup_objs, command_post, compile_objs, "-o", conf.target_out, libs_static,
                        __cz.link_tail);
  else
    return make_command(command_pre, conf.target, command_post, compile_objs, "-o", conf.target_out, libs_static, __cz.link_tail);
};

string_type
batch_asm(const config_t &conf)
{
  const string_type main_flags = (conf.width == 64) ? "-f elf64" : "-f elf32";
  string_type defines_flags;
  for ( const auto &p : conf.defines ) {
    defines_flags += "-D";
    defines_flags += p;
    defines_flags += ' ';
  }
  if ( !defines_flags.empty() ) defines_flags.pop_back();
  string_type includes_location;
  for ( const auto &p : conf.include_path ) {
    includes_location += "-I";
    includes_location += p;
    includes_location += ' ';
  }
  if ( !includes_location.empty() ) includes_location.pop_back();
  string_type command_pre = make_command(conf.compiler_path, main_flags);

  string_type command_post = make_command(defines_flags, includes_location);

  return make_command(command_pre, conf.target, command_post, "-o", conf.target_out);
};

string_type
batch_gas(const config_t &conf)
{
  string_type main_flags;
  if ( conf.arch == __arch::x86 )
    main_flags = (conf.width == 64) ? make_flags(gcc::x86_flags::flags::m64) : make_flags(gcc::x86_flags::flags::m32);
  else if ( conf.arch == __arch::arm )
    main_flags = make_flags(gcc::arm_flags::flags::march_armv7_a, gcc::arm_flags::flags::mfpu_neon, gcc::arm_flags::flags::mfloat_abi_hard);
  else if ( conf.arch == __arch::arm64 )
    main_flags = make_flags(gcc::arm_flags::flags::march_armv8_a);
  const string_type comp_type = (conf.compile_type == __comp_type::object)         ? "-c"
                                : (conf.compile_type == __comp_type::preprocessed) ? "-E"
                                                                                   : "";
  const bool linking = (conf.compile_type == __comp_type::linked);
  const string_type debug_flags = (conf.mode == __opt_modes::debug) ? make_flags(gcc::debug_flags::flags::g_three) : "";
  // freestanding asm links bare; hosted asm gets the libc startup files from the driver
  const string_type freestanding = (conf.freestanding and linking) ? make_flags(gcc::linker_flags::flags::nostdlib) : "";
  const string_type bin_type = (conf.static_binary and linking) ? make_flags(gcc::linker_flags::flags::static_link) : "";
  string_type defines_flags;
  for ( const auto &p : conf.defines ) {
    defines_flags += "-D";
    defines_flags += p;
    defines_flags += ' ';
  }
  if ( !defines_flags.empty() ) defines_flags.pop_back();
  string_type includes_location;
  for ( const auto &p : conf.include_path ) {
    includes_location += "-I";
    includes_location += p;
    includes_location += ' ';
  }
  if ( !includes_location.empty() ) includes_location.pop_back();
  string_type compile_objs = "";
  if ( !conf.bonus_objs.empty() ) {
    for ( auto &n : conf.bonus_objs ) {
      compile_objs += n;
      compile_objs += " ";
    }
    compile_objs.pop_back();
  }

  string_type command_pre = make_command(conf.compiler_path, comp_type, main_flags, debug_flags, bin_type, freestanding);
  string_type command_post = make_command(defines_flags, includes_location);

  return make_command(command_pre, conf.target, command_post, compile_objs, "-o", conf.target_out);
};

inline __attribute__((always_inline)) string_type
batch(const config_t &conf)
{
  if ( conf.language == __languages::lasm or conf.compiler == __compilers::nasm )
    return batch_asm(conf);
  else if ( conf.language == __languages::gas )
    return batch_gas(conf);
  else {
    if ( conf.arch == __arch::x86 )
      return batch_cmp(conf);
    else if ( conf.arch == __arch::arm )
      return batch_cmp_armv7(conf);
    else if ( conf.arch == __arch::arm64 )
      return batch_cmp_aarch64(conf);
  }
  __builtin_trap();
}
};      // namespace gnu
};      // namespace recipes
