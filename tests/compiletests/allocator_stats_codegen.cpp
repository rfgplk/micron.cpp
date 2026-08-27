//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/allocator.hpp"

namespace
{
inline micron::arena_resource<micron::allocator_exact<>> codegen_arena{ 4096 };
}

extern "C" [[gnu::noinline]] micron::chunk<byte>
allocator_stats_codegen_allocate(usize bytes)
{
  return micron::allocator_traits<micron::allocator_exact<>>::allocate<16>(bytes);
}

extern "C" [[gnu::noinline]] void
allocator_stats_codegen_deallocate(micron::chunk<byte> memory)
{
  micron::allocator_traits<micron::allocator_exact<>>::deallocate<16>(memory);
}

extern "C" [[gnu::noinline]] micron::chunk<byte>
arena_stats_codegen_allocate(usize bytes)
{
  return codegen_arena.allocate<16>(bytes);
}

int
main()
{
  micron::chunk<byte> memory = allocator_stats_codegen_allocate(32);
  allocator_stats_codegen_deallocate(memory);
  (void)arena_stats_codegen_allocate(32);
  codegen_arena.release();
  return 0;
}
