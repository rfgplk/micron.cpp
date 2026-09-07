//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::port; running global constructors

#include "../types.hpp"

namespace micron
{
namespace port
{

using init_fn = void (*)(void);

// run [first, last), in order, skipping the null and -1 slots the toolchain may leave
inline void
run_init_array(init_fn *__first, init_fn *__last) noexcept
{
  if ( __first == nullptr || __last == nullptr ) return;
  for ( init_fn *__p = __first; __p < __last; ++__p ) {
    init_fn __f = *__p;
    if ( __f == nullptr || __f == reinterpret_cast<init_fn>(~static_cast<usize>(0)) ) continue;
    __f();
  }
}

inline void
run_fini_array(init_fn *__first, init_fn *__last) noexcept
{
  if ( __first == nullptr || __last == nullptr ) return;
  for ( init_fn *__p = __last; __p > __first; ) {
    --__p;
    init_fn __f = *__p;
    if ( __f == nullptr || __f == reinterpret_cast<init_fn>(~static_cast<usize>(0)) ) continue;
    __f();
  }
}

};      // namespace port
};      // namespace micron
