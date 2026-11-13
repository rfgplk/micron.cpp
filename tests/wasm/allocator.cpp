//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/memory/allocation/wasmalloc/wasmalloc.hpp"

namespace wa = micron::wasmalloc;

namespace
{
struct allocation {
  byte *ptr{};
  usize size{};
  u8 pattern{};
};

bool
check(const allocation &a)
{
  for ( usize i = 0; i < a.size; ++i )
    if ( a.ptr[i] != static_cast<byte>(a.pattern ^ static_cast<u8>(i * 13u)) ) return false;
  return true;
}

void
fill(const allocation &a)
{
  for ( usize i = 0; i < a.size; ++i ) a.ptr[i] = static_cast<byte>(a.pattern ^ static_cast<u8>(i * 13u));
}

u32
next(u32 &seed)
{
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}
}      // namespace

extern "C" int
main()
{
  if ( wa::try_alloc(0) != nullptr ) return 6;
  wa::dealloc(nullptr);
  byte *reserved = static_cast<byte *>(micron::wasm::linear_reserve(8192, 64));
  if ( reserved == nullptr || (reinterpret_cast<uintptr_t>(reserved) & 63u) != 0 ) return 6;
  for ( usize i = 0; i < 8192; ++i ) reserved[i] = static_cast<byte>(0xc3);

  for ( usize alignment = 1; alignment <= (1u << 20); alignment *= 2 ) {
    const auto a = wa::aligned_balloc(alignment, 73);
    if ( a.ptr == nullptr || a.len < 73 || (reinterpret_cast<uintptr_t>(a.ptr) & (alignment - 1)) != 0 ) return 6;
    if ( !wa::is_present(a.ptr) || !wa::within(a.ptr) || wa::query_size(a.ptr) < 73 ) return 6;
    wa::dealloc(a.ptr);
  }
  if ( wa::aligned_balloc(3, 32).ptr != nullptr || wa::aligned_alloc(64, 65) != nullptr ) return 6;
  const u64 cursor = micron::wasm::__linear_cursor;
  if ( wa::aligned_balloc(1u << 31, 73).ptr != nullptr || wa::try_alloc(static_cast<usize>(-1)) != nullptr ) return 6;
  if ( micron::wasm::__linear_cursor != cursor ) return 6;
  if ( wa::calloc(static_cast<usize>(-1), 2) != nullptr ) return 6;
  byte *zero = static_cast<byte *>(wa::calloc(77, 13));
  if ( zero == nullptr ) return 6;
  for ( usize i = 0; i < 77 * 13; ++i )
    if ( zero[i] != byte{} ) return 6;
  wa::dealloc(zero);

  allocation items[64]{};
  u32 seed = 0x6a09e667u;
  for ( usize iteration = 0; iteration < 12000; ++iteration ) {
    allocation &a = items[next(seed) % 64];
    if ( a.ptr && !check(a) ) return 6;
    const usize size = 1 + next(seed) % 16384;
    if ( a.ptr && (next(seed) & 3u) == 0 ) {
      wa::dealloc(a.ptr);
      a = {};
    } else if ( a.ptr ) {
      byte *p = static_cast<byte *>(wa::realloc(a.ptr, size));
      if ( p == nullptr ) return 6;
      const usize keep = size < a.size ? size : a.size;
      if ( !check({ p, keep, a.pattern }) ) return 6;
      a.ptr = p;
      a.size = size;
      fill(a);
    } else {
      a = { static_cast<byte *>(wa::try_alloc(size)), size, static_cast<u8>(next(seed)) };
      if ( a.ptr == nullptr || (reinterpret_cast<uintptr_t>(a.ptr) & 15u) != 0 ) return 6;
      fill(a);
    }
    for ( usize i = 0; i < 64; ++i ) {
      if ( !items[i].ptr ) continue;
      const uintptr_t lo = reinterpret_cast<uintptr_t>(items[i].ptr), hi = lo + items[i].size;
      for ( usize j = i + 1; j < 64; ++j ) {
        if ( !items[j].ptr ) continue;
        const uintptr_t other = reinterpret_cast<uintptr_t>(items[j].ptr);
        if ( lo < other + items[j].size && other < hi ) return 6;
      }
    }
  }
  for ( auto &a : items ) {
    if ( a.ptr && !check(a) ) return 6;
    wa::dealloc(a.ptr);
  }

  allocation retained{ static_cast<byte *>(wa::try_alloc(513)), 513, 0x5a };
  if ( !retained.ptr ) return 6;
  fill(retained);
  if ( wa::realloc(retained.ptr, static_cast<usize>(-1)) != nullptr || !check(retained) ) return 6;
  if ( wa::realloc(retained.ptr, 0) != nullptr ) return 6;

  const u64 before = micron::wasm::linear_memory_bytes();
  void *large[3]{};
  for ( usize i = 0; i < 3; ++i ) {
    large[i] = wa::try_alloc(8u << 20);
    if ( large[i] == nullptr ) return 6;
  }
  if ( micron::wasm::linear_memory_bytes() <= before ) return 6;
  for ( void *ptr : large ) wa::dealloc(ptr);
  for ( usize i = 0; i < 8192; ++i )
    if ( reserved[i] != static_cast<byte>(0xc3) ) return 6;
  if ( wa::within(reserved) ) return 6;
  const u64 previous_end = micron::wasm::linear_memory_bytes();
  const auto *fresh = static_cast<byte *>(micron::wasm::linear_reserve(2 * __micron_wasm_page_size, __micron_wasm_page_size));
  if ( !fresh || reinterpret_cast<uintptr_t>(fresh) < previous_end || wa::within(fresh) ) return 6;
  for ( usize i = 0; i < 2 * __micron_wasm_page_size; ++i )
    if ( fresh[i] != byte{} ) return 6;
  return 1;
}
