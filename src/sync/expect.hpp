//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "futex.hpp"
#include "pause.hpp"

namespace micron
{

// swap to seq_cst if needed
template <is_atomic_type T, memory_order O = memory_order::acq_rel>
inline void
expect(const atomic_token<T> &tok, const T val)
{
  while ( tok.get(O) != val )
    wait_futex(tok.ptr(), val);
}

// keep these helpers
template <typename T, typename F, typename R, typename... Args>
inline bool
expect(const T &t, const F &f, R r, Args &&...args)
{
  if ( t == f ) {
    r(args...);
    return true;
  }
  return false;
}

template <typename T, typename F, typename... Args>
inline bool
expect(const T &t, const F &f)
{
  return (f == t);
}

};
