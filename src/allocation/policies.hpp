//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "__internal.hpp"
#include "linux/kmemory.hpp"
#include "linux/sysinfo.hpp"

namespace micron
{
// NOTE: pooling has been removed
// TODO: remove shareable/concurrent unless a need for them arises

// determine how memory will be allocated, whenever a container requests more
// mem follow this policy
struct serial_allocation_policy {
  static constexpr bool concurrent = false;         // used in concurrent structures
  static constexpr f32 on_grow = 3.0f;              // by how much memory grows on each realloc (& how)
  static constexpr u32 granularity = page_size;     // minimum amount of memory alloc'd
  static constexpr u32 shareable = 0;               // can be shared between owners
};

struct constrained_allocation_policy {
  static constexpr bool concurrent = false;     // used in concurrent structures
  static constexpr f32 on_grow = 1.5f;          // by how much memory grows on each realloc (& how)
  static constexpr u32 granularity = 256;       // minimum amount of memory alloc'd
  static constexpr u32 shareable = 1;           // can be shared between owners
};

// NOTE: legacy/deprecated

// for huge pages
struct huge_allocation_policy {
  static constexpr bool concurrent = false;               // used in concurrent structures
  static constexpr f32 on_grow = 4.0f;                    // by how much memory grows on each realloc (& how)
  static constexpr u32 granularity = large_page_size;     // minimum amount of memory alloc'd
  static constexpr u32 shareable = 0;                     // can be shared between owners
};
};     // namespace micron
