//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt

#include "../../src/allocator.hpp"
#include "../../src/new.hpp"

#if !defined(__micron_wasm_eh)
#error "allocator_eh requires the native WebAssembly exception profile"
#endif

namespace
{
void *held[16384]{};
unsigned destroyed = 0;

struct scope {
  ~scope() { ++destroyed; }
};
}      // namespace

int
main()
{
  volatile usize impossible = static_cast<usize>(-1);
  unsigned caught = 0;
  try {
    scope s;
    (void)::operator new(impossible);
    return 6;
  } catch ( const micron::except::memory_error_new_scalar & ) {
    ++caught;
  }
  try {
    scope s;
    (void)::operator new[](impossible);
    return 6;
  } catch ( const micron::except::memory_error_new_array & ) {
    ++caught;
  }
  try {
    scope s;
    (void)::operator new(impossible, static_cast<std::align_val_t>(64));
    return 6;
  } catch ( const micron::except::memory_error_new_scalar_aligned & ) {
    ++caught;
  }
  try {
    scope s;
    (void)::operator new[](impossible, static_cast<std::align_val_t>(64));
    return 6;
  } catch ( const micron::except::memory_error_new_array_aligned & ) {
    ++caught;
  }
  try {
    scope s;
    (void)micron::abc_allocator<byte>::allocate(impossible);
    return 6;
  } catch ( const micron::except::memory_error & ) {
    ++caught;
  }
  if ( caught != 5 || destroyed != 5 ) return 6;

  auto *retained = micron::wasmalloc::alloc(127);
  if ( !retained ) return 6;
  __builtin_memset(retained, 0x6d, 127);
  usize count = 0;
  for ( usize size = 16u << 20; size >= 16; size /= 2 ) {
    while ( auto *p = micron::wasmalloc::alloc(size) ) {
      if ( count == sizeof(held) / sizeof(held[0]) ) return 6;
      held[count++] = p;
    }
  }
  if ( !count || micron::wasmalloc::try_alloc(16) != nullptr ) return 6;
  // With every heap tier exhausted, exception objects must use the runtime's
  // fixed emergency storage, and the throwing allocation must release its lock.
  for ( unsigned i = 0; i < 32; ++i ) {
    try {
      scope s;
      (void)::operator new(16);
      return 6;
    } catch ( const micron::except::memory_error_new_scalar & ) {
      ++caught;
    }
  }
  if ( caught != 37 || destroyed != 37 ) return 6;
  for ( usize i = 0; i < 127; ++i )
    if ( retained[i] != 0x6d ) return 6;
  for ( usize i = 0; i < count; ++i ) micron::wasmalloc::dealloc(held[i]);
  micron::wasmalloc::dealloc(retained);
  auto *again = new unsigned(19);
  if ( !again || *again != 19 ) return 6;
  delete again;
  return 1;
}
