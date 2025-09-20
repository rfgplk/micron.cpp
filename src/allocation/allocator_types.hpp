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
template <typename P>
concept is_policy = requires {
  { P::concurrent } -> micron::same_as<const bool &>;
  { P::on_grow } -> micron::same_as<const uint32_t &>;
  { P::pooling } -> micron::same_as<const uint32_t &>;
  { P::granularity } -> micron::same_as<const uint32_t &>;
};
};

#include "allocator_types/bits.hpp"

#include "allocator_types/map_allocator.hpp"

#include "allocator_types/huge_allocator.hpp"
#include "allocator_types/serial_allocator.hpp"
#include "allocator_types/tiny_allocator.hpp"
#include "allocator_types/total_allocator.hpp"
