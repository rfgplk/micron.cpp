//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// math_defects_hw_fp128_b.cpp
//
// (a) hw::__constexpr_sqrt was an unscaled heron iteration under a fixed cap, so it only converged
//     within about 2^-74 < x < 2^74 and answered x * 2^-40 outside it -- 1e300 came back as
//     9.09e287 and 1e-300 as 9.09e-13, 94.8% of random f64 normals were inexact and the worst was
//     2.1e18 ulp. It is the consteval body of sqrt_ss/sqrt_sd on every compiler that rejects
//     __builtin_sqrt in a constant expression, which is all of clang, and of rsqrt_approx_ss's
//     consteval branch everywhere. The golden values below are correctly-rounded roots, agreed by
//     mpmath at 400 bits and by the hardware instruction; they are read through `constexpr`
//     variables so a wrong root is a runtime require() failure and not a build error.
//
// (b) __gcc_fp128_syms.hpp gated its whole binary128 runtime on __FLT128_MANT_DIG__, which NO clang
//     defines on ANY target -- so every clang build got none of the 24 symbols. On aarch64, where
//     `long double` IS binary128 and the compiler emits the TF-mode calls whatever the spelling,
//     that left tests/build/longdouble_freestanding_link.cpp with eight undefined binary128 symbols.
//     The check here is the portable half of that: the shim's block must be LIVE wherever the target
//     has both a 128-bit integer and a 128-bit float. The link half is those manifests' own cells.
//
// NOT a defect, and pinned here so it is not "fixed" back: on armv7-a without VFPv4 hw::fmadd_* is
// a chained vmla, so its runtime value differs from its own consteval value. That is deliberate and
// tests/rigor/arm32_fma_dispatch.cpp asserts it directly. What this file does assert is that the
// f32 and f64 ladders make the SAME choice -- they did not, because the f64 arm carried no
// capability gate at all, which also made it the only arm reachable on a target with no FP
// registers, where it could not be assembled.

#include "../../src/math/__asm/hw.hpp"
#include "../../src/math/__gcc_fp128_syms.hpp"

#include "../snowball/snowball.hpp"

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::skip;
using ::sb::test_case;

namespace hw = micron::math::hw;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (a) the software square root

// evaluated by the constant evaluator, which is the only way in on a compiler without a constexpr
// __builtin_sqrt; a volatile round trip would silently take the hardware arm instead
constexpr f64 k_sw_2 = hw::__constexpr_sqrt<f64>(0x1p+1);
constexpr f64 k_sw_3 = hw::__constexpr_sqrt<f64>(0x1.8p+1);
constexpr f64 k_sw_half = hw::__constexpr_sqrt<f64>(0x1p-1);
constexpr f64 k_sw_ten = hw::__constexpr_sqrt<f64>(0x1.4p+3);
constexpr f64 k_sw_big = hw::__constexpr_sqrt<f64>(0x1.7e43c8800759cp+996);
constexpr f64 k_sw_small = hw::__constexpr_sqrt<f64>(0x1.56e1fc2f8f359p-997);
constexpr f64 k_sw_e99 = hw::__constexpr_sqrt<f64>(0x1.93e5939a08ceap+99);
constexpr f64 k_sw_em100 = hw::__constexpr_sqrt<f64>(0x1.4484bfeebc2a0p-100);
constexpr f64 k_sw_e332 = hw::__constexpr_sqrt<f64>(0x1.249ad2594c37dp+332);
constexpr f64 k_sw_em333 = hw::__constexpr_sqrt<f64>(0x1.bff2ee48e0530p-333);
constexpr f64 k_sw_max = hw::__constexpr_sqrt<f64>(0x1.fffffffffffffp+1023);
constexpr f64 k_sw_den = hw::__constexpr_sqrt<f64>(0x0.0000000000001p-1022);
constexpr f64 k_sw_minnorm = hw::__constexpr_sqrt<f64>(0x1p-1022);
constexpr f64 k_sw_near4 = hw::__constexpr_sqrt<f64>(0x1.ffffffffffffep+1);

