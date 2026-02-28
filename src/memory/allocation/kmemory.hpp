//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/mman.hpp"

#include "../../numerics.hpp"

#include "../../types.hpp"

namespace micron
{

constexpr usize page_size = 4096;                      // generally is this on linux
constexpr usize large_page_size = 2 * 1024 * 1024;     // likewise
constexpr usize alloc_auto_sz = page_size;
constexpr usize alloc_auto_large_sz = large_page_size;

template <typename T = byte> struct alignas(16) __chunk {     // total memory allocated
  T *ptr;
  usize len;

  __chunk &
  operator=(nullptr_t t [[maybe_unused]])
  {
    ptr = nullptr;
    len = 0;
    return *this;
  }

  bool
  zero(void) const noexcept
  {
    if ( ptr == nullptr or len == 0 )
      return true;
    return false;
  }

  bool
  failed_allocation(void) const noexcept
  {
    return ((ptr == micron::numeric_limits<byte *>::max() - 1) and len == 0xFF);
  }

  bool
  invalid(void) const noexcept
  {
    return (ptr == (byte *)-1);
  }
};

template <typename T> using chunk = micron::__chunk<T>;

template <typename T>
chunk<byte>
to_chunk(T *ptr, usize len)
{
  return chunk<byte>(reinterpret_cast<byte *>(ptr), len);
}

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
};     // namespace micron
