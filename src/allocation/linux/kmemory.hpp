//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/mman.hpp"

#include "../../except.hpp"
#include "../../types.hpp"
#include "bits.hpp"

namespace micron
{

constexpr size_t page_size = 4096;                      // generally is this on linux
constexpr size_t large_page_size = 2 * 1024 * 1024;     // likewise
constexpr size_t alloc_auto_sz = page_size;
constexpr size_t alloc_auto_large_sz = large_page_size;

template <typename T> using chunk = micron::__chunk<T>;

template <typename T>
chunk<byte>
to_chunk(T *ptr, size_t len)
{
  return chunk<byte>(reinterpret_cast<byte *>(ptr), len);
}

inline addr_t *
map_normal(addr_t *ptr, const size_t n)
{
  return reinterpret_cast<addr_t *>(micron::mmap(ptr, n, prot_read | prot_write, map_private | map_anonymous, -1, 0));
};

inline addr_t *
map_frozen(addr_t *ptr, const size_t n)
{
  return reinterpret_cast<addr_t *>(micron::mmap(ptr, n, prot_read, map_private | map_anonymous, -1, 0));
};

inline addr_t *
map_large(addr_t *ptr, const size_t n)
{
  return reinterpret_cast<addr_t *>(
      micron::mmap(ptr, n, prot_read | prot_write, map_private | map_anonymous | map_hugetlb, -1, 0));
};
};     // namespace micron
