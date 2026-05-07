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
namespace exp_f64_data
{

inline constexpr f64 ln2_32_hi = 0x1.62e42fefa0000p-6;
inline constexpr f64 ln2_32_lo = 0x1.cf79ac0000000p-45;
inline constexpr f64 inv_ln2_32 = 0x1.71547652b82fep+5;

// 2^(i/32)
inline constexpr f64 twoN[32] = {
  0x1.0000000000000p+0, 0x1.059b0d3158574p+0, 0x1.0b5586cf9890fp+0, 0x1.11301d0125b51p+0, 0x1.172b83c7d517bp+0, 0x1.1d4873168b9aap+0,
  0x1.2387a6e756238p+0, 0x1.29e9df51fdee1p+0, 0x1.306fe0a31b715p+0, 0x1.371a7373aa9cbp+0, 0x1.3dea64c123422p+0, 0x1.44e086061892dp+0,
  0x1.4bfdad5362a27p+0, 0x1.5342b569d4f82p+0, 0x1.5ab07dd485429p+0, 0x1.6247eb03a5585p+0, 0x1.6a09e667f3bcdp+0, 0x1.71f75e8ec5f74p+0,
  0x1.7a11473eb0187p+0, 0x1.82589994cce13p+0, 0x1.8ace5422aa0dbp+0, 0x1.93737b0cdc5e5p+0, 0x1.9c49182a3f090p+0, 0x1.a5503b23e255dp+0,
  0x1.ae89f995ad3adp+0, 0x1.b7f76f2fb5e47p+0, 0x1.c199bdd85529cp+0, 0x1.cb720dcef9069p+0, 0x1.d5818dcfba487p+0, 0x1.dfc97337b9b5fp+0,
  0x1.ea4afa2a490dap+0, 0x1.f50765b6e4540p+0,
};

inline constexpr poly_coeffs<f64, 4> rem = { {
    0x1.0000000000000p-1,
    0x1.5555555555555p-3,
    0x1.5555555555555p-5,
    0x1.1111111111111p-7,
    0x1.6c16c16c16c17p-10,
} };

};     // namespace exp_f64_data
};     // namespace coeff
};     // namespace mkbits
};     // namespace math
};     // namespace micron
