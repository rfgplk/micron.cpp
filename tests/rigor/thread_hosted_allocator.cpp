//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Hosted allocator/stack ownership regression. Keep this harness-free so
// Valgrind --leak-check=full has no process-lifetime reporting noise.

#define ABCMALLOC_DISABLE 1
#define MICRON_ABC_MT 1

#include "../../src/thread/thread_types/auto_thread.hpp"

namespace
{
struct alignas(64) wide_block {
  u64 words[8];
};

[[gnu::noinline]] void
escape(const void *ptr)
{
  asm volatile("" : : "r"(ptr) : "memory");
}
}      // namespace

int
main()
{
  i64 lhs = 17;
  i64 rhs = 25;
  i64 result = 0;
  auto shard = [&lhs, &rhs, &result]() { result = lhs + rhs; };

  micron::auto_thread<> thread(shard);
  if ( thread.stack() == nullptr ) return 6;
  if ( thread.join() != 0 || result != 42 ) return 6;
  if ( thread.stack() != nullptr ) return 6;

  result = 0;
  thread[shard];
  if ( thread.join() != 0 || result != 42 ) return 6;

  auto *scalar = new u64(0x12345678u);
  escape(scalar);
  if ( *scalar != 0x12345678u ) return 6;
  delete scalar;

  auto *array = new u32[7];
  array[6] = 0xabcdefu;
  escape(array);
  if ( array[6] != 0xabcdefu ) return 6;
  delete[] array;

  auto *wide = new wide_block{};
  wide->words[7] = 0xfeedfaceu;
  escape(wide);
  if ( (reinterpret_cast<uintptr_t>(wide) & 63u) != 0 || wide->words[7] != 0xfeedfaceu ) return 6;
  delete wide;

  return 1;
}
