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
namespace atan_f64_data
{

inline constexpr f64 aT[11] = {
  3.33333333333329318027e-01, -1.99999999998764832476e-01, 1.42857142725034663711e-01, -1.11111104054623557880e-01,
  9.09088713343650656196e-02, -7.69187620504482999495e-02, 6.66107313738753120669e-02, -5.83357013379057348645e-02,
  4.97687799461593236017e-02, -3.65315727442169155270e-02, 1.62858201153657823623e-02,
};

inline constexpr f64 atan_lo[5] = {
  0.0, 0x1.dac670561bb4fp-2, 0x1.921fb54442d18p-1, 0x1.f730bd281f69bp-1, 0x1.921fb54442d18p+0,
};

};      // namespace atan_f64_data
};      // namespace coeff
};      // namespace mkbits
};      // namespace math
};      // namespace micron
