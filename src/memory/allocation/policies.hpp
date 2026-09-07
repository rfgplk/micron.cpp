//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../defs.hpp"      // __micron_bb_alloc -- see the barebones policy below
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the default under every container, and the one number that decides whether micron fits on a
// device.
//
// allocation_policy<page_size, page_size, 3, 1> means an empty mc::vector<u8> costs a full page:
// 4 KiB, or 64 KiB on arm64 (__micron_page_size_default, bits/__arch.hpp). On a 256 KiB MCU that is
// four vectors. So the barebones tier retunes it, and the numbers are not arbitrary:
//
//   minimum 64    a cache line, and what bb_allocator::auto_size() answers. 16 would give a hash
//                 map one bucket (robin.hpp divides auto_size by the node size).
//   granularity 16  the allocator's native alignment, so capacity <= grant with no rounding loss.
//   growth 2/1    a power-of-two step from a power-of-two base is zero waste in a buddy tier;
//                 3/1 lands between size classes and rounds up into the next one.
//
// GATED ON __micron_bb_alloc, WHICH IS WHY defs.hpp IS INCLUDED ABOVE. This header did not include
// it before, and a `#if defined(__micron_bb_alloc)` without that include is not a conservative
// default -- it is dead text that silently always takes the else branch. Verified: the macro is not
// visible here without it.
#if defined(__micron_bb_alloc)
struct barebones_allocation_policy: allocation_policy<64, 16, 2, 1> {
};

struct serial_allocation_policy: barebones_allocation_policy {
};
#else
struct serial_allocation_policy: allocation_policy<page_size, page_size, 3, 1> {
};
#endif

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
