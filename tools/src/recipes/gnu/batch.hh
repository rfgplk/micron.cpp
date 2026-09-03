#pragma once

#include "control.hpp"
#include "linux/process/exec.hpp"
#include "linux/process/fork.hpp"
#include "linux/process/process.hpp"
#include "linux/process/signals.hpp"
#include "std.hpp"

#include "chrono.hpp"

#include "io/console.hpp"
#include "io/filesystem.hpp"
#include "string/format.hpp"
#include "string/sstring.hpp"

#include "linux/std.hpp"

#include "../clang/flags.hh"
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

template<typename... Fs>
inline void
__compose_flags(string_type &dst, Fs... fs)
{
  (__compose_add(dst, get_string_flag(fs)), ...);
}

inline bool
__is_clang(const config_t &conf)
{
  return conf.compiler == __compilers::clang;
}

inline string_type
__opt_flags(const config_t &conf)
{
  if ( !__is_clang(conf) ) return make_flags(conf.opt_mode);
  using O = gcc::opt_flags::flags;
  if ( conf.opt_mode == O::optimize_zero ) return clang_flags::optimize_zero;
  if ( conf.opt_mode == O::optimize_one ) return clang_flags::optimize_one;
  if ( conf.opt_mode == O::optimize_two ) return clang_flags::optimize_two;
  if ( conf.opt_mode == O::optimize_three ) return clang_flags::optimize_three;
  if ( conf.opt_mode == O::optimize_size ) return clang_flags::optimize_size;
  if ( conf.opt_mode == O::optimize_z ) return clang_flags::optimize_tiny;
  if ( conf.opt_mode == O::optimize_debug ) return clang_flags::optimize_debug;
  // Clang 22 deprecated -Ofast in favor of its exact expansion.
  string_type r = clang_flags::optimize_three;
  __compose_add(r, clang_flags::fast_math);
  return r;
}

inline string_type
__main_opt_flags(const config_t &conf)
{
  string_type r = __opt_flags(conf);
  if ( conf.mode == __opt_modes::optimized ) {
    if ( !__is_clang(conf) )
      __compose_flags(r, gcc::opt_flags::flags::modulo_sched, gcc::opt_flags::flags::modulo_sched_allow_regmoves,
                      gcc::opt_flags::flags::gcse_sm, gcc::opt_flags::flags::gcse_las);
  } else if ( __is_clang(conf) ) {
    __compose_add(r, clang_flags::debug_three);
    __compose_add(r, clang_flags::debug_gdb_three);
    __compose_add(r, clang_flags::debug_columns);
  } else
    __compose_flags(r, gcc::debug_flags::flags::g_three, gcc::debug_flags::flags::ggdb_three, gcc::debug_flags::flags::gcolumn_info,
                    gcc::debug_flags::flags::ginline_points, gcc::debug_flags::flags::gstatement_frontiers);
  return r;
}

