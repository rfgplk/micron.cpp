//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "limb.hpp"
#include "unsigned.hpp"

namespace micron
{
namespace math
{

namespace __arb
{

template<class T> struct is_arbuint: micron::false_type {
};

template<usize B, arb_solver S, class A> struct is_arbuint<arbuint<B, S, A>>: micron::true_type {
};

template<class T> inline constexpr bool is_arbuint_v = is_arbuint<T>::value;

template<class U>
using wide_t = arbuint<U::width_bits == 0u ? 0u : U::width_bits + mpn::limb_bits, typename U::solver_type, typename U::allocator_type>;

};      // namespace __arb

template<typename T>
concept arb_unsigned = __arb::is_arbuint_v<T>;

};      // namespace math
};      // namespace micron
