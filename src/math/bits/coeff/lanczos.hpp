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
namespace lanczos_data
{

inline constexpr f64 g = 7.0;

inline constexpr f64 p[9] = {
  0.99999999999980993, 676.5203681218851,    -1259.1392167224028,   771.32342877765313,    -176.61502916214059,
  12.507343278686905,  -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7,
};

};      // namespace lanczos_data
};      // namespace coeff
};      // namespace mkbits
};      // namespace math
};      // namespace micron