inline string_type
__clang_cross_flags(const config_t &conf, bool linking)
{
  if ( !__is_clang(conf) or conf.arch == __arch::x86 ) return {};
  const string_type &target = conf.arch == __arch::arm ? __clang_target_arm : __clang_target_arm64;
  const string_type &toolchain = conf.arch == __arch::arm ? __clang_toolchain_arm : __clang_toolchain_arm64;
  const string_type &sysroot = conf.arch == __arch::arm ? __clang_sysroot_arm : __clang_sysroot_arm64;
  const string_type &linker = conf.arch == __arch::arm ? __clang_linker_arm : __clang_linker_arm64;
  string_type r = clang_flags::target;
  r += target;
  r += ' ';
  r += clang_flags::gcc_toolchain;
  r += toolchain;
  r += ' ';
  r += clang_flags::sysroot;
  r += sysroot;
  if ( linking ) {
    r += ' ';
    r += clang_flags::ld_path;
    r += linker;
  }
  return r;
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
  if ( conf.asan ) __compose_flags(c.sanitize, gcc::profiling_flags::flags::sanitize_address, gcc::opt_flags::flags::no_omit_frame_pointer);
  if ( conf.ubsan ) __compose_flags(c.sanitize, gcc::profiling_flags::flags::sanitize_undefined);
  if ( conf.tsan ) __compose_flags(c.sanitize, gcc::profiling_flags::flags::sanitize_thread, gcc::opt_flags::flags::no_omit_frame_pointer);
  if ( conf.lsan and !conf.asan ) __compose_flags(c.sanitize, gcc::profiling_flags::flags::sanitize_leak);      // asan already covers leak

  // stack protector: --no-ssp (none) > default (all) > --spall unset (strong)
  const auto __ssp = conf.no_ssp  ? gcc::profiling_flags::flags::nostack_protector
                     : conf.spall ? gcc::profiling_flags::flags::stack_protector_all
                                  : gcc::profiling_flags::flags::stack_protector_strong;
  if ( fs ) {
    if ( cpp and !__is_clang(conf) ) c.extensions = make_flags(gcc::cpp_flags::flags::ext_numeric_literals);
  } else if ( __is_clang(conf) )
    c.extensions = make_flags(__ssp, gcc::profiling_flags::flags::stack_clash_protection, gcc::profiling_flags::flags::strict_overflow);
  else
    c.extensions
        = cpp ? make_flags(__ssp, gcc::profiling_flags::flags::stack_clash_protection, gcc::profiling_flags::flags::strict_overflow,
                           gcc::cpp_flags::flags::ext_numeric_literals)
              : make_flags(__ssp, gcc::profiling_flags::flags::stack_clash_protection, gcc::profiling_flags::flags::strict_overflow);
  // LTO on by default except under sanitizers, freestanding EH, a raw object, or an explicit --no-lto
  if ( !sanitized and !conf.freestanding_eh and !conf.raw_object and !conf.no_lto )
    __compose_add(c.extensions, __is_clang(conf) ? clang_flags::thin_lto : get_string_flag(gcc::opt_flags::flags::lto_eight));
  if ( conf.raw_object or conf.no_lto ) __compose_flags(c.extensions, gcc::opt_flags::flags::no_lto);

  // more safeties
  if ( conf.cfi ) {
    if ( conf.arch == __arch::x86 )
      __compose_flags(c.extensions, gcc::profiling_flags::flags::cf_protection_full);      // Intel CET
    else if ( conf.arch == __arch::arm64 )
      __compose_flags(c.extensions, gcc::aarch64_flags::flags::mbranch_protection_standard);      // PAC + BTI
    else
      __compose_flags(c.extensions, gcc::arm_flags::flags::mbranch_protection_pac_ret);      // arm32
  }
  // FORTIFY is hosted only
  if ( conf.fortify and !fs and !sanitized and conf.opt_mode != gcc::opt_flags::flags::optimize_zero )
    __compose_flags(c.extensions, conf.arch == __arch::x86 ? gcc::preprocessor_flags::flags::fortify_source_three
                                                           : gcc::preprocessor_flags::flags::fortify_source_two);
  // -fPIE is codegen, and is legitimate under -k
  // --harden's implied pie still collapses under -k
  if ( (conf.pie and (!fs or conf.pie_explicit)) or conf.static_pie ) __compose_flags(c.extensions, gcc::opt_flags::flags::PIE);
  if ( conf.gc ) __compose_flags(c.extensions, gcc::opt_flags::flags::function_sections, gcc::opt_flags::flags::data_sections);
  if ( conf.unroll ) __compose_flags(c.extensions, gcc::opt_flags::flags::unroll_loops);
  // --fp. asan/tsan already inject this above; don't emit it twice
  if ( conf.frame_pointer and !conf.asan and !conf.tsan ) __compose_flags(c.extensions, gcc::opt_flags::flags::no_omit_frame_pointer);
  if ( conf.pgo_gen and conf.pgo_use ) mc::cerror("--pgo-gen and --pgo-use are mutually exclusive");
  if ( (conf.pgo_gen or conf.pgo_use) and fs ) mc::cerror("PGO needs a hosted runtime; not valid under -k");
  if ( conf.pgo_gen ) __compose_flags(c.extensions, gcc::profiling_flags::flags::profile_generate);
  if ( conf.pgo_use ) {
    __compose_flags(c.extensions, gcc::opt_flags::flags::profile_use);
    if ( !__is_clang(conf) )
      __compose_flags(c.extensions, gcc::opt_flags::flags::profile_correction, gcc::w_flags::flags::Wno_missing_profile);
  }
  // explicit only; hosted micron uses EH
  if ( conf.no_eh and cpp ) __compose_flags(c.extensions, gcc::profiling_flags::flags::no_exceptions);
  if ( conf.no_rtti and cpp ) __compose_flags(c.extensions, gcc::cpp_flags::flags::no_rtti);
  if ( conf.reflection and cpp ) __compose_flags(c.extensions, gcc::w_flags::flags::freflection);      // validated in finalize_and_infer
  if ( conf.opnames and cpp ) __compose_flags(c.extensions, gcc::cpp_flags::flags::no_operator_names);

  // links
  if ( linking ) {
    if ( (conf.pie or conf.static_pie) and conf.static_binary ) mc::cerror("--pie/--static-pie conflicts with -s (static)");
    if ( conf.static_pie ) {
      __compose_flags(c.link_tail, gcc::linker_flags::flags::static_pie);
      // under -k the image has no _start
      if ( fs )
        __compose_flags(c.link_tail, gcc::linker_flags::flags::nostartfiles, gcc::linker_flags::flags::wl_entry_entry,
                        gcc::linker_flags::flags::wl_no_dynamic_linker);
    } else if ( conf.pie and !fs )
      __compose_flags(c.link_tail, gcc::linker_flags::flags::pie);
    if ( conf.relro )
      __compose_flags(c.link_tail, gcc::linker_flags::flags::wl_z_relro, gcc::linker_flags::flags::wl_z_now,
                      gcc::linker_flags::flags::wl_z_noexecstack);
    if ( conf.gc ) __compose_flags(c.link_tail, gcc::linker_flags::flags::wl_gc_sections);
    if ( conf.strip ) __compose_flags(c.link_tail, gcc::linker_flags::flags::wl_strip_all);
    if ( conf.tsan and conf.static_binary and !__is_clang(conf) ) __compose_flags(c.link_tail, gcc::linker_flags::flags::static_libtsan);
  }

  return c;
}

