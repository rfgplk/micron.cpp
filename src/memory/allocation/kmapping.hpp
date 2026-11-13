//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "kmemory.hpp"

#if defined(__micron_arch_wasm32)
#error "micron: Linux virtual-memory mappings are unavailable on WebAssembly"
#else
#include "../mman.hpp"

namespace micron
{
inline addr_t *
map_normal(addr_t *ptr, usize bytes)
{
  return mmap(ptr, bytes, prot_read | prot_write, map_private | map_anonymous, -1, 0);
}

inline addr_t *
map_frozen(addr_t *ptr, usize bytes)
{
  return mmap(ptr, bytes, prot_read, map_private | map_anonymous, -1, 0);
}

inline addr_t *
map_large(addr_t *ptr, usize bytes)
{
  return mmap(ptr, bytes, prot_read | prot_write, map_private | map_anonymous | map_hugetlb, -1, 0);
}
};      // namespace micron
#endif
