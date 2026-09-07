//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// MICRON ON BARE METAL: containers, hashing, sorting and printing with no OS underneath

#include "../../src/maps.hpp"
#include "../../src/print.hpp"
#include "../../src/sort/sort.hpp"
#include "../../src/strings.hpp"
#include "../../src/vector.hpp"

#if !defined(__micron_port_metal)
#error "examples/metal: this must be built with -DMICRON_PORT_METAL (duck --metal)"
#endif

#if !defined(__micron_bb_alloc)
#error "MICRON_PORT_METAL must select the barebones allocator -- see defs.hpp. abcmalloc reserves 256 GiB of VA."
#endif

namespace
{

constexpr u32 kElems = 4096;
constexpr u32 kKeys = 2048;

// deterministic, so a failure is reproducible
u64
splitmix(u64 &__s) noexcept
{
  __s += 0x9E3779B97F4A7C15ull;
  u64 __z = __s;
  __z = (__z ^ (__z >> 30)) * 0xBF58476D1CE4E5B9ull;
  __z = (__z ^ (__z >> 27)) * 0x94D049BB133111EBull;
  return __z ^ (__z >> 31);
}

struct ctor_probe {
  u32 stamp;

  ctor_probe() noexcept : stamp(0xC70C7EE5u) { }
};

ctor_probe __probe{};

};      // namespace

extern "C" int
metal_main(void)
{
  micron::println("micron-metal: image entered, no OS underneath");
  micron::println("micron-metal: port::page_size=", micron::port::page_size, " has_paging=", micron::port::has_paging ? "true" : "false");

  const auto __he = micron::port::heap_extent();
  micron::println("micron-metal: pool total=", __he.total, " free=", __he.free);

  const bool __ctors = (__probe.stamp == 0xC70C7EE5u);
  micron::println("micron-metal: .init_array ran=", __ctors ? "true" : "false");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // a vector, grown and sorted

  u64 __s = 0x243F6A8885A308D3ull;
  micron::vector<u32> __v;
  for ( u32 __i = 0; __i < kElems; ++__i ) __v.push_back(static_cast<u32>(splitmix(__s) % 100000u));
  micron::sort::sort(__v);

  bool __sorted = true;
  u64 __sum = 0;
  u32 __min = __v[0];
  u32 __max = __v[0];
  for ( u32 __i = 0; __i < kElems; ++__i ) {
    if ( __i && __v[__i] < __v[__i - 1] ) __sorted = false;
    __sum += __v[__i];
    if ( __v[__i] < __min ) __min = __v[__i];
    if ( __v[__i] > __max ) __max = __v[__i];
  }
  micron::println("micron-metal: vector n=", __v.size(), " sorted=", __sorted ? "true" : "false", " sum=", __sum, " min=", __min,
                  " max=", __max);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // a hash map

  micron::hopscotch_map<u64, u64> __m;
  u64 __want = 0;
  for ( u64 __k = 0; __k < kKeys; ++__k ) {
    __m[__k] = __k * 3ull;
    __want += __k * 3ull;
  }
  u64 __got = 0;
  for ( u64 __k = 0; __k < kKeys; ++__k ) __got += __m[__k];
  const bool __map_ok = (__got == __want) && (__m.size() == kKeys);
  micron::println("micron-metal: map n=", __m.size(), " sum=", __got, " expected=", __want, " ok=", __map_ok ? "true" : "false");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // a string, built by append

  micron::string __str{ "micron-metal" };
  for ( int __i = 0; __i < 64; ++__i ) __str += "0123456789";
  bool __str_head = true;
  {
    const char *__want = "micron-metal";
    for ( u32 __i = 0; __want[__i]; ++__i )
      if ( __str[__i] != __want[__i] ) __str_head = false;
  }
  const bool __str_ok = (__str.size() == 12 + 64 * 10) && __str_head;
  micron::println("micron-metal: string len=", __str.size(), " head_ok=", __str_head ? "true" : "false",
                  " ok=", __str_ok ? "true" : "false");

  volatile u32 __on_stack = 0;
  const bool __rd_stack = micron::port::addr_readable(const_cast<const u32 *>(&__on_stack));
  const bool __rd_static = micron::port::addr_readable(&__probe);
  const bool __rd_heap = micron::port::addr_readable(__v.data());
  const bool __rd_null = !micron::port::addr_readable(nullptr);
  const bool __rd_ok = __rd_stack && __rd_static && __rd_heap && __rd_null;
  micron::println("micron-metal: addr_readable stack=", __rd_stack ? "true" : "false", " static=", __rd_static ? "true" : "false",
                  " heap=", __rd_heap ? "true" : "false", " null_rejected=", __rd_null ? "true" : "false");

  const auto __after = micron::port::heap_extent();
  micron::println("micron-metal: pool free after=", __after.free, " (was ", __he.free, ")");

  const bool __ok = __ctors && __sorted && (__v.size() == kElems) && __map_ok && __str_ok && __rd_ok;
  micron::println(__ok ? "micron-metal: ALL CHECKS PASSED" : "micron-metal: CHECKS FAILED");

  return __ok ? 1 : 6;
}
