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
namespace exp_f32_data
{

inline constexpr f32 ln2_16_hi = 0x1.62e400p-5f;
inline constexpr f32 ln2_16_lo = 0x1.800000p-24f;
inline constexpr f32 inv_ln2_16 = 0x1.715476p+4f;

inline constexpr f32 twoN[16] = {
  0x1.000000p+0f, 0x1.0b5586p+0f, 0x1.172b84p+0f, 0x1.2387a6p+0f, 0x1.306fe0p+0f, 0x1.3dea64p+0f, 0x1.4bfdaep+0f, 0x1.5ab07ep+0f,
  0x1.6a09e6p+0f, 0x1.7a1148p+0f, 0x1.8ace54p+0f, 0x1.9c4918p+0f, 0x1.ae8a00p+0f, 0x1.c199bep+0f, 0x1.d5818ep+0f, 0x1.ea4afap+0f,
};

inline constexpr poly_coeffs<f32, 1> rem = { {
    0x1.000000p-1f,
    0x1.555556p-3f,
} };

};      // namespace exp_f32_data
};      // namespace coeff
};      // namespace mkbits
};      // namespace math
};      // namespace micron
