//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// page acquisition, on bare metal

#include "__mport_abi.hpp"

#include "../irq.hpp"

#include "../../memory/allocation/kmemory.hpp"

#include "../../memory/mmap_bits.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

constexpr usize page_size = micron::page_size;
constexpr usize large_page_size = micron::large_page_size;

constexpr bool has_paging = false;

struct page_span {
  addr_t *ptr;
  usize len;

  bool
  failed(void) const noexcept
  {
    return ptr == nullptr;
  }
};

namespace __bits
{

struct __page_node {
  __page_node *next;
  usize len;
};

static_assert(page_size >= sizeof(__page_node), "micron::port (metal): MICRON_PORT_PAGE_SIZE is smaller than a free-list node -- the "
                                                "carve stores its links inside free blocks, so the granule must hold two words");

inline __page_node *__pool_head = nullptr;
inline byte *__pool_lo = nullptr;
inline byte *__pool_hi = nullptr;
inline usize __pool_avail = 0;
inline bool __pool_ready = false;

[[gnu::always_inline]] inline usize
__round_up(usize __n, usize __a) noexcept
{
  return (__n + __a - 1) & ~(__a - 1);
}

inline void
__pool_init(void) noexcept
{
  if ( __pool_ready ) return;
  __pool_ready = true;

  void *__b = nullptr;
  mc_mport_usize __l = 0;
  ::mc_mport_heap(&__b, &__l);
  if ( __b == nullptr || static_cast<usize>(__l) < page_size * 2 ) return;

  byte *__lo = reinterpret_cast<byte *>(__round_up(reinterpret_cast<usize>(__b), page_size));
  byte *__hi = reinterpret_cast<byte *>((reinterpret_cast<usize>(__b) + static_cast<usize>(__l)) & ~(page_size - 1));
  if ( __hi <= __lo ) return;

  __pool_lo = __lo;
  __pool_hi = __hi;
  __page_node *__n = reinterpret_cast<__page_node *>(__lo);
  __n->next = nullptr;
  __n->len = static_cast<usize>(__hi - __lo);
  __pool_head = __n;
  __pool_avail = __n->len;
}

};      // namespace __bits

inline page_span
page_alloc(usize __n, bool __huge = false) noexcept
{
  (void)__huge;
  if ( __n == 0 ) return page_span{ nullptr, 0 };

  const usize __want = __bits::__round_up(__n, page_size);
  const irq_state __st = irq_save();
  __bits::__pool_init();

  __bits::__page_node *__prev = nullptr;
  __bits::__page_node *__cur = __bits::__pool_head;
  while ( __cur != nullptr ) {
    if ( __cur->len >= __want ) {
      byte *__base;
      if ( __cur->len == __want ) {
        __base = reinterpret_cast<byte *>(__cur);
        if ( __prev == nullptr )
          __bits::__pool_head = __cur->next;
        else
          __prev->next = __cur->next;
      } else {
        __cur->len -= __want;
        __base = reinterpret_cast<byte *>(__cur) + __cur->len;
      }
      __bits::__pool_avail -= __want;
      irq_restore(__st);
      return page_span{ reinterpret_cast<addr_t *>(__base), __want };
    }
    __prev = __cur;
    __cur = __cur->next;
  }

  irq_restore(__st);
  return page_span{ nullptr, 0 };
}

inline void
page_free(page_span __s) noexcept
{
  if ( __s.ptr == nullptr || __s.len == 0 ) return;

  byte *__base = reinterpret_cast<byte *>(__s.ptr);
  const usize __len = __bits::__round_up(__s.len, page_size);
  const irq_state __st = irq_save();
  __bits::__pool_init();

  if ( __base < __bits::__pool_lo || __base + __len > __bits::__pool_hi ) {
    irq_restore(__st);
    return;
  }

  if ( (reinterpret_cast<usize>(__base) & (page_size - 1)) != 0 ) {
    irq_restore(__st);
    return;
  }

  __bits::__page_node *__prev = nullptr;
  __bits::__page_node *__cur = __bits::__pool_head;
  while ( __cur != nullptr && reinterpret_cast<byte *>(__cur) < __base ) {
    __prev = __cur;
    __cur = __cur->next;
  }

  // __cur == __base                          an exact double free
  // __prev ends after __base                 the span starts inside the previous node
  // __base + __len ends after __cur          the span runs into the following node
  if ( (__cur != nullptr && reinterpret_cast<byte *>(__cur) == __base)
       || (__prev != nullptr && reinterpret_cast<byte *>(__prev) + __prev->len > __base)
       || (__cur != nullptr && __base + __len > reinterpret_cast<byte *>(__cur)) ) {
    irq_restore(__st);
    return;
  }

  __bits::__page_node *__n = reinterpret_cast<__bits::__page_node *>(__base);
  __n->len = __len;
  __n->next = __cur;
  if ( __prev == nullptr )
    __bits::__pool_head = __n;
  else
    __prev->next = __n;

  if ( __cur != nullptr && __base + __n->len == reinterpret_cast<byte *>(__cur) ) {
    __n->len += __cur->len;
    __n->next = __cur->next;
  }
  if ( __prev != nullptr && reinterpret_cast<byte *>(__prev) + __prev->len == __base ) {
    __prev->len += __n->len;
    __prev->next = __n->next;
  }

  __bits::__pool_avail += __len;
  irq_restore(__st);
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
  (void)__p;
  (void)__n;
  (void)__prot;
  return false;
}

inline void
page_discard(addr_t *__p, usize __n) noexcept
{
  (void)__p;
  (void)__n;
}

struct heap_size {
  usize total;
  usize free;
};

inline heap_size
heap_extent(void) noexcept
{
  const irq_state __st = irq_save();
  __bits::__pool_init();
  const usize __total = static_cast<usize>(__bits::__pool_hi - __bits::__pool_lo);
  const usize __free = __bits::__pool_avail;
  irq_restore(__st);
  return heap_size{ __total, __free };
}

// %%%%%%%%%%%%%%%%%%%
// addr_readable
extern "C" {
[[gnu::weak]] extern char __stack_bottom[];
[[gnu::weak]] extern char __stack_top[];
[[gnu::weak]] extern char __data_start[];
[[gnu::weak]] extern char __bss_end[];
};

inline bool
addr_readable(const void *__p) noexcept
{
  if ( __p == nullptr ) return false;
  const byte *__b = reinterpret_cast<const byte *>(__p);

  // the stack, and then static data -- both outside the pool, both ordinary memory
  if ( __stack_bottom != nullptr && __stack_top != nullptr && __b >= reinterpret_cast<const byte *>(__stack_bottom)
       && __b < reinterpret_cast<const byte *>(__stack_top) )
    return true;
  if ( __data_start != nullptr && __bss_end != nullptr && __b >= reinterpret_cast<const byte *>(__data_start)
       && __b < reinterpret_cast<const byte *>(__bss_end) )
    return true;

  const irq_state __st = irq_save();
  __bits::__pool_init();
  const bool __r = __b >= __bits::__pool_lo && __b < __bits::__pool_hi;
  irq_restore(__st);
  return __r;
}

};      // namespace port
};      // namespace micron