inline const char *
__flags_output()
{
  return get_string_flag(gcc::linker_flags::flags::object_file_name);
}

inline const char *
__flags_comp_type(const config_t &conf)
{
  return (conf.compile_type == __comp_type::object)         ? get_string_flag(gcc::driver_flags::flags::compile_only)
         : (conf.compile_type == __comp_type::assembly)     ? get_string_flag(gcc::driver_flags::flags::assemble_only)
         : (conf.compile_type == __comp_type::preprocessed) ? get_string_flag(gcc::driver_flags::flags::preprocess_only)
                                                            : "";
}

inline string_type
__flags_bin_type(const config_t &conf, bool linking)
{
  return (conf.static_binary and linking) ? make_flags(gcc::linker_flags::flags::static_link) : "";
}

inline string_type
__flags_libs_static(const config_t &conf, bool linking)
{
  return (conf.static_binary and linking) ? make_flags(gcc::linker_flags::flags::static_libgcc, gcc::linker_flags::flags::static_libstdc_pp)
                                          : "";
}

inline string_type
__flags_warn_base()
{
  return make_flags(gcc::w_flags::flags::Wall, gcc::w_flags::flags::Wextra, gcc::w_flags::flags::pedantic);
}

inline string_type
__flags_errors_extra()
{
  return make_flags(gcc::w_flags::flags::Werror_missing_field_initializers, gcc::w_flags::flags::Werror_return_type);
}

inline string_type
__flags_warn_ignore()
{
  return make_flags(gcc::w_flags::flags::Wno_variadic_macros, gcc::w_flags::flags::Wno_inline);
}

