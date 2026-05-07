//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "../ieee.hpp"

namespace micron
{
namespace math
{
namespace splines
{

enum class bc_kind : u32 {
  natural = 0,
  clamped = 1,
  not_a_knot = 2,
};

enum class extrap : u32 {
  clamp_to_endpoints = 0,
  linear_continue = 1,
  error_value = 2,
};

enum class build_status : u32 {
  ok = 0,
  too_few_points = 1,       // need >= 2 (linear) or >= 4 (cubic / B-spline)
  non_monotonic_x = 2,      // xs not strictly increasing
  size_mismatch = 3,        // xs and ys (or w) have different lengths
  degenerate = 4,           // duplicate or near-zero spacing
  singular_system = 5,      // banded solve detected non SPD pivot
  max_iter = 6,             // GCV, adaptive-knot loop hit cap
  invalid_argument = 7,     // bad degree
};

template <ieee754_floating F> struct build_info {
  build_status status{ build_status::ok };
  usize n_iterations{ 0 };
  F residual{ F(0) };
};

};     // namespace splines
};     // namespace math
};     // namespace micron
