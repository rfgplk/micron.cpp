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

enum class constant_side : u32 {
  previous = 0,
  nearest = 1,
  next = 2,
};

enum class akima_kind : u32 {
  akima = 0,
  makima = 1,
};

enum class quadratic_boundary : u32 {
  minimum_curvature = 0,
  left_slope = 1,
  right_slope = 2,
};

enum class curve_parameterization : u32 {
  uniform = 0,
  chord_length = 1,
  centripetal = 2,
};

enum class knot_style : u32 {
  clamped_uniform = 0,
  averaged = 1,
  unclamped_uniform = 2,
  periodic_uniform = 3,
};

enum class lsq_method : u32 {
  givens_qr = 0,
  householder_qr = givens_qr,
  normal_equations = 1,
};

enum class extension_mode : u32 {
  clamp = 0,
  linear = 1,
  zero = 2,
  periodic = 3,
  reflect = 4,
  polynomial = 5,
};

template<ieee754_floating F> struct spline_domain {
  F lower{ F(0) };
  F upper{ F(0) };
  bool valid{ false };
};

struct alignas(64) spline_cursor {
  usize segment{ 0 };
  unsigned char __padding[64 - sizeof(usize)]{};
};

static_assert(sizeof(spline_cursor) == 64);
static_assert(alignof(spline_cursor) == 64);

};      // namespace splines
};      // namespace math
};      // namespace micron