constexpr f32 k_swf_2 = hw::__constexpr_sqrt<f32>(0x1p+1f);
constexpr f32 k_swf_em30 = hw::__constexpr_sqrt<f32>(0x1.4484cp-100f);
constexpr f32 k_swf_e30 = hw::__constexpr_sqrt<f32>(0x1.93e594p+99f);
constexpr f32 k_swf_max = hw::__constexpr_sqrt<f32>(0x1.fffffep+127f);
constexpr f32 k_swf_den = hw::__constexpr_sqrt<f32>(0x1p-149f);
constexpr f32 k_swf_minnorm = hw::__constexpr_sqrt<f32>(0x1p-126f);
constexpr f32 k_swf_em40 = hw::__constexpr_sqrt<f32>(0x1.16c2p-133f);

static bool
software_root_is_correctly_rounded(void) noexcept
{
  return k_sw_2 == 0x1.6a09e667f3bcdp+0 && k_sw_3 == 0x1.bb67ae8584caap+0 && k_sw_half == 0x1.6a09e667f3bcdp-1
         && k_sw_ten == 0x1.94c583ada5b53p+1 && k_sw_big == 0x1.38d352e5096afp+498 && k_sw_small == 0x1.a2fe76a3f9475p-499
         && k_sw_e99 == 0x1.c6bf52634p+49 && k_sw_em100 == 0x1.203af9ee75616p-50 && k_sw_e332 == 0x1.11b0ec57e649ap+166
         && k_sw_em333 == 0x1.dee7a4ad4b81fp-167 && k_sw_max == 0x1.fffffffffffffp+511 && k_sw_den == 0x1p-537 && k_sw_minnorm == 0x1p-511
         && k_sw_near4 == 0x1.fffffffffffffp+0 && k_swf_2 == 0x1.6a09e6p+0f && k_swf_em30 == 0x1.203afap-50f && k_swf_e30 == 0x1.c6bf52p+49f
         && k_swf_max == 0x1.fffffep+63f && k_swf_den == 0x1.6a09e6p-75f && k_swf_minnorm == 0x1p-63f && k_swf_em40 == 0x1.79c9cep-67f;
}

// the same roots through the public entry points, still at compile time: these are what a caller
// actually writes, and on a compiler with a constexpr __builtin_sqrt they take the other arm
constexpr f64 k_ce_2 = hw::sqrt_sd(0x1p+1);
constexpr f64 k_ce_big = hw::sqrt_sd(0x1.7e43c8800759cp+996);
constexpr f64 k_ce_small = hw::sqrt_sd(0x1.56e1fc2f8f359p-997);
constexpr f64 k_ce_max = hw::sqrt_sd(0x1.fffffffffffffp+1023);
constexpr f32 k_cef_2 = hw::sqrt_ss(0x1p+1f);
constexpr f32 k_cef_em30 = hw::sqrt_ss(0x1.4484cp-100f);

// volatile in and out: the first defeats the `if consteval`, the second forces the result down to
// its own format where __FLT_EVAL_METHOD__ keeps it wider
volatile f32 g_f32 = 0.0f;
volatile f64 g_f64 = 0.0;

[[gnu::noinline]] static f64
rt_sqrt_sd(f64 x) noexcept
{
  g_f64 = x;
  g_f64 = hw::sqrt_sd(g_f64);
  return g_f64;
}

[[gnu::noinline]] static f32
rt_sqrt_ss(f32 x) noexcept
{
  g_f32 = x;
  g_f32 = hw::sqrt_ss(g_f32);
  return g_f32;
}

[[gnu::noinline]] static f64
rt_sw_sqrt(f64 x) noexcept
{
  g_f64 = x;
  g_f64 = hw::__constexpr_sqrt<f64>(g_f64);
  return g_f64;
}

[[gnu::noinline]] static f32
rt_sw_sqrtf(f32 x) noexcept
{
  g_f32 = x;
  g_f32 = hw::__constexpr_sqrt<f32>(g_f32);
  return g_f32;
}

// -ffast-math folds `v != v` to false, so a NaN has to be read out of the bits
static bool
is_nan64(f64 v) noexcept
{
  const u64 b = __builtin_bit_cast(u64, v);
  return (b & 0x7FF0000000000000ull) == 0x7FF0000000000000ull && (b & 0x000FFFFFFFFFFFFFull) != 0;
}

static bool
is_nan32(f32 v) noexcept
{
  const u32 b = __builtin_bit_cast(u32, v);
  return (b & 0x7F800000u) == 0x7F800000u && (b & 0x007FFFFFu) != 0;
}

