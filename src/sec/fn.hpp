//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../function.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "bits.hpp"

namespace micron
{
namespace sec
{

using micron::operator|;

template<typename R> using __unit_if_void_t = micron::conditional_t<micron::is_void_v<R>, unit_t, micron::decay_t<R>>;

template<typename R>
concept __resultable = micron::distinct<__unit_if_void_t<R>, error_t>;

};      // namespace sec
};      // namespace micron
