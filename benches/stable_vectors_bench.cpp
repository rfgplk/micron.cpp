//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/vector/reserved_vector.hpp"
#include "../src/vector/segmented_vector.hpp"

struct record {
  u32 a{}, b{}, c{}, d{};
  u64 value{};
};

static volatile u64 sink;

u64
now()
{
  micron::timespec_t t{};
  micron::clock_gettime(micron::clock_monotonic, t);
  return static_cast<u64>(t.tv_sec) * 1000000000 + t.tv_nsec;
}

template<class Container>
void
measure(const char *name, Container &values, usize count)
{
  const u64 begin = now();
  for ( usize size = 1; size < count; size *= 2 )
    if ( !values.try_resize(size) ) __builtin_trap();
  if ( !values.try_resize(count) ) __builtin_trap();
  for ( usize i = 0; i < count; ++i ) values[i].value = i;
  const u64 growth = now() - begin;
  u64 total = 0, random = 0x5120041;
  const u64 scan_begin = now();
  for ( u32 repeat = 0; repeat < 64; ++repeat )
    for ( usize i = 0; i < count; ++i ) total += values[i].value;
  const u64 scan = now() - scan_begin;
  const u64 random_begin = now();
  for ( usize i = 0; i < count * 64; ++i ) {
    random = random * 6364136223846793005ull + 1;
    total += values[(random >> 32) & (count - 1)].value;
  }
  const u64 random_time = now() - random_begin;
  sink = total;
  micron::console(name, ",", count, ",", growth, ",", scan, ",", random_time, ",", total);
}

int
main()
{
  micron::console("layout,count,growth_ns,scan_ns,random_ns,checksum");
  for ( u32 repeat = 0; repeat < 9; ++repeat )
    for ( const usize count : { usize{ 1024 }, usize{ 16384 }, usize{ 262144 } } ) {
      micron::reserved_vector<record> contiguous;
      if ( !contiguous.initialize(usize{ 1 } << 28) ) return 2;
      micron::segmented_vector<record, 256> chunks;
      if ( repeat & 1 ) {
        measure("chunked", chunks, count);
        measure("contiguous", contiguous, count);
      } else {
        measure("contiguous", contiguous, count);
        measure("chunked", chunks, count);
      }
    }
  return 1;
}
