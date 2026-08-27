//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../atomic/flag.hpp"
#include "../../../memory/mman.hpp"
#include "../../../numerics.hpp"
#include "../../../types.hpp"

namespace abc
{

enum class external_provenance : u8 { none, caller, fixed, guarded, huge, secure, immutable };

struct __external_range {
  byte *base;
  usize len;
  external_provenance provenance;
};

struct __external_page {
  __external_page *next;
  usize capacity;
  usize used;
  usize high_water;
  __external_range ranges[1];
};

inline micron::atomic_flag __external_lock{};
inline __external_page *__external_pages = nullptr;
inline usize __external_count = 0;

class __external_guard
{
public:
  __external_guard() noexcept
  {
    while ( __external_lock.test_and_set(micron::memory_order::acquire) ) __cpu_pause();
  }

  ~__external_guard() { __external_lock.clear(micron::memory_order::release); }

  __external_guard(const __external_guard &) = delete;
  __external_guard &operator=(const __external_guard &) = delete;
};

[[nodiscard]] inline bool
__external_bounds(byte *base, usize len, uintptr_t &lo, uintptr_t &hi) noexcept
{
  if ( base == nullptr || len == 0 ) return false;
  lo = reinterpret_cast<uintptr_t>(base);
  if ( len > micron::numeric_limits<uintptr_t>::max() - lo ) return false;
  hi = lo + len;
  return hi > lo;
}

[[nodiscard]] inline bool
__external_overlap(uintptr_t alo, uintptr_t ahi, const __external_range &range) noexcept
{
  if ( range.base == nullptr ) return false;
  const uintptr_t blo = reinterpret_cast<uintptr_t>(range.base);
  const uintptr_t bhi = blo + range.len;
  return alo < bhi && blo < ahi;
}

[[nodiscard, gnu::cold, gnu::noinline]] inline __external_page *
__external_new_page() noexcept
{
  constexpr usize header = __builtin_offsetof(__external_page, ranges);
  const usize mapping_len = micron::page_size < header + sizeof(__external_range) ? header + sizeof(__external_range) : micron::page_size;
  addr_t *memory
      = micron::mmap(nullptr, mapping_len, micron::prot_read | micron::prot_write, micron::map_private | micron::map_anonymous, -1, 0);
  if ( micron::mmap_failed(memory) ) return nullptr;
  auto *page = reinterpret_cast<__external_page *>(memory);
  page->next = nullptr;
  page->capacity = (mapping_len - header) / sizeof(__external_range);
  page->used = 0;
  page->high_water = 1;
  for ( usize i = 0; i < page->capacity; ++i ) page->ranges[i] = { nullptr, 0, external_provenance::none };
  return page;
}

[[nodiscard, gnu::noinline]] inline bool
__external_register_slow(byte *base, usize len, external_provenance provenance, uintptr_t lo, uintptr_t hi) noexcept
{
  __external_range *vacant = nullptr;
  __external_page *vacant_page = nullptr;
  __external_page *tail_vacant = nullptr;
  for ( __external_page *page = __external_pages; page; page = page->next ) {
    for ( usize i = 0; i < page->high_water; ++i ) {
      __external_range &range = page->ranges[i];
      if ( __external_overlap(lo, hi, range) ) return false;
      if ( vacant == nullptr && range.base == nullptr ) {
        vacant = &range;
        vacant_page = page;
      }
    }
    if ( vacant == nullptr && tail_vacant == nullptr && page->high_water < page->capacity ) tail_vacant = page;
  }

  if ( vacant == nullptr && tail_vacant ) {
    vacant = &tail_vacant->ranges[tail_vacant->high_water++];
    vacant_page = tail_vacant;
  }

  if ( vacant == nullptr ) {
    __external_page *page = __external_new_page();
    if ( page == nullptr ) return false;
    page->next = __external_pages;
    __external_pages = page;
    vacant = &page->ranges[0];
    vacant_page = page;
  }

  *vacant = { base, len, provenance };
  ++vacant_page->used;
  ++__external_count;
  return true;
}

[[nodiscard, gnu::always_inline]] inline bool
register_external(byte *base, usize len, external_provenance provenance = external_provenance::caller) noexcept
{
  uintptr_t lo;
  uintptr_t hi;
  if ( !__external_bounds(base, len, lo, hi) ) return false;

  __external_guard guard;
  if ( __external_count == 0 && __external_pages ) {
    __external_page *page = __external_pages;
    page->ranges[0] = { base, len, provenance };
    page->used = 1;
    page->high_water = 1;
    __external_count = 1;
    return true;
  }
  return __external_register_slow(base, len, provenance, lo, hi);
}

[[nodiscard, gnu::noinline]] inline bool
__external_unregister_slow(byte *base, usize len) noexcept
{
  for ( __external_page *page = __external_pages; page; page = page->next ) {
    for ( usize i = 0; i < page->high_water; ++i ) {
      __external_range &range = page->ranges[i];
      if ( range.base != base ) continue;
      if ( range.len != len ) return false;
      range = { nullptr, 0, external_provenance::none };
      --page->used;
      --__external_count;
      if ( i + 1 == page->high_water )
        while ( page->high_water && page->ranges[page->high_water - 1].base == nullptr ) --page->high_water;
      return true;
    }
  }
  return false;
}

[[nodiscard, gnu::always_inline]] inline bool
unregister_external(byte *base, usize len) noexcept
{
  if ( base == nullptr || len == 0 ) return false;
  __external_guard guard;
  __external_page *page = __external_pages;
  if ( page && page->high_water && page->ranges[0].base == base ) {
    __external_range &range = page->ranges[0];
    if ( range.len != len ) return false;
    range = { nullptr, 0, external_provenance::none };
    --page->used;
    --__external_count;
    if ( page->high_water == 1 ) page->high_water = 0;
    return true;
  }
  return __external_unregister_slow(base, len);
}

[[nodiscard, gnu::cold]] inline bool
external_is_present(const byte *base) noexcept
{
  if ( base == nullptr ) return false;
  __external_guard guard;
  for ( const __external_page *page = __external_pages; page; page = page->next )
    for ( usize i = 0; i < page->high_water; ++i )
      if ( page->ranges[i].base == base ) return true;
  return false;
}

[[nodiscard, gnu::cold]] inline bool
external_within(const byte *address) noexcept
{
  if ( address == nullptr ) return false;
  const uintptr_t target = reinterpret_cast<uintptr_t>(address);
  __external_guard guard;
  for ( const __external_page *page = __external_pages; page; page = page->next ) {
    for ( usize i = 0; i < page->high_water; ++i ) {
      const __external_range &range = page->ranges[i];
      if ( range.base == nullptr ) continue;
      const uintptr_t lo = reinterpret_cast<uintptr_t>(range.base);
      if ( target >= lo && target - lo < range.len ) return true;
    }
  }
  return false;
}

[[nodiscard, gnu::cold]] inline usize
external_query_size(const byte *base) noexcept
{
  if ( base == nullptr ) return 0;
  __external_guard guard;
  for ( const __external_page *page = __external_pages; page; page = page->next )
    for ( usize i = 0; i < page->high_water; ++i )
      if ( page->ranges[i].base == base ) return page->ranges[i].len;
  return 0;
}

[[nodiscard, gnu::cold]] inline external_provenance
external_query_provenance(const byte *base) noexcept
{
  if ( base == nullptr ) return external_provenance::none;
  __external_guard guard;
  for ( const __external_page *page = __external_pages; page; page = page->next )
    for ( usize i = 0; i < page->high_water; ++i )
      if ( page->ranges[i].base == base ) return page->ranges[i].provenance;
  return external_provenance::none;
}

};      // namespace abc
