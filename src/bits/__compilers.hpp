//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compiler identities and compiler specific settings/spellings

// TODO: if we add support for more compilers they should go here

#if defined(__clang__)
#define __micron_compiler_clang 1
#define __micron_compiler_clang_major __clang_major__
#define __micron_compiler_clang_minor __clang_minor__
#define __micron_compiler_clang_patch __clang_patchlevel__
#elif defined(__GNUC__)
#define __micron_compiler_gcc 1
#define __micron_compiler_gcc_major __GNUC__
#define __micron_compiler_gcc_minor __GNUC_MINOR__
#define __micron_compiler_gcc_patch __GNUC_PATCHLEVEL__
#elif defined(_MSC_VER)
#define __micron_compiler_msvc 1
#define __micron_compiler_msvc_ver _MSC_VER
#elif defined(__INTEL_COMPILER) || defined(__ICC)
#define __micron_compiler_icc 1
#define __micron_compiler_icc_ver __INTEL_COMPILER
#elif defined(__INTEL_LLVM_COMPILER)
#define __micron_compiler_icx 1
#else
#define __micron_compiler_unknown 1
#endif

#if defined(__micron_compiler_gcc) || defined(__micron_compiler_clang)
#define __micron_compiler_gcc_compat 1
#endif

#define __micron_pragma_(x) _Pragma(#x)

#if defined(__micron_compiler_clang)
#define __micron_diagnostic_push __micron_pragma_(clang diagnostic push)
#define __micron_diagnostic_ignored(w) __micron_pragma_(clang diagnostic ignored w)
#define __micron_diagnostic_nan __micron_diagnostic_ignored("-Wnan-infinity-disabled")
#define __micron_diagnostic_pop __micron_pragma_(clang diagnostic pop)
#define __micron_loop_ivdep __micron_pragma_(clang loop vectorize(enable))
#define __micron_push_options
#define __micron_pop_options
#define __micron_optimize_no_fast_math
#define __micron_optimize_no_unsafe_math
#define __micron_optimize_no_tree_loop_distribute
#define __micron_gcc_unroll_4
#define __micron_gcc_unroll_8
#define __micron_gcc_target_avx512
#define __micron_externally_visible
#define __micron_optimize_O0
#else
#define __micron_diagnostic_push __micron_pragma_(GCC diagnostic push)
#define __micron_diagnostic_ignored(w) __micron_pragma_(GCC diagnostic ignored w)
#define __micron_diagnostic_nan
#define __micron_diagnostic_pop __micron_pragma_(GCC diagnostic pop)
#define __micron_loop_ivdep __micron_pragma_(GCC ivdep)
#define __micron_push_options __micron_pragma_(GCC push_options)
#define __micron_pop_options __micron_pragma_(GCC pop_options)
#define __micron_optimize_no_fast_math                                                                                                     \
  __micron_pragma_(GCC optimize("no-fast-math", "no-associative-math", "no-reciprocal-math", "signed-zeros"))
#define __micron_optimize_no_unsafe_math                                                                                                   \
  __micron_pragma_(GCC optimize("-fno-unsafe-math-optimizations", "-fno-associative-math", "-fno-reciprocal-math"))
#define __micron_optimize_no_tree_loop_distribute __attribute__((optimize("-fno-tree-loop-distribute-patterns")))
#define __micron_gcc_unroll_4 __micron_pragma_(GCC unroll 4)
#define __micron_gcc_unroll_8 __micron_pragma_(GCC unroll 8)
#define __micron_gcc_target_avx512 __micron_pragma_(GCC target("avx512f,avx512bw,avx512dq,avx512vl"))
#define __micron_externally_visible __attribute__((externally_visible))
#define __micron_optimize_O0 __attribute__((optimize("O0")))
#endif

#if defined(__micron_compiler_gcc)
#define __micron_diagnostic_suggest_noreturn __micron_diagnostic_ignored("-Wsuggest-attribute=noreturn")
#else
#define __micron_diagnostic_suggest_noreturn
#endif

#if defined(__micron_compiler_clang)
#define __micron_builtin_is_trivially_destructible(T) __is_trivially_destructible(T)
#else
#define __micron_builtin_is_trivially_destructible(T) __has_trivial_destructor(T)
#endif
