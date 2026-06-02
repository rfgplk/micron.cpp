//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// memoizing fixpoint combinator
#include "../maps/robin.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{
namespace __impl
{
// open-recursion carrier with a result cache keyed by the (single) argument
template<typename Arg, typename R, typename F> struct __memo_fix {
  F __fn;
  mutable micron::robin_map<Arg, R> __cache;

  R
  operator()(const Arg &a) const
  {
    if ( const R *hit = __cache.find(a) ) return *hit;
    auto self = [this](const Arg &x) -> R { return (*this)(x); };
    R val = __fn(self, a);
    __cache.insert(a, R(val));
    return val;
  }
};
};      // namespace __impl

template<typename Arg, typename R, typename F>
auto
memo_fix(F &&f)
{
  return __impl::__memo_fix<Arg, R, micron::decay_t<F>>{ micron::forward<F>(f), {} };
}
};      // namespace micron
