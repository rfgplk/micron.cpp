//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// arm32_fma_dispatch.cpp
// ARMv7 VFPv3 deliberately implements math::fma as native multiply-accumulate.
// VFPv4 and the non-ARM32 targets retain fused semantics.

#include "../../src/math/bits/impl.hpp"
#include "../../src/math/dispatch.hpp"

#include "../snowball/snowball.hpp"

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::test_case;

namespace ieee = micron::math::ieee;

volatile f64 g_a64 = 0.0;
volatile f64 g_b64 = 0.0;
volatile f64 g_c64 = 0.0;
volatile f32 g_a32 = 0.0f;
volatile f32 g_b32 = 0.0f;
volatile f32 g_c32 = 0.0f;

extern "C" [[gnu::noinline, gnu::used]] f64
micron_fma_dispatch_f64(f64 a, f64 b, f64 c) noexcept
{
  return micron::math::fma<f64>(a, b, c);
}

extern "C" [[gnu::noinline, gnu::used]] f32
micron_fma_dispatch_f32(f32 a, f32 b, f32 c) noexcept
{
  return micron::math::fma<f32>(a, b, c);
}

[[gnu::always_inline]] inline f64
runtime_fma64() noexcept
{
  return micron_fma_dispatch_f64(g_a64, g_b64, g_c64);
}

[[gnu::always_inline]] inline f32
runtime_fma32() noexcept
{
  return micron_fma_dispatch_f32(g_a32, g_b32, g_c32);
}

int
main()
{
  print("=== ARM32 MATH::FMA DISPATCH ===");

  test_case("ordinary finite multiply-accumulate");
  {
    g_a64 = 2.0;
    g_b64 = 3.0;
    g_c64 = 4.0;
    require_true(runtime_fma64() == 10.0);
    g_a64 = -7.5;
    g_b64 = 0.25;
    g_c64 = 1.0;
    require_true(runtime_fma64() == -0.875);

    g_a32 = 2.0f;
    g_b32 = 3.0f;
    g_c32 = 4.0f;
    require_true(runtime_fma32() == 10.0f);
  }
  end_test_case();

  test_case("architecture-selected fused or non-fused rounding");
  {
    g_a64 = ieee::from_bits<f64>(0x3ff0000002000000ULL);      // 1 + 2^-27
    g_b64 = ieee::from_bits<f64>(0x3feffffffc000000ULL);      // 1 - 2^-27
    g_c64 = -1.0;
    const f64 r64 = runtime_fma64();

    g_a32 = ieee::from_bits<f32>(0x3f800400U);      // 1 + 2^-13
    g_b32 = ieee::from_bits<f32>(0x3f7ff800U);      // 1 - 2^-13
    g_c32 = -1.0f;
    const f32 r32 = runtime_fma32();

#if defined(__micron_arch_arm32) && !defined(__micron_arm_fma)
    static_assert(__micron_arch == __micron_arch_arm32);
    static_assert(!micron::math::arch::has_arm_fma);
    require_true(ieee::is_zero(r64));
    require_true(ieee::is_zero(r32));
#else
#if defined(__micron_arch_arm32)
    static_assert(micron::math::arch::has_arm_fma);
#endif
    require_true(r64 == -0x1p-54);
    require_true(r32 == -0x1p-26f);
#endif
  }
  end_test_case();

  test_case("deterministic result bits");
  {
    g_a64 = ieee::from_bits<f64>(0x400921fb54442d18ULL);
    g_b64 = ieee::from_bits<f64>(0xbfd5555555555555ULL);
    g_c64 = ieee::from_bits<f64>(0x3eb0c6f7a0b5ed8dULL);
    const u64 expected = ieee::to_bits(runtime_fma64());
    for ( u32 i = 0; i < 4096; ++i ) require_true(ieee::to_bits(runtime_fma64()) == expected);

    g_a32 = ieee::from_bits<f32>(0x40490fdbU);
    g_b32 = ieee::from_bits<f32>(0xbeaaaaabU);
    g_c32 = ieee::from_bits<f32>(0x358637bdU);
    const u32 expected32 = ieee::to_bits(runtime_fma32());
    for ( u32 i = 0; i < 4096; ++i ) require_true(ieee::to_bits(runtime_fma32()) == expected32);
  }
  end_test_case();

  test_case("NaN and infinity classification");
  {
    g_a64 = ieee::inf_v<f64>();
    g_b64 = 2.0;
    g_c64 = 1.0;
    require_true(ieee::inf_sign(runtime_fma64()) == 1);
    g_a64 = ieee::inf_v<f64>();
    g_b64 = 0.0;
    g_c64 = 1.0;
    require_true(ieee::is_nan(runtime_fma64()));
    g_a64 = ieee::qnan_v<f64>(0x1234);
    g_b64 = 2.0;
    g_c64 = 1.0;
    require_true(ieee::is_nan(runtime_fma64()));

    g_a32 = ieee::inf_v<f32>();
    g_b32 = 2.0f;
    g_c32 = -ieee::inf_v<f32>();
    require_true(ieee::is_nan(runtime_fma32()));
    g_a32 = 1.0f;
    g_b32 = 2.0f;
    g_c32 = 3.0f;
    require_true(ieee::is_finite(runtime_fma32()));
  }
  end_test_case();

  print("[ARM32 MATH::FMA DISPATCH OK]");
  return 1;
}
