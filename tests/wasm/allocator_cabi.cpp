//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/cmalloc.hpp"

extern "C" int
main()
{
  free(nullptr);
  if ( malloc(0) != nullptr || calloc(0, 4) != nullptr ) return 6;
  errno = 0;
  if ( calloc(static_cast<usize>(-1), 2) != nullptr || errno != 12 ) return 6;
  void *out = reinterpret_cast<void *>(128);
  if ( posix_memalign(&out, 3, 16) != 22 || out != reinterpret_cast<void *>(128) ) return 6;
  if ( posix_memalign(&out, 4096, 73) != 0 || !out || (reinterpret_cast<uintptr_t>(out) & 4095u) != 0 ) return 6;
  free(out);
  errno = 0;
  if ( aligned_alloc(64, 65) != nullptr || errno != 22 ) return 6;
  out = aligned_alloc(256, 512);
  if ( out == nullptr || (reinterpret_cast<uintptr_t>(out) & 255u) != 0 ) return 6;
  free(out);
  auto *bytes = static_cast<byte *>(malloc(113));
  if ( bytes == nullptr ) return 6;
  for ( usize i = 0; i < 113; ++i ) bytes[i] = static_cast<byte>(i);
  void *failed = realloc(bytes, static_cast<usize>(-1));
  if ( failed != nullptr ) return 6;
  for ( usize i = 0; i < 113; ++i )
    if ( bytes[i] != static_cast<byte>(i) ) return 6;
  bytes = static_cast<byte *>(realloc(bytes, 16384));
  if ( bytes == nullptr ) return 6;
  for ( usize i = 0; i < 113; ++i )
    if ( bytes[i] != static_cast<byte>(i) ) return 6;
  free(bytes);
  out = pvalloc(0);
  if ( out == nullptr || (reinterpret_cast<uintptr_t>(out) & 4095u) != 0 ) return 6;
  free(out);
  errno = 0;
  if ( pvalloc(static_cast<usize>(-1)) != nullptr || errno != 12 ) return 6;
  return 1;
}