// -k/-ke
inline string_type
__flags_freestanding(const config_t &conf, bool linking)
{
  string_type r;
  if ( !conf.freestanding ) return r;
  // Clang gives main ordinary C++ linkage under -ffreestanding. Keep its hosted
  // main ABI while reproducing freestanding semantics explicitly.
  if ( __is_clang(conf) ) {
    __compose_add(r, clang_flags::hosted_main);
    __compose_add(r, clang_flags::no_builtin);
    __compose_add(r, clang_flags::micron_freestanding);
    if ( linking ) __compose_flags(r, gcc::linker_flags::flags::nostdlib, gcc::linker_flags::flags::nostdlib_pp);
    __compose_flags(r, gcc::profiling_flags::flags::nostack_protector);
  } else
    r = linking ? make_flags(gcc::c_flags::flags::freestanding, gcc::linker_flags::flags::nostdlib, gcc::linker_flags::flags::nostdlib_pp,
                             gcc::profiling_flags::flags::nostack_protector)
                : make_flags(gcc::c_flags::flags::freestanding, gcc::profiling_flags::flags::nostack_protector);
  // mic-thread futex/mutex use __atomic builtins
  if ( conf.arch == __arch::arm64 ) __compose_flags(r, gcc::aarch64_flags::flags::mno_outline_atomics);
  if ( conf.freestanding_eh ) {
    __compose_flags(r, gcc::cpp_flags::flags::exceptions, gcc::cpp_flags::flags::rtti,
                    conf.arch == __arch::arm ? gcc::opt_flags::flags::unwind_tables : gcc::opt_flags::flags::asynchronous_unwind_tables);
    string_type eh_define = get_string_flag(gcc::preprocessor_flags::flags::d_macro);
    eh_define += "__micron_eh";
    __compose_add(r, eh_define.c_str());
    // emit PT_GNU_EH_FRAME so find_fde works. EHABI carries its own index section, no program header
    if ( linking and conf.arch != __arch::arm ) __compose_flags(r, gcc::linker_flags::flags::wl_eh_frame_hdr);
  } else
    __compose_flags(r, gcc::profiling_flags::flags::no_exceptions, gcc::cpp_flags::flags::no_rtti);
  if ( !conf.freestanding_eh and __is_clang(conf) ) {
    __compose_add(r, clang_flags::no_unwind_tables);
    __compose_add(r, clang_flags::no_async_unwind_tables);
  }
  return r;
}

// -k/-ke entry stub, one per arch+width
inline const char *
__start_stub_name(const config_t &conf)
{
  if ( conf.mx ) {
    if ( conf.arch == __arch::arm ) return "mx/start_arm32.s";
    if ( conf.arch == __arch::arm64 ) return "mx/start_arm64.s";
    return (conf.width == 32) ? "mx/start_i386.s" : "mx/start.s";
  }
  if ( conf.cont ) {
    if ( conf.arch == __arch::arm ) return "mx/cont_arm32.s";
    if ( conf.arch == __arch::arm64 ) return "mx/cont_arm64.s";
    return (conf.width == 32) ? "mx/cont_i386.s" : "mx/cont.s";
  }
  if ( conf.arch == __arch::arm ) return conf.direct ? "direct_arm32.s" : "start_arm32.s";
  if ( conf.arch == __arch::arm64 ) return conf.direct ? "direct_arm64.s" : "start_arm64.s";
  // width-aware _start: a -32 freestanding link must use the i386 crt, not the amd64 one
  return (conf.width == 32) ? (conf.direct ? "direct_i386.s" : "start_i386.s") : (conf.direct ? "direct.s" : "start.s");
}

inline void
__start_append(string_type &dst, const config_t &conf, const char *file)
{
  string_type p = conf.start_dir;      // already slash-terminated by finalize_and_infer
  p += file;
  if ( !mc::posix::exists(p.c_str()) )
    mc::cerror("freestanding link needs '", p, "' - install the micron crt with 'scripts/install_start.py <dir>', ",
               "then point duck at it with --start <dir> or MICRON_START");
  if ( !dst.empty() ) dst += ' ';
  dst += p;
}

// the crt inputs of a freestanding link, or empty
inline string_type
__startup_objs(const config_t &conf, bool linking)
{
  string_type r;
  if ( !conf.freestanding or !linking ) return r;
  if ( conf.arch == __arch::x86 and conf.static_pie ) return r;
  __start_append(r, conf, __start_stub_name(conf));
  if ( conf.cont ) return r;
  __start_append(r, conf, "start.cpp");
  if ( conf.freestanding_eh ) __start_append(r, conf, "eh_runtime.cpp");
  return r;
}

