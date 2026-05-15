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
namespace log_f32_data
{

inline constexpr f32 ln2_hi = 0x1.62e000p-1f;
inline constexpr f32 ln2_lo = 0x1.0bfbe8p-15f;

inline constexpr f32 Lg1 = 0xaaaaaa.0p-24f;
inline constexpr f32 Lg2 = 0xccce13.0p-25f;
inline constexpr f32 Lg3 = 0x91e9eep-25f;
inline constexpr f32 Lg4 = 0xf89e26.0p-26f;

};      // namespace log_f32_data
};      // namespace coeff
};      // namespace mkbits
};      // namespace math
};      // namespace micron