// xorshift over a fixed seed; the hardware root is the oracle, so the sweep needs no table
static u64 g_seed = 0x243F6A8885A308D3ull;

static u64
next_rand(void) noexcept
{
  g_seed ^= g_seed << 13;
  g_seed ^= g_seed >> 7;
  g_seed ^= g_seed << 17;
  return g_seed;
}

static bool
software_matches_hardware(void) noexcept
{
  g_seed = 0x243F6A8885A308D3ull;
  for ( u32 i = 0; i < 40000u; ++i ) {
    const u64 e = 1ull + (next_rand() % 2046ull);
    const u64 m = next_rand() & 0x000FFFFFFFFFFFFFull;
    const f64 x = __builtin_bit_cast(f64, (e << 52) | m);
    if ( rt_sw_sqrt(x) != rt_sqrt_sd(x) ) return false;
  }
  for ( u32 i = 0; i < 40000u; ++i ) {
    const u32 e = 1u + static_cast<u32>(next_rand() % 254ull);
    const u32 m = static_cast<u32>(next_rand()) & 0x007FFFFFu;
    const f32 x = __builtin_bit_cast(f32, (e << 23) | m);
    if ( rt_sw_sqrtf(x) != rt_sqrt_ss(x) ) return false;
  }
  for ( u32 i = 0; i < 8000u; ++i ) {      // f64 subnormal inputs: the root is always normal
    u64 m = next_rand() & 0x000FFFFFFFFFFFFFull;
    if ( m == 0 ) m = 1;
    const f64 x = __builtin_bit_cast(f64, m);
    if ( rt_sw_sqrt(x) != rt_sqrt_sd(x) ) return false;
  }
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (a2) fmadd: whichever way the target rounds, both widths must round the same way

volatile f32 g_a32 = 0.0f, g_b32 = 0.0f, g_c32 = 0.0f;
volatile f64 g_a64 = 0.0, g_b64 = 0.0, g_c64 = 0.0;

[[gnu::noinline]] static bool
fmadd_ss_is_fused(void) noexcept
{
  g_a32 = 1.0f + 0x1p-13f;
  g_b32 = 1.0f - 0x1p-13f;
  g_c32 = -1.0f;
  return hw::fmadd_ss(g_a32, g_b32, g_c32) != 0.0f;
}

[[gnu::noinline]] static bool
fmadd_sd_is_fused(void) noexcept
{
  g_a64 = 1.0 + 0x1p-27;
  g_b64 = 1.0 - 0x1p-27;
  g_c64 = -1.0;
  return hw::fmadd_sd(g_a64, g_b64, g_c64) != 0.0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (b) is the binary128 runtime compiled in?

// __tf_bits lives inside the shim's own #if and nowhere else, and the call is dependent on T, so an
// absent declaration leaves the constraint unsatisfied instead of raising a hard error
template<typename T>
concept __mc_tf_shim_live = requires(T v) { __tf_bits(v); };

#if defined(__FLT128_MANT_DIG__)
using __mc_tf_probe = _Float128;
#elif defined(__SIZEOF_FLOAT128__)
using __mc_tf_probe = __float128;
#elif defined(__LDBL_MANT_DIG__) && __LDBL_MANT_DIG__ == 113
using __mc_tf_probe = long double;
#else
using __mc_tf_probe = f64;
#endif

#if defined(__SIZEOF_INT128__)                                                                                                             \
    && (defined(__FLT128_MANT_DIG__) || defined(__SIZEOF_FLOAT128__) || (defined(__LDBL_MANT_DIG__) && __LDBL_MANT_DIG__ == 113))
#define __mc_target_has_binary128 1
#endif

// the residual correction is a veltkamp split: exact only where every operation rounds to its own
// format and nothing is reassociated. The constant evaluator always does both, and so does every
// IEEE run-time target -- but x87 excess precision (__FLT_EVAL_METHOD__ 2, i386) leaves the f64
// split 1 ulp short on 99.9% of inputs, and -ffast-math on 16.5%. Neither is reachable from micron:
// the software root is a consteval body everywhere except armv7 with no FPU, where
// __FLT_EVAL_METHOD__ is 0. So the run-time sweep claims the bit-exact result only where it is owed
#if defined(__FLT_EVAL_METHOD__) && __FLT_EVAL_METHOD__ == 0 && !defined(__FAST_MATH__)
#define __mc_exact_fp_at_runtime 1
#endif

int
main()
{
  print("=== HW SOFTWARE ROOT + BINARY128 GATE ===");

  test_case("__constexpr_sqrt is the correctly-rounded root across the whole exponent range");
  {
    require_true(software_root_is_correctly_rounded());
  }
  end_test_case();

  test_case("sqrt_ss / sqrt_sd agree with it in a constant expression");
  {
    require_true(k_ce_2 == 0x1.6a09e667f3bcdp+0);
    require_true(k_ce_big == 0x1.38d352e5096afp+498);
    require_true(k_ce_small == 0x1.a2fe76a3f9475p-499);
    require_true(k_ce_max == 0x1.fffffffffffffp+511);
    require_true(k_cef_2 == 0x1.6a09e6p+0f);
    require_true(k_cef_em30 == 0x1.203afap-50f);
  }
  end_test_case();

  test_case("negative control: the comparisons above can fail");
  {
    require_true(k_sw_2 != 0x1.6a09e667f3bccp+0);          // the old body's answer, one ulp low
    require_true(k_sw_big != 0x1.7e43c8800759cp+956);      // the old body's answer, x * 2^-40
    require_true(k_sw_den != 0x1p-40);
    require_true(k_ce_2 != 3.0);
  }
  end_test_case();

  test_case("the software root reproduces this target's hardware root, bit for bit");
  {
#if defined(__mc_exact_fp_at_runtime)
    require_true(software_matches_hardware());
#else
    skip("x87 excess precision or -ffast-math here: the residual correction is exact in neither");
#endif
  }
  end_test_case();

  // reaching the end of this case at all is most of what it asserts: an unbounded range reduction
  // spins forever on +inf once -ffinite-math-only has deleted the guard in front of it
  test_case("the range reduction terminates on every non-finite and non-positive input");
  {
    const f64 inf = __builtin_huge_val();
    const f64 nan = __builtin_nan("");
    (void)rt_sw_sqrt(inf);
    (void)rt_sw_sqrt(nan);
    (void)rt_sw_sqrt(-inf);
    (void)rt_sw_sqrtf(__builtin_huge_valf());
    require_true(rt_sw_sqrt(1.0) == 1.0);
#if !defined(__FAST_MATH__)
    require_true(rt_sw_sqrt(0.0) == 0.0);
    require_true(__builtin_bit_cast(u64, rt_sw_sqrt(-0.0)) == 0x8000000000000000ull);
    require_true(rt_sw_sqrt(inf) == inf);
    require_true(is_nan64(rt_sw_sqrt(nan)));
    require_true(is_nan64(rt_sw_sqrt(-4.0)));
    require_true(is_nan64(rt_sw_sqrt(-inf)));
    require_true(is_nan32(rt_sw_sqrtf(-1.0f)));
#else
    skip("-ffast-math: neither a NaN nor an infinity survives the optimiser, so only the reduction "
         "reaching this line is claimed here");
#endif
  }
  end_test_case();

  test_case("fmadd_ss and fmadd_sd make the same fused-or-chained choice");
  {
    require_true(fmadd_ss_is_fused() == fmadd_sd_is_fused());
  }
  end_test_case();

  test_case("arm32 without VFPv4 rounds fmadd twice, by design");
  {
#if defined(__micron_arch_arm32) && !defined(__micron_arm_fma)
    require_true(!fmadd_sd_is_fused());      // vmla; tests/rigor/arm32_fma_dispatch.cpp owns this
#else
    skip("not an armv7 target without VFPv4, so the documented chained rounding is out of reach");
#endif
  }
  end_test_case();

  test_case("the binary128 runtime is compiled wherever the target has one");
  {
#if defined(__mc_target_has_binary128)
    require_true(__mc_tf_shim_live<__mc_tf_probe>);
#else
    require_true(!__mc_tf_shim_live<__mc_tf_probe>);
    skip("no 128-bit integer or no binary128 on this target; the shim is correctly absent");
#endif
  }
  end_test_case();

  print("=== DONE ===");
  return 1;
}
