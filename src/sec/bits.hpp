//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../errno.hpp"
#include "../sum.hpp"
#include "../tuple.hpp"
#include "../types.hpp"

#include "../io/bits.hpp"

namespace micron
{
namespace sec
{

using error_t = micron::io::error_t;

template<typename T> using result = micron::option<T, error_t>;
using unit_t = micron::tuple<>;

[[nodiscard]] inline result<unit_t>
to_unit(i32 r)
{
  if ( r < 0 ) [[unlikely]]
    return result<unit_t>{ error_t(r) };
  return result<unit_t>{ unit_t{} };
}

};      // namespace sec
};      // namespace micron
