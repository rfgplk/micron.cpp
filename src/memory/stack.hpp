//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "stack_constants.hpp"

namespace micron
{
#if defined(__micron_freestanding)

inline constinit stack_t __micron_main_stack{ nullptr, 0 };

inline auto
get_stack(void) -> stack_t
{
  if ( __micron_main_stack.start != nullptr ) return __micron_main_stack;

  constexpr uintptr_t pg = 4096;
  const uintptr_t hi = (reinterpret_cast<uintptr_t>(__builtin_frame_address(0)) + (pg - 1)) & ~(pg - 1);
  const usize size = static_cast<usize>(default_stack_size);
  const uintptr_t lo = hi - static_cast<uintptr_t>(size);
  return stack_t{ reinterpret_cast<addr_t *>(lo), size };
}

#else

auto get_stack(void) -> stack_t;

#endif

inline auto
get_stack_start(void) -> addr_t *
{
  return get_stack().start;
}

inline auto
get_stack_size(void) -> usize
{
  return get_stack().size;
}
};      // namespace micron
