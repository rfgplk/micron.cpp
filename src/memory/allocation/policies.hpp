//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "kmemory.hpp"

namespace micron
{

template<usize MinimumBytes, usize Granularity, usize GrowthNumerator, usize GrowthDenominator> struct allocation_policy {
  static_assert(Granularity != 0, "allocation_policy: granularity must be non-zero");
  static_assert(GrowthDenominator != 0, "allocation_policy: growth denominator must be non-zero");
  static_assert(GrowthNumerator >= GrowthDenominator, "allocation_policy: growth ratio must be at least one");

  static constexpr bool concurrent = false;
  static constexpr bool shareable = false;
  static constexpr usize minimum_bytes = MinimumBytes;
  static constexpr usize granularity = Granularity;
  static constexpr usize growth_numerator = GrowthNumerator;
  static constexpr usize growth_denominator = GrowthDenominator;

  // Kept for source compatibility. Integer fields above are canonical.
  static constexpr f32 on_grow = static_cast<f32>(GrowthNumerator) / static_cast<f32>(GrowthDenominator);
};

struct serial_allocation_policy: allocation_policy<page_size, page_size, 3, 1> {
};

struct small_allocation_policy: allocation_policy<512, 512, 2, 1> {
};

struct constrained_allocation_policy: allocation_policy<256, 256, 3, 2> {
  static constexpr bool shareable = true;
};

struct huge_allocation_policy: allocation_policy<large_page_size, large_page_size, 4, 1> {
};

struct exact_allocation_policy: allocation_policy<0, 1, 1, 1> {
};

};      // namespace micron
