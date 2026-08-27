//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__internal.hpp"
#include "bits.hpp"
#include "policies.hpp"

namespace micron
{
template<typename P>
concept is_policy = requires {
  P::concurrent;
  P::shareable;
  P::on_grow;
  P::minimum_bytes;
  P::granularity;
  P::growth_numerator;
  P::growth_denominator;
};
};      // namespace micron

// clang-format off: these headers form a dependency chain.
#include "allocator_types/bits.hpp"
#include "allocator_types/__scheme.hpp"
#include "allocator_types/abc_policy_allocator.hpp"
#include "allocator_types/advanced_abc_allocators.hpp"

#include "allocator_types/constrained_allocator.hpp"
#include "allocator_types/exact_allocator.hpp"
#include "allocator_types/fixed_map_allocator.hpp"
#include "allocator_types/guarded_allocator.hpp"
#include "allocator_types/huge_allocator.hpp"
#include "allocator_types/map_allocator.hpp"
#include "allocator_types/immutable_allocator.hpp"
#include "allocator_types/secure_allocator.hpp"
#include "allocator_types/serial_allocator.hpp"
#include "allocator_types/small_allocator.hpp"
#include "allocator_types/static_allocator.hpp"
#include "allocator_types/monotonic_allocator.hpp"
#include "allocator_types/arena_resource.hpp"
// clang-format on
