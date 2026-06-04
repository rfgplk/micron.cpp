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

template<memory_order O = memory_order::acquire, is_atomic_type T>
inline void
expect(const atomic_token<T> &tok, const T val)
{
  static_assert(O != memory_order::release && O != memory_order::acq_rel,
                "expect(): order is used for a pure load; release/acq_rel are invalid load orders");
  while ( tok.get(O) != val ) {
    cpu_pause<1000>();
  }
}

template<memory_order O = memory_order::acquire, is_atomic_type T>
inline void
expect(const atomic_token<T> &tok, const atomic_token<T> &o_tok)
{
  static_assert(O != memory_order::release && O != memory_order::acq_rel,
                "expect(): order is used for a pure load; release/acq_rel are invalid load orders");
  while ( tok.get(O) != o_tok.get(O) ) {
    cpu_pause<1000>();
  }
}

// keep these helpers
template<typename T, typename F, typename R, typename... Args>
inline bool
expect(const T &t, const F &f, R r, Args &&...args)
{
  if ( t == f ) {
    r(args...);
    return true;
  }
  return false;
}

template<typename T, typename F, typename... Args>
inline bool
expect(const T &t, const F &f)
{
  return (f == t);
}

};      // namespace micron