inline string_type
__flags_warn_extra(const config_t &conf)
{
  if ( __is_clang(conf) ) return clang_flags::warnings_extra;
  return make_flags(gcc::w_flags::flags::Wunused, gcc::w_flags::flags::Wshadow, gcc::w_flags::flags::Wlogical_op,
                    gcc::w_flags::flags::Wnull_dereference, gcc::w_flags::flags::Wconversion, gcc::w_flags::flags::Wcast_qual,
                    gcc::w_flags::flags::Woverlength_strings, gcc::w_flags::flags::Wpointer_arith, gcc::w_flags::flags::Wunused,
                    gcc::w_flags::flags::Wvarargs, gcc::w_flags::flags::Wvla, gcc::w_flags::flags::Wwrite_strings,
                    gcc::w_flags::flags::Wduplicated_cond, gcc::w_flags::flags::Wduplicated_branches,
                    gcc::w_flags::flags::Wdouble_promotion, gcc::w_flags::flags::Winline, gcc::w_flags::flags::Wcast_function_type,
                    gcc::w_flags::flags::Wformat_security, gcc::w_flags::flags::Wmissing_noreturn, gcc::w_flags::flags::Wpacked,
                    gcc::w_flags::flags::Wnonnull, gcc::w_flags::flags::Wundef, gcc::w_flags::flags::Wtrampolines,
                    gcc::w_flags::flags::Warray_bounds, gcc::w_flags::flags::Wcast_align, gcc::w_flags::flags::Winit_self,
                    gcc::w_flags::flags::Wnarrowing, gcc::w_flags::flags::Wregister, gcc::w_flags::flags::Wsequence_point);
}

inline string_type
__flags_freestanding_post(const config_t &conf)
{
  if ( !conf.freestanding ) return {};
  return __is_clang(conf) ? make_flags(gcc::w_flags::flags::Wno_odr)
                          : make_flags(gcc::w_flags::flags::Wno_odr, gcc::w_flags::flags::Wno_lto_type_mismatch);
}

// -fdiagnostics-* for C++ always
inline string_type
__flags_extensions_supple(const config_t &conf)
{
  string_type r;
  if ( __is_cpp_standard(conf.standard) ) {
    __compose_flags(r, gcc::diagnostic_flags::flags::diagnostics_color_always);
    if ( !__is_clang(conf) ) __compose_flags(r, gcc::w_flags::flags::fconcepts_diagnostics_depth_two);
  }
  if ( conf.doctor ) {
    __compose_flags(r, gcc::diagnostic_flags::flags::time_report);
    if ( __is_clang(conf) ) {
      __compose_add(r, clang_flags::remarks_passed);
      __compose_add(r, clang_flags::remarks_missed);
      __compose_add(r, clang_flags::remarks_analysis);
    } else
      __compose_flags(r, gcc::diagnostic_flags::flags::time_report_details, gcc::diagnostic_flags::flags::mem_report,
                      gcc::diagnostic_flags::flags::opt_info, gcc::diagnostic_flags::flags::opt_info_missed);
  }
  return r;
}

inline string_type
__flags_link_libs(const config_t &conf, bool linking)
{
  string_type compile_libs = (conf.freestanding or !linking) ? "" : make_flags(gcc::linker_flags::flags::l_pthread);
  // AArch64 long double is IEEE binary128; Clang lowers its support operations
  // to the GCC compiler runtime supplied by the selected cross toolchain.
  if ( linking and conf.freestanding and __is_clang(conf) and conf.arch == __arch::arm64 )
    __compose_add(compile_libs, clang_flags::gcc_runtime);
  if ( linking and !conf.bonus_libs.empty() )
    for ( auto &n : conf.bonus_libs ) {
      if ( !compile_libs.empty() ) compile_libs += ' ';
      compile_libs += get_string_flag(gcc::linker_flags::flags::l_library);
      compile_libs += n;
    }
  return compile_libs;
}

inline string_type
__flags_bonus_objs(const config_t &conf)
{
  string_type compile_objs = "";
  if ( !conf.bonus_objs.empty() ) {
    for ( auto &n : conf.bonus_objs ) {
      compile_objs += n;
      compile_objs += " ";
    }
    compile_objs.pop_back();
  }
  return compile_objs;
}

