//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// math_defects_hw_asm.cpp
// hw::sqrt_ss's x86 arm was gated on bare __micron_arch_x86_any, so an x86 build without SSE picked
// the sqrtss asm and died on "impossible constraint in 'asm'"; the ladder's own scalar arm three
// lines below was unreachable. That capability-absent target is also the one micron's simd stack
// #errors out of (simd/memory.hpp:12), so snowball cannot be built there -- on it this file reduces
// to hw.hpp plus a main and the COMPILE is the gate. Everywhere else the values below still pin
// whichever arm of the ladder the target selected.

#include "../../src/bits/__arch.hpp"
#include "../../src/math/__asm/hw.hpp"

namespace hw = micron::math::hw;

#if defined(__micron_arch_x86_any) && !defined(__micron_x86_sse)
#define __mc_hw_no_sse_x86 1
#endif

// volatile in, volatile out: the first defeats the `if consteval` body, the second forces the result
// down to its own format on an x87 target, where __FLT_EVAL_METHOD__ is 2 and a float otherwise
// stays 80-bit in st(0) all the way into the comparison
volatile f32 g_f32 = 0.0f;
volatile f64 g_f64 = 0.0;

[[gnu::noinline]] static f32
rt_sqrt_ss(f32 x) noexcept
{
  g_f32 = x;
  g_f32 = hw::sqrt_ss(g_f32);
  return g_f32;
}

[[gnu::noinline]] static f64
rt_sqrt_sd(f64 x) noexcept
{
  g_f64 = x;
  g_f64 = hw::sqrt_sd(g_f64);
  return g_f64;
}

// every arm of the ladder -- sqrtss/sqrtsd, x87 fsqrt, vsqrt.f32/f64, neon, __builtin_sqrt* -- has
// to agree on these; the f64 ones are chosen clear of the halfway cases where an x87 fsqrt at 64-bit
// precision control double-rounds
static bool
sqrt_values_hold() noexcept
{
  if ( rt_sqrt_ss(0.0f) != 0.0f ) return false;
  if ( rt_sqrt_ss(1.0f) != 1.0f ) return false;
  if ( rt_sqrt_ss(4.0f) != 2.0f ) return false;
  if ( rt_sqrt_ss(2.0f) != 0x1.6a09e6p+0f ) return false;
  if ( rt_sqrt_ss(0.5f) != 0x1.6a09e6p-1f ) return false;
  if ( rt_sqrt_ss(0x1.2a05f2p+33f) != 0x1.86ap+16f ) return false;

  if ( rt_sqrt_sd(0.0) != 0.0 ) return false;
  if ( rt_sqrt_sd(1.0) != 1.0 ) return false;
  if ( rt_sqrt_sd(4.0) != 2.0 ) return false;
  if ( rt_sqrt_sd(2.0) != 0x1.6a09e667f3bcdp+0 ) return false;
  if ( rt_sqrt_sd(0.5) != 0x1.6a09e667f3bcdp-1 ) return false;
  if ( rt_sqrt_sd(3.0) != 0x1.bb67ae8584caap+0 ) return false;
  if ( rt_sqrt_sd(0x1.56e1fc2f8f359p-997) != 0x1.a2fe76a3f9475p-499 ) return false;
  if ( rt_sqrt_sd(0x1.7e43c8800759cp+996) != 0x1.38d352e5096afp+498 ) return false;

  // an exact square must come back exactly, on every arm and at every magnitude
  for ( int k = 1; k <= 2048; ++k ) {
    const f32 sf = f32(k) * f32(k);
    if ( rt_sqrt_ss(sf) != f32(k) ) return false;
    const f64 sd = f64(k) * f64(k);
    if ( rt_sqrt_sd(sd) != f64(k) ) return false;
  }
  return true;
}

#if defined(__mc_hw_no_sse_x86)

// snowball is unbuildable on this target; reaching main at all is the finding
int
main()
{
  if ( !sqrt_values_hold() ) return 0;
  return 1;
}

#else

#include "../snowball/snowball.hpp"

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::skip;
using ::sb::test_case;

int
main()
{
  print("=== HW ASM SQRT LADDER ===");

  test_case("sqrt_ss / sqrt_sd values on the arm this target selected");
  {
    require_true(sqrt_values_hold());
  }
  end_test_case();

  test_case("negative control: the comparison above is live");
  {
    require_true(rt_sqrt_ss(4.0f) != 3.0f);
    require_true(rt_sqrt_sd(4.0) != 3.0);
  }
  end_test_case();

  test_case("x86 without SSE reaches a live sqrt arm");
  {
#if defined(__micron_arch_x86_any)
    skip("this x86 build has SSE; the capability-absent ladder cannot be exercised from here");
#else
    skip("not an x86 target");
#endif
  }
  end_test_case();

  print("=== DONE ===");
  return 1;
}

#endif
