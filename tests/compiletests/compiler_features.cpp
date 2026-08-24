//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate for the compiler feature vocabulary shared by every arch and build mode.

#include "../../src/bits/__arch.hpp"

#if defined(__micron_compiler_clang)
static_assert(__micron_compiler_clang_major >= 1);
#elif defined(__micron_compiler_gcc)
static_assert(__micron_compiler_gcc_major >= 1);
#else
#error "the compiler feature vocabulary must identify the active compiler"
#endif

#if defined(__micron_arch_x86_any)
__micron_push_options
__micron_gcc_target_avx512
__micron_pop_options
#endif

__micron_optimize_no_fast_math
__micron_optimize_no_unsafe_math
__micron_optimize_no_tree_loop_distribute
inline int
__compiler_feature_function(int n) noexcept
{
  int result = 0;
  __micron_loop_ivdep
  __micron_gcc_unroll_4
  for ( int i = 0; i < n; ++i ) result += i;
  return result;
}

int
main()
{
  return __compiler_feature_function(4) == 6 ? 1 : 0;
}
