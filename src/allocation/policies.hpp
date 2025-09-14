//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__internal.hpp"
#include "linux/kmemory.hpp"
#include "linux/sysinfo.hpp"
#include "../concepts.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

#define ALLOC_FREE 0
#define ALLOC_USED 1

#define POOL_SERIAL 0
#define POOL_LINKED 1
#define POOL_NONE 0xFF
// determine how memory will be allocated, whenever a container requests more
// mem follow this policy
struct serial_allocation_policy {
  static constexpr bool concurrent = false;              // used in concurrent structures
  static constexpr uint32_t on_grow = 3;                 // by how much memory grows on each realloc (& how)
  static constexpr uint32_t pooling = POOL_SERIAL;       // what type of pool
  static constexpr uint32_t granularity = page_size;     // minimum amount of memory alloc'd
  static constexpr uint32_t shareable = 0;               // can be shared between owners
};

// for huge pages
struct huge_allocation_policy {
  static constexpr bool concurrent = false;                    // used in concurrent structures
  static constexpr uint32_t on_grow = 4;                       // by how much memory grows on each realloc (& how)
  static constexpr uint32_t pooling = POOL_SERIAL;             // what type of pool
  static constexpr uint32_t granularity = large_page_size;     // minimum amount of memory alloc'd
  static constexpr uint32_t shareable = 0;                     // can be shared between owners
};

// for allocating nearly all RAM
struct total_allocation_policy {
  static constexpr bool concurrent = false;              // used in concurrent structures
  static constexpr uint32_t on_grow = 0;                 // cannot grow by definition
  static constexpr uint32_t pooling = POOL_NONE;         // what type of pool
  static constexpr uint32_t granularity = page_size;     // minimum amount of memory alloc'd
  static constexpr uint32_t shareable = 1;               // can be shared between owners
};

struct tiny_allocation_policy {
  static constexpr bool concurrent = false;            // used in concurrent structures
  static constexpr uint32_t on_grow = 2;               // by how much memory grows on each realloc (& how)
  static constexpr uint32_t pooling = POOL_SERIAL;     // what type of pool
  static constexpr uint32_t granularity = 256;         // minimum amount of memory alloc'd
  static constexpr uint32_t shareable = 1;             // can be shared between owners
};

struct linked_allocation_policy {
  static constexpr bool concurrent = true;               // used in concurrent structures
  static constexpr uint32_t on_grow = 3;                 // by how much memory grows on each realloc (& how)
  static constexpr uint32_t pooling = POOL_LINKED;       // what type of pool
  static constexpr uint32_t granularity = page_size;     // minimum amount of memory alloc'd
  static constexpr uint32_t shareable = 1;               // can be shared between owners
};

};     // namespace micron
