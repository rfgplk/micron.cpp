//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../types.hpp"

namespace micron
{
namespace math
{
namespace mkbits
{
namespace coeff
{
namespace sin_f32_data
{

inline constexpr f32 S1 = -0x1.555556p-3f;     // -1/6
inline constexpr f32 S2 = 0x1.111100p-7f;      // 1/120
inline constexpr f32 S3 = -0x1.a01a00p-13f;
inline constexpr f32 S4 = 0x1.71de40p-19f;

inline constexpr f32 C1 = 0x1.555556p-5f;       // 1/24
inline constexpr f32 C2 = -0x1.6c16c0p-10f;     // -1/720
inline constexpr f32 C3 = 0x1.a01a00p-16f;
inline constexpr f32 C4 = -0x1.27e4fcp-22f;

};     // namespace sin_f32_data
};     // namespace coeff
};     // namespace mkbits
};     // namespace math
};     // namespace micron
