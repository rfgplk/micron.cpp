// Copyright (c) 2026 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "config.hpp"

#include "../../../types.hpp"

#if defined(MICRON_BB_PORT_POOL) || defined(MICRON_BB_PORT_GROW)
#include "../../../port/pages.hpp"
#endif

namespace bb
{

struct __pool_desc {
  byte *mem;
  usize len;
  u8 *tags;
  usize tag_cap;
  bool zeroed;
};

#if defined(MICRON_BB_STATIC_POOL)
template <usize Id> struct __pool_store {
  alignas(__default_max_alignment) static inline byte mem[MICRON_BB_STATIC_POOL];
  static inline u8 tags[MICRON_BB_STATIC_POOL / __default_min_block];
};
static_assert(MICRON_BB_STATIC_POOL >= 4 * __default_tlsf_sheet, "bb: static pool too small");
#endif

#if defined(MICRON_BB_LINKER_POOL)
extern "C" byte __heap_start[];
extern "C" byte __heap_end[];
#endif

inline __pool_desc
__configured_pool() noexcept
{
#if defined(MICRON_BB_STATIC_POOL)
  using store = __pool_store<__abi_id>;
  return { store::mem, sizeof(store::mem), store::tags, sizeof(store::tags), true };
#elif defined(MICRON_BB_LINKER_POOL)
  const uintptr_t lo = reinterpret_cast<uintptr_t>(&__heap_start[0]);
  const uintptr_t hi = reinterpret_cast<uintptr_t>(&__heap_end[0]);
  if( hi <= lo )
    return { nullptr, 0, nullptr, 0, false };
  return { __heap_start, static_cast<usize>(hi - lo), nullptr, 0, false };
#elif defined(MICRON_BB_PORT_POOL)
  auto s = micron::port::page_alloc(static_cast<usize>(MICRON_BB_PORT_POOL));
  if( s.failed() )
    return { nullptr, 0, nullptr, 0, false };
  return { reinterpret_cast<byte *>(s.ptr), s.len, nullptr, 0, false };
#else
  return { nullptr, 0, nullptr, 0, false };
#endif
}

#if defined(MICRON_BB_PORT_GROW)

constexpr bool __growable = true;
constexpr usize __grow_bytes = static_cast<usize>(MICRON_BB_PORT_GROW);

static_assert(__grow_bytes >= 4 * __default_tlsf_sheet, "bb: MICRON_BB_PORT_GROW is below four sheets");
static_assert(__grow_bytes <= __default_max_region_bytes, "bb: MICRON_BB_PORT_GROW exceeds the region cap");

inline __pool_desc
__grow_pool(usize bytes) noexcept
{
  usize n = bytes < __grow_bytes ? __grow_bytes : bytes;
  if( n > __default_max_region_bytes )
    n = __default_max_region_bytes;
  if( n < bytes )
    return { nullptr, 0, nullptr, 0, false };
  auto s = micron::port::page_alloc(n);
  if( s.failed() )
    return { nullptr, 0, nullptr, 0, false };
  return { reinterpret_cast<byte *>(s.ptr), s.len, nullptr, 0, false };
}

#else

constexpr bool __growable = false;

inline __pool_desc
__grow_pool(usize) noexcept
{
  return { nullptr, 0, nullptr, 0, false };
}

#endif

};