inline string_type
__flags_defines(const config_t &conf)
{
  string_type defines_flags;
  if ( conf.opnames ) {
    // workaround
    defines_flags += get_string_flag(gcc::preprocessor_flags::flags::d_macro);
    defines_flags += "FUNGUS_OPERATOR_NAMES ";
  }
  for ( const auto &p : conf.defines ) {
    defines_flags += get_string_flag(gcc::preprocessor_flags::flags::d_macro);
    defines_flags += p;
    defines_flags += ' ';
  }
  if ( !defines_flags.empty() ) defines_flags.pop_back();
  return defines_flags;
}

inline string_type
__flags_includes(const config_t &conf)
{
  string_type includes_location;
  for ( const auto &p : conf.include_path ) {
    includes_location += get_string_flag(gcc::preprocessor_flags::flags::i_dir);
    includes_location += p;
    includes_location += ' ';
  }
  if ( !includes_location.empty() ) includes_location.pop_back();
  return includes_location;
}

inline string_type
__flags_lib_paths(const config_t &conf, bool linking)
{
  string_type libs_location;
  if ( linking )
    for ( const auto &p : conf.lib_path ) {
      libs_location += get_string_flag(gcc::linker_flags::flags::l_dir);
      libs_location += p;
      libs_location += ' ';
    }
  if ( !libs_location.empty() ) libs_location.pop_back();
  return libs_location;
}

// x86
string_type
batch_cmp(const config_t &conf)
{
  string_type main_flags = __main_opt_flags(conf);
  __compose_add(main_flags, __isa_march(conf.isa));
  const char *comp_type = __flags_comp_type(conf);
  // -c/-S/-E never reach the linker: link inputs and link flags must stay out of those commands
  const bool linking = (conf.compile_type == __comp_type::linked);
  const string_type bin_type = __flags_bin_type(conf, linking);
  const string_type freestanding = __flags_freestanding(conf, linking);
  // -Wno-odr placed after -flto=8 so it actually disables Wodr
  const string_type freestanding_post = __flags_freestanding_post(conf);
  const string_type startup_objs = __startup_objs(conf, linking);
  const string_type arch_width = (conf.width == 64) ? make_flags(gcc::x86_flags::flags::m64) : make_flags(gcc::x86_flags::flags::m32);
  const string_type compile_libs = __flags_link_libs(conf, linking);
  const string_type compile_objs = __flags_bonus_objs(conf);
  const string_type flags_warn_base = __flags_warn_base();

  // no more useless cast + floats
  const string_type flags_warn_extra = __flags_warn_extra(conf);

  const string_type flags_errors_extra = __flags_errors_extra();

  const string_type flags_warn_ignore = __flags_warn_ignore();

  // sanitizer + hardening + perf + size + language fragment, shared across the three arch builders
  const composed_t __cz = compose(conf, linking);
  const string_type &flags_sanitize = __cz.sanitize;
  const string_type &flags_extensions = __cz.extensions;
  const string_type flags_extensions_supple = __flags_extensions_supple(conf);

  const string_type libs_location = __flags_lib_paths(conf, linking);
  const string_type defines_flags = __flags_defines(conf);
  const string_type includes_location = __flags_includes(conf);
  const string_type libs_static = __flags_libs_static(conf, linking);

  string_type command_pre = conf.warnings ? make_command(conf.compiler_path, conf.standard, comp_type, main_flags, flags_sanitize, bin_type,
                                                         freestanding, arch_width, flags_warn_base, flags_warn_extra, flags_warn_ignore,
                                                         flags_errors_extra, flags_extensions, freestanding_post, flags_extensions_supple)
                                          : make_command(conf.compiler_path, conf.standard, comp_type, main_flags, flags_sanitize, bin_type,
                                                         freestanding, arch_width, flags_warn_base, flags_warn_ignore, flags_extensions,
                                                         freestanding_post, flags_extensions_supple);

  string_type command_post = make_command(defines_flags, compile_libs, includes_location, libs_location);

  if ( conf.freestanding )
    return make_command(command_pre, conf.target, startup_objs, command_post, compile_objs, __flags_output(), conf.target_out, libs_static,
                        __cz.link_tail);
  else
    return make_command(command_pre, conf.target, command_post, compile_objs, __flags_output(), conf.target_out, libs_static,
                        __cz.link_tail);
};

