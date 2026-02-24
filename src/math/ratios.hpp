//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{

template <uintmax_t N, uintmax_t D = 1> struct ratio {
  static constexpr uintmax_t num = N;
  static constexpr uintmax_t denom = D;

protected:
  static constexpr uintmax_t
  gcd(uintmax_t a, uintmax_t b)
  {
    return b == 0 ? a : gcd(b, a % b);
  }
};

typedef ratio<static_cast<uintmax_t>(1e18), 1> exa;
typedef ratio<static_cast<uintmax_t>(1e15), 1> peta;
typedef ratio<static_cast<uintmax_t>(1e12), 1> tera;
typedef ratio<static_cast<uintmax_t>(1e9), 1> giga;
typedef ratio<static_cast<uintmax_t>(1e6), 1> mega;
typedef ratio<static_cast<uintmax_t>(1e3), 1> kilo;
typedef ratio<static_cast<uintmax_t>(1e2), 1> hecto;
typedef ratio<static_cast<uintmax_t>(1e1), 1> deca;
typedef ratio<1, 1> base_ratio;
typedef ratio<1, static_cast<uintmax_t>(1e1)> deci;
typedef ratio<1, static_cast<uintmax_t>(1e2)> centi;
typedef ratio<1, static_cast<uintmax_t>(1e3)> milli;
typedef ratio<1, static_cast<uintmax_t>(1e6)> micro;
typedef ratio<1, static_cast<uintmax_t>(1e9)> nano;
typedef ratio<1, static_cast<uintmax_t>(1e12)> pico;
typedef ratio<1, static_cast<uintmax_t>(1e15)> femto;
typedef ratio<1, static_cast<uintmax_t>(1e18)> atto;
// typedef ratio<1, static_cast<uintmax_t>(1e21)> zepto;
// typedef ratio<1, static_cast<uintmax_t>(1e24)> yocto;
// typedef ratio<1, static_cast<uintmax_t>(1e27)> ronto;
// typedef ratio<1, static_cast<uintmax_t>(1e30)> quecto;
};     // namespace micron
