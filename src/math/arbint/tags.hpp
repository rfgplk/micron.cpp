//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

namespace micron
{
namespace math
{

namespace solver
{

struct automatic {
};

struct basecase {
};

struct comba {
};

struct karatsuba {
};

struct toom {
};

struct nussbaumer {
};

};      // namespace solver

namespace mpn
{

enum class algo : u8 { basecase = 0, comba = 1, karatsuba = 2, toom3 = 3, toom4 = 4, nussbaumer = 5 };

enum class divalgo : u8 { sbpi1 = 0, dc = 1, mu = 2 };

enum class gcd_algo : u8 { binary = 0, lehmer = 1, dc = 2 };

enum class modalgo : u8 { redc = 0, barrett = 1 };

template<typename E>
[[nodiscard, gnu::always_inline]] inline constexpr E
clamp_to(E k, E cap) noexcept
{
  return static_cast<u8>(k) > static_cast<u8>(cap) ? cap : k;
}

};      // namespace mpn

template<typename S>
concept arb_solver
    = micron::is_same_v<S, solver::automatic> || micron::is_same_v<S, solver::basecase> || micron::is_same_v<S, solver::comba>
      || micron::is_same_v<S, solver::karatsuba> || micron::is_same_v<S, solver::toom> || micron::is_same_v<S, solver::nussbaumer>;

template<typename S>
concept arb_solver_auto = micron::is_same_v<S, solver::automatic>;

};      // namespace math
};      // namespace micron
