//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// MICRON ON AN MCU: containers, hashing, sorting and printing in 192 KiB of SRAM
//
// This is metal_demo.cpp with the numbers changed, and the numbers are the point. That file asks
// for an 8 MiB pool and builds a 4096-element vector, a 2048-entry map and a 652-byte string; an
// STM32F405 has 192 KiB of SRAM in total, so running it here is not a matter of it being slow.
// The shape of the test is identical and every assertion is the same assertion.
//
// The budget, and it is deliberately not the whole of SRAM: 48 KiB of pool against 192 KiB of part,
// which leaves the 8 KiB stack metal_stm32.ld reserves plus room for the image's own .data/.bss.
// bb's TINY profile (min block 16, sheet 4 KiB, one region) is what --metal selects here without
// being asked.
//
//   duck build examples/metal/metal_stm32_demo.cpp examples/metal/board_stm32.cpp \
//        --cortex-m cortex-m4 --metal --start ./start -i . -i ./src \
//        --def MICRON_BB_PORT_POOL=49152 -Os -o bin
//   qemu-system-arm -M netduinoplus2 -kernel bin/... -nographic \
//        -semihosting-config enable=on,target=native

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

#if !defined(__ARM_ARCH_PROFILE) || (__ARM_ARCH_PROFILE != 'M')
#error "metal_stm32_demo.cpp is sized for an MCU -- build it with duck --cortex-m (metal_demo.cpp is the board-class one)"
#endif

namespace
{

constexpr u32 kElems = 512;
constexpr u32 kKeys = 256;

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

// a non-zero initialised global, which is the ONLY thing that proves the flash -> SRAM .data copy
// in reset_cortexm.s ran. On the A-profile boards LMA == VMA and that loop never executes; here it
// is the difference between initialised data and whatever the SRAM powered up holding. A zero
// initialiser would prove nothing -- .bss is zeroed by a different loop.
volatile u32 __data_probe = 0xD47A5EEDu;

};      // namespace

extern "C" int
metal_main(void)
{
  micron::println("micron-metal: image entered, no OS underneath");
  micron::println("micron-metal: port::page_size=", micron::port::page_size, " has_paging=", micron::port::has_paging ? "true" : "false");

  const auto __he = micron::port::heap_extent();
  micron::println("micron-metal: pool total=", __he.total, " free=", __he.free);

  const bool __ctors = (__probe.stamp == 0xC70C7EE5u);
  const bool __data_ok = (__data_probe == 0xD47A5EEDu);
  micron::println("micron-metal: .init_array ran=", __ctors ? "true" : "false", " .data copied=", __data_ok ? "true" : "false");

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
  for ( int __i = 0; __i < 16; ++__i ) __str += "0123456789";
  bool __str_head = true;
  {
    const char *__wanted = "micron-metal";
    for ( u32 __i = 0; __wanted[__i]; ++__i )
      if ( __str[__i] != __wanted[__i] ) __str_head = false;
  }
  const bool __str_ok = (__str.size() == 12 + 16 * 10) && __str_head;
  micron::println("micron-metal: string len=", __str.size(), " head_ok=", __str_head ? "true" : "false",
                  " ok=", __str_ok ? "true" : "false");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // the clock has to move, and on this target that is not free
  //
  // The weak default in start/metal/mc_mport.cpp returns ++counter -- monotonic, and useless.
  // board_stm32.cpp overrides it with DWT_CYCCNT. If the override did not take, the two readings
  // below differ by the number of intervening calls rather than by elapsed time, and a spin of a
  // few thousand iterations is nowhere near a microsecond.

  const i64 __t0 = micron::port::mono_ticks();
  for ( volatile u32 __i = 0; __i < 20000u; ++__i ) { }
  const i64 __t1 = micron::port::mono_ticks();
  const bool __clock_ok = (__t1 - __t0) > 1000;      // ns
  micron::println("micron-metal: clock delta=", __t1 - __t0, "ns real=", __clock_ok ? "true" : "false");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // port::addr_readable, on the three kinds of address a board actually has
  //
  // It answered false for the stack until 2026-09-07 -- the linker scripts put the stack below
  // __heap_start, so it fell outside the only range the check knew about. Its one caller in the
  // tree is snowball's stack walker, probing frames, so the walker could not walk one. A local's
  // address is the cheapest way to ask the question that failed.

  volatile u32 __on_stack = 0;
  const bool __rd_stack = micron::port::addr_readable(const_cast<const u32 *>(&__on_stack));
  const bool __rd_static = micron::port::addr_readable(&__probe);
  const bool __rd_heap = micron::port::addr_readable(__v.data());
  const bool __rd_null = !micron::port::addr_readable(nullptr);
  const bool __rd_ok = __rd_stack && __rd_static && __rd_heap && __rd_null;
  micron::println("micron-metal: addr_readable stack=", __rd_stack ? "true" : "false", " static=", __rd_static ? "true" : "false",
                  " heap=", __rd_heap ? "true" : "false", " null_rejected=", __rd_null ? "true" : "false");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // the pool must come back

  const auto __after = micron::port::heap_extent();
  micron::println("micron-metal: pool free after=", __after.free, " (was ", __he.free, ")");

  const bool __ok = __ctors && __data_ok && __sorted && (__v.size() == kElems) && __map_ok && __str_ok && __clock_ok && __rd_ok;
  micron::println(__ok ? "micron-metal: ALL CHECKS PASSED" : "micron-metal: CHECKS FAILED");

  // 1 is micron's PASS sentinel, and board_stm32.cpp hands it to the host through semihosting
  return __ok ? 1 : 6;
}
