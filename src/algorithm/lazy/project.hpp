//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "source.hpp"
#include "transform.hpp"

#include "../../tuple.hpp"
#include "../fpalgorithm.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// keys / values

namespace micron
{
namespace lz
{

namespace __impl
{

struct __key_of {
  template<typename P>
  constexpr decltype(auto)
  operator()(P &&__p) const
  {
    return __first_of(micron::forward<P>(__p));
  }
};

struct __value_of {
  template<typename P>
  constexpr decltype(auto)
  operator()(P &&__p) const
  {
    return __second_of(micron::forward<P>(__p));
  }
};

};      // namespace __impl

[[nodiscard]] constexpr auto
keys()
{
  return fmap(__impl::__key_of{});
}

[[nodiscard]] constexpr auto
values()
{
  return fmap(__impl::__value_of{});
}

template<typename R>
[[nodiscard]] constexpr auto
keys(R &&__r)
{
  return keys()(micron::forward<R>(__r));
}

template<typename R>
[[nodiscard]] constexpr auto
values(R &&__r)
{
  return values()(micron::forward<R>(__r));
}

using micron::fp::on;

};      // namespace lz
};      // namespace micron
