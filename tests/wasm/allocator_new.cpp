//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt

#include "../../src/alloc.hpp"
#include "../../src/allocator.hpp"
#include "../../src/new.hpp"
#include "../../src/vector/vector.hpp"

namespace
{
struct alignas(65536) over_aligned {
  unsigned value = 0x12345678;

  ~over_aligned() { ++destroyed; }

  inline static unsigned destroyed = 0;
};
}      // namespace

int
main()
{
  void *zero = ::operator new(0);
  void *other_zero = ::operator new[](0);
  if ( !zero || !other_zero || zero == other_zero ) return 6;
  ::operator delete(zero);
  ::operator delete[](other_zero);
  ::operator delete(nullptr);
  ::operator delete[](nullptr);

  auto *one = new over_aligned;
  auto *many = new over_aligned[3];
  if ( reinterpret_cast<uintptr_t>(one) % alignof(over_aligned) || reinterpret_cast<uintptr_t>(many) % alignof(over_aligned) ) return 6;
  if ( one->value != 0x12345678 || many[2].value != 0x12345678 ) return 6;
  delete one;
  delete[] many;
  if ( over_aligned::destroyed != 4 ) return 6;

  for ( usize alignment = 16; alignment <= (1u << 20); alignment *= 2 ) {
    const auto a = static_cast<std::align_val_t>(alignment);
    void *p = ::operator new(37, a);
    void *q = ::operator new[](53, a);
    if ( !p || !q || reinterpret_cast<uintptr_t>(p) % alignment || reinterpret_cast<uintptr_t>(q) % alignment ) return 6;
    ::operator delete(p, 37, a);
    ::operator delete[](q, 53, a);
  }
  if ( micron::__aligned_new::__do_alloc(static_cast<usize>(-1), 64) != nullptr ) return 6;

  auto raw = micron::abc_allocator<byte>::allocate(257);
  if ( !raw.ptr || raw.len < 257 ) return 6;
  for ( usize i = 0; i < 257; ++i )
    if ( raw.ptr[i] != 0 ) return 6;
  micron::abc_allocator<byte>::deallocate(raw.ptr, raw.len);
  auto aligned = micron::abc_allocator<byte>::allocate_aligned(317, 4096);
  if ( !aligned.ptr || reinterpret_cast<uintptr_t>(aligned.ptr) % 4096 ) return 6;
  micron::abc_allocator<byte>::dealloc_aligned(aligned.ptr, 4096);
  auto *unmanaged = micron::abc_allocator<byte>::unmanaged_allocate(12288);
  if ( !unmanaged ) return 6;
  micron::abc_allocator<byte>::unmanaged_deallocate(unmanaged, 12288);
  auto *typed = micron::alloc<unsigned>(4 * sizeof(unsigned));
  if ( !typed ) return 6;
  for ( unsigned i = 0; i < 4; ++i ) typed[i] = i * 7;
  if ( typed[3] != 21 ) return 6;
  micron::free(typed);

  micron::vector<unsigned> values;
  for ( unsigned i = 0; i < 4096; ++i ) values.push_back(i ^ 0x5a5a);
  for ( unsigned i = 0; i < 4096; ++i )
    if ( values[i] != (i ^ 0x5a5a) ) return 6;
  return 1;
}