// armv7
string_type
batch_cmp_armv7(const config_t &conf)
{
  string_type main_flags = __main_opt_flags(conf);
  __compose_flags(main_flags, gcc::arm_flags::flags::march_armv7_a, gcc::arm_flags::flags::mfpu_neon,
                  gcc::arm_flags::flags::mfloat_abi_hard);
  // -marm has to be on the command line
  // a #pragma GCC target("arm") does __not__ cover the compiler-synthesized .text.startup
  if ( conf.marm ) __compose_flags(main_flags, gcc::arm_flags::flags::marm);
  if ( !conf.mtp.empty() ) {
    string_type tp = get_string_flag(gcc::arm_flags::flags::mtp);
    tp += conf.mtp.c_str();
    __compose_add(main_flags, tp.c_str());
  }
  const char *comp_type = __flags_comp_type(conf);
  // -c/-S/-E never reach the linker: link inputs and link flags must stay out of those commands
  const bool linking = (conf.compile_type == __comp_type::linked);
  const string_type bin_type = __flags_bin_type(conf, linking);
  const string_type freestanding = __flags_freestanding(conf, linking);
  // -Wno-odr placed after -flto=8 so it disable Wodr
  const string_type freestanding_post = __flags_freestanding_post(conf);
  const string_type startup_objs = __startup_objs(conf, linking);
  const string_type compile_libs = __flags_link_libs(conf, linking);
  const string_type compile_objs = __flags_bonus_objs(conf);
  const string_type flags_warn_base = __flags_warn_base();

  // no more useless cast + floats
  const string_type flags_warn_extra = __flags_warn_extra(conf);

  const string_type flags_errors_extra = __flags_errors_extra();

  const string_type flags_warn_ignore = __flags_warn_ignore();

  // sanitizer + hardening + perf + size + language
  const composed_t __cz = compose(conf, linking);
  const string_type &flags_sanitize = __cz.sanitize;
  const string_type &flags_extensions = __cz.extensions;

  const string_type flags_extensions_supple = __flags_extensions_supple(conf);

  const string_type libs_location = __flags_lib_paths(conf, linking);
  const string_type defines_flags = __flags_defines(conf);
  const string_type includes_location = __flags_includes(conf);
  const string_type libs_static = __flags_libs_static(conf, linking);

  const string_type cross_flags = __clang_cross_flags(conf, linking);
  string_type command_pre
      = conf.warnings
            ? make_command(conf.compiler_path, cross_flags, conf.standard, comp_type, main_flags, flags_sanitize, bin_type, freestanding,
                           flags_warn_base, flags_warn_extra, flags_warn_ignore, flags_errors_extra, flags_extensions, freestanding_post,
                           flags_extensions_supple)
            : make_command(conf.compiler_path, cross_flags, conf.standard, comp_type, main_flags, flags_sanitize, bin_type, freestanding,
                           flags_warn_base, flags_warn_ignore, flags_extensions, freestanding_post, flags_extensions_supple);

  string_type command_post = make_command(defines_flags, compile_libs, includes_location, libs_location);

  if ( conf.freestanding )
    return make_command(command_pre, conf.target, startup_objs, command_post, compile_objs, __flags_output(), conf.target_out, libs_static,
                        __cz.link_tail);
  else
    return make_command(command_pre, conf.target, command_post, compile_objs, __flags_output(), conf.target_out, libs_static,
                        __cz.link_tail);
};

