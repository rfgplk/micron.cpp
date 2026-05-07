//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// sin(r) r + r**3 * (S1 + r**2xS2 + r**4xS3 + r**6xS4 + r**8xS5 + r**10xS6)
// cos(r) 1 - r**2/2 + r**4 * (C1 + r**2xC2 + r**4xC3 + r**6xC4 + r**8xC5 + r**10xC6)

#include "../../../types.hpp"

namespace micron
{
namespace math
{
namespace mkbits
{
namespace coeff
{
namespace sin_f64_data
{

// sin Remez kernel
inline constexpr f64 S1 = -0x1.5555555555549p-3;     // -1/6
inline constexpr f64 S2 = 0x1.111111110f8a6p-7;      // 1/120
inline constexpr f64 S3 = -0x1.a01a019c161d5p-13;
inline constexpr f64 S4 = 0x1.71de357b1fe7dp-19;
inline constexpr f64 S5 = -0x1.ae5e68a2b9cebp-26;
inline constexpr f64 S6 = 0x1.5d93a5acfd57cp-33;

// cos Remez kernel
inline constexpr f64 C1 = 0x1.555555555554cp-5;       // 1/24
inline constexpr f64 C2 = -0x1.6c16c16c15177p-10;     // -1/720
inline constexpr f64 C3 = 0x1.a01a019cb1590p-16;
inline constexpr f64 C4 = -0x1.27e4f809c52adp-22;
inline constexpr f64 C5 = 0x1.1ee9ebdb4b1c4p-29;
inline constexpr f64 C6 = -0x1.8fae9be8838d4p-37;

};     // namespace sin_f64_data
};     // namespace coeff
};     // namespace mkbits
};     // namespace math
};     // namespace micron
