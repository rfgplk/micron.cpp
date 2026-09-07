//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

#include "../mman.hpp"

namespace micron
{

inline addr_t *
map_normal(addr_t *ptr, const usize n)
{
  return (micron::mmap(ptr, n, prot_read | prot_write, map_private | map_anonymous, -1, 0));
};

inline addr_t *
map_frozen(addr_t *ptr, const usize n)
{
  return (micron::mmap(ptr, n, prot_read, map_private | map_anonymous, -1, 0));
};

inline addr_t *
map_large(addr_t *ptr, const usize n)
{
  return (micron::mmap(ptr, n, prot_read | prot_write, map_private | map_anonymous | map_hugetlb, -1, 0));
};
};      // namespace micron
