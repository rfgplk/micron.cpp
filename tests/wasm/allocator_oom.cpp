//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/memory/allocation/wasmalloc/wasmalloc.hpp"

extern "C" int
main()
{
  namespace wa = micron::wasmalloc;
  auto *retained = static_cast<byte *>(wa::try_alloc(128));
  if ( retained == nullptr ) return 6;
  for ( usize i = 0; i < 128; ++i ) retained[i] = static_cast<byte>(i ^ 0x5a);
  void *large[256]{};
  usize count = 0;
  for ( ; count < 256; ++count ) {
    large[count] = wa::try_alloc(16u << 20);
    if ( large[count] == nullptr ) break;
  }
  if ( count == 0 || count == 256 ) return 6;
  const u64 cursor = micron::wasm::__linear_cursor;
  if ( wa::realloc(retained, 256u << 20) != nullptr ) return 6;
  if ( micron::wasm::__linear_cursor != cursor ) return 6;
  for ( usize i = 0; i < 128; ++i )
    if ( retained[i] != static_cast<byte>(i ^ 0x5a) ) return 6;
  for ( usize i = 0; i < count; ++i ) wa::dealloc(large[i]);
  auto *reused = wa::try_alloc(16u << 20);
  if ( reused == nullptr || micron::wasm::__linear_cursor != cursor ) return 6;
  wa::dealloc(reused);
  wa::dealloc(retained);
  if ( micron::wasm::linear_reserve(static_cast<usize>(-1), 16) != nullptr ) return 6;
  if ( micron::wasm::__linear_cursor != cursor ) return 6;
  return 1;
}
