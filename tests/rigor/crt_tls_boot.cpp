// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
#include "../../src/types.hpp"

// Both initialized and zero TLS must be usable before main in the freestanding CRT.
thread_local u64 boot_value = 0x123456789abcdef0ull;
alignas(64) thread_local u8 boot_bytes[513];

__attribute__((constructor)) void
initialize_tls()
{
  ++boot_value;
  boot_bytes[0] = 7;
  boot_bytes[512] = 9;
}

int
main()
{
  if ( boot_value != 0x123456789abcdef1ull || boot_bytes[0] != 7 || boot_bytes[512] != 9 || (reinterpret_cast<uintptr_t>(boot_bytes) & 63) )
    return 2;
  for ( usize i = 1; i < 512; ++i )
    if ( boot_bytes[i] != 0 ) return 2;
  return 1;
}
