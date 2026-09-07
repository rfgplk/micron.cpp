//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// page acquisition and the heap's extent, inside a Linux kernel module
//
// NOTE: this file must NOT include memory/mman.hpp or allocation/kmapping.hpp

#include "__kport_abi.hpp"

#include "../../memory/allocation/kmemory.hpp"

#include "../../memory/mmap_bits.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

constexpr usize page_size = micron::page_size;
constexpr usize large_page_size = micron::large_page_size;

constexpr bool has_paging = true;

struct page_span {
  addr_t *ptr;
  usize len;

  bool
  failed(void) const noexcept
  {
    return ptr == nullptr;
  }
};

inline page_span
page_alloc(usize __n, bool __huge = false) noexcept
{
  void *__p = ::mc_kport_page_alloc(static_cast<mc_kport_usize>(__n), __huge ? 1 : 0);
  if ( __p == nullptr ) return page_span{ nullptr, 0 };
  return page_span{ reinterpret_cast<addr_t *>(__p), __n };
}

inline void
page_free(page_span __s) noexcept
{
  if ( __s.ptr != nullptr ) ::mc_kport_page_free(__s.ptr, static_cast<mc_kport_usize>(__s.len));
}

inline page_span
page_reserve(usize __n) noexcept
{
  return page_alloc(__n, false);
}

inline bool
page_commit(addr_t *__slot, usize __n) noexcept
{
  (void)__n;
  return __slot != nullptr;
}

inline void
page_decommit(addr_t *__slot, usize __n) noexcept
{
  (void)__slot;
  (void)__n;
}

inline bool
page_protect(addr_t *__p, usize __n, int __prot) noexcept
{
  return ::mc_kport_page_protect(__p, static_cast<mc_kport_usize>(__n), __prot) != 0;
}

inline void
page_discard(addr_t *__p, usize __n) noexcept
{
  ::mc_kport_page_discard(__p, static_cast<mc_kport_usize>(__n));
}

struct heap_size {
  usize total;
  usize free;
};

inline heap_size
heap_extent(void) noexcept
{
  mc_kport_usize __total = 0;
  mc_kport_usize __free = 0;
  ::mc_kport_heap_extent(&__total, &__free);
  return heap_size{ static_cast<usize>(__total), static_cast<usize>(__free) };
}

inline bool
addr_readable(const void *__p) noexcept
{
  return ::mc_kport_addr_readable(__p) != 0;
}

};      // namespace port
};      // namespace micron
