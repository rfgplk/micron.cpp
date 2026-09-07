//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// page acquisition and the heap's extent, on userspace Linux

#include "../../memory/allocation/kmapping.hpp"

#include "../../memory/allocation/kmemory.hpp"

#include "../../memory/mman.hpp"

#include "__syscall.hpp"

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
  addr_t *__p = __huge ? micron::map_large(nullptr, __n) : micron::map_normal(nullptr, __n);
  if ( micron::mmap_failed(__p) ) return page_span{ nullptr, 0 };
  return page_span{ __p, __n };
}

inline void
page_free(page_span __s) noexcept
{
  if ( __s.ptr != nullptr ) (void)micron::munmap(__s.ptr, __s.len);
}

inline page_span
page_reserve(usize __n) noexcept
{
  addr_t *__p = micron::mmap(nullptr, __n, micron::prot_none, micron::map_private | micron::map_anonymous | 0x4000, -1, 0);
  if ( micron::mmap_failed(__p) ) return page_span{ nullptr, 0 };
  return page_span{ __p, __n };
}

inline bool
page_commit(addr_t *__slot, usize __n) noexcept
{
  addr_t *__got = micron::mmap(__slot, __n, micron::prot_read | micron::prot_write,
                               micron::map_private | micron::map_anonymous | micron::map_fixed, -1, 0);
  return !micron::mmap_failed(__got) && __got == __slot;
}

inline void
page_decommit(addr_t *__slot, usize __n) noexcept
{
  (void)micron::mmap(__slot, __n, micron::prot_none, micron::map_private | micron::map_anonymous | micron::map_fixed | 0x4000, -1, 0);
}

inline bool
page_protect(addr_t *__p, usize __n, int __prot) noexcept
{
  return micron::mprotect(__p, __n, __prot) == 0;
}

inline void
page_discard(addr_t *__p, usize __n) noexcept
{
  (void)micron::madvise(__p, __n, micron::madv_dontneed);
}

struct heap_size {
  usize total;
  usize free;
};

namespace __bits
{

struct __sysinfo_raw {
  kernel_long_t uptime;
  kernel_ulong_t loads[3];
  kernel_ulong_t totalram;
  kernel_ulong_t freeram;
  kernel_ulong_t sharedram;
  kernel_ulong_t bufferram;
  kernel_ulong_t totalswap;
  kernel_ulong_t freeswap;
  u16 procs;
  u16 pad;
  kernel_ulong_t totalhigh;
  kernel_ulong_t freehigh;
  u32 mem_unit;
#if __wordsize == 32
  char _f[8];
#endif
};
};      // namespace __bits

inline heap_size
heap_extent(void) noexcept
{
  __bits::__sysinfo_raw __si{};
  if ( micron::syscall(SYS_sysinfo, &__si) != 0 ) return heap_size{ 0, 0 };
  const usize __unit = __si.mem_unit ? static_cast<usize>(__si.mem_unit) : 1u;
  return heap_size{ static_cast<usize>(__si.totalram) * __unit, static_cast<usize>(__si.freeram) * __unit };
}

inline bool
addr_readable(const void *__p) noexcept
{
  if ( __p == nullptr ) return false;
  unsigned char __v = 0;
  const usize __mask = ~(static_cast<usize>(page_size) - 1);
  const usize __a = reinterpret_cast<usize>(__p) & __mask;
  return micron::syscall(SYS_mincore, reinterpret_cast<void *>(__a), 1, &__v) == 0;
}

};      // namespace port
};      // namespace micron
