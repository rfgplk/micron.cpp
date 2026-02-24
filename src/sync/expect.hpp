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
template <memory_order O = memory_order::acq_rel, is_atomic_type T>
inline void
expect(const atomic_token<T> &tok, const T val)
{
  while ( tok.get(O) != val ) {
    cpu_pause<1000>();
  }
}

template <memory_order O = memory_order::acq_rel, is_atomic_type T>
inline void
expect(const atomic_token<T> &tok, const atomic_token<T> &o_tok)
{
  while ( tok.get(O) != o_tok.get(O) ) {
    cpu_pause<1000>();
  }
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

};     // namespace micron