string_type
batch_cmp_aarch64(const config_t &conf)
{
  string_type main_flags = __main_opt_flags(conf);
  __compose_flags(main_flags, gcc::arm_flags::flags::march_armv8_a);
  const char *comp_type = __flags_comp_type(conf);
  // -c/-S/-E never reach the linker: link inputs and link flags must stay out of those commands
  const bool linking = (conf.compile_type == __comp_type::linked);
  const string_type bin_type = __flags_bin_type(conf, linking);
  const string_type freestanding = __flags_freestanding(conf, linking);
  const string_type freestanding_post = __flags_freestanding_post(conf);
  const string_type startup_objs = __startup_objs(conf, linking);
  const string_type compile_libs = __flags_link_libs(conf, linking);
  const string_type compile_objs = __flags_bonus_objs(conf);
  const string_type flags_warn_base = __flags_warn_base();

  const string_type flags_warn_extra = __flags_warn_extra(conf);

  const string_type flags_errors_extra = __flags_errors_extra();

  const string_type flags_warn_ignore = __flags_warn_ignore();

  // sanitizer + hardening + perf + size + language fragment, shared across the three arch builders
  const composed_t __cz = compose(conf, linking);
  const string_type &flags_sanitize = __cz.sanitize;
  const string_type &flags_extensions = __cz.extensions;

  const string_type flags_extensions_supple = __flags_extensions_supple(conf);

  const string_type libs_location = __flags_lib_paths(conf, linking);
  const string_type defines_flags = __flags_defines(conf);
  const string_type includes_location = __flags_includes(conf);
  const string_type libs_static = __flags_libs_static(conf, linking);

  const string_type cross_flags = __clang_cross_flags(conf, linking);
  string_type command_pre
      = conf.warnings
            ? make_command(conf.compiler_path, cross_flags, conf.standard, comp_type, main_flags, flags_sanitize, bin_type, freestanding,
                           flags_warn_base, flags_warn_extra, flags_warn_ignore, flags_errors_extra, flags_extensions, freestanding_post,
                           flags_extensions_supple)
            : make_command(conf.compiler_path, cross_flags, conf.standard, comp_type, main_flags, flags_sanitize, bin_type, freestanding,
                           flags_warn_base, flags_warn_ignore, flags_extensions, freestanding_post, flags_extensions_supple);

  string_type command_post = make_command(defines_flags, compile_libs, includes_location, libs_location);

  if ( conf.freestanding )
    return make_command(command_pre, conf.target, startup_objs, command_post, compile_objs, __flags_output(), conf.target_out, libs_static,
                        __cz.link_tail);
  else
    return make_command(command_pre, conf.target, command_post, compile_objs, __flags_output(), conf.target_out, libs_static,
                        __cz.link_tail);
};

string_type
batch_asm(const config_t &conf)
{
  const string_type main_flags
      = (conf.width == 64) ? make_flags(nasm::format_flags::flags::elf64) : make_flags(nasm::format_flags::flags::elf32);
  const string_type defines_flags = __flags_defines(conf);
  const string_type includes_location = __flags_includes(conf);
  string_type command_pre = make_command(conf.compiler_path, main_flags);

  string_type command_post = make_command(defines_flags, includes_location);

  return make_command(command_pre, conf.target, command_post, __flags_output(), conf.target_out);
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
  const char *comp_type = (conf.compile_type == __comp_type::object)         ? get_string_flag(gcc::driver_flags::flags::compile_only)
                          : (conf.compile_type == __comp_type::preprocessed) ? get_string_flag(gcc::driver_flags::flags::preprocess_only)
                                                                             : "";
  const bool linking = (conf.compile_type == __comp_type::linked);
  const string_type debug_flags = (conf.mode == __opt_modes::debug) ? make_flags(gcc::debug_flags::flags::g_three) : "";
  // freestanding asm links bare; hosted asm gets the libc startup files from the driver
  const string_type freestanding = (conf.freestanding and linking) ? make_flags(gcc::linker_flags::flags::nostdlib) : "";
  const string_type bin_type = __flags_bin_type(conf, linking);
  const string_type defines_flags = __flags_defines(conf);
  const string_type includes_location = __flags_includes(conf);
  const string_type compile_objs = __flags_bonus_objs(conf);
  const string_type cross_flags = __clang_cross_flags(conf, linking);

  string_type command_pre = make_command(conf.compiler_path, cross_flags, comp_type, main_flags, debug_flags, bin_type, freestanding);
  string_type command_post = make_command(defines_flags, includes_location);

  return make_command(command_pre, conf.target, command_post, compile_objs, __flags_output(), conf.target_out);
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
