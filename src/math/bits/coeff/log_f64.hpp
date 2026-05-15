//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../types.hpp"
#include "../impl.hpp"

namespace micron
{
namespace math
{
namespace mkbits
{
namespace coeff
{
namespace log_f64_data
{

inline constexpr f64 ln2_hi = 0x1.62e42fefa3800p-1;
inline constexpr f64 ln2_lo = 0x1.ef35793c76730p-45;

inline constexpr f64 Lg1 = 0x1.5555555555593p-1;
inline constexpr f64 Lg2 = 0x1.999999997fa04p-2;
inline constexpr f64 Lg3 = 0x1.2492494229359p-2;
inline constexpr f64 Lg4 = 0x1.c71c51d8e78afp-3;
inline constexpr f64 Lg5 = 0x1.7466496cb03dep-3;
inline constexpr f64 Lg6 = 0x1.39a09d078c69fp-3;
inline constexpr f64 Lg7 = 0x1.2f112df3e5244p-3;

};      // namespace log_f64_data
};      // namespace coeff
};      // namespace mkbits
};      // namespace math
};      // namespace micron
