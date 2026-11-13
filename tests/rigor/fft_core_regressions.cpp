//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/fft.hpp"
#include "../snowball/snowball.hpp"

namespace ft = micron::math::fft;
using micron::math::complex;

#include "../support/fft/oracle.hpp"

template<typename F>
constexpr bool
constexpr_division()
{
  constexpr auto q = complex<F>{ F(3), F(4) } / complex<F>{ F(1), F(-2) };
  return q == complex<F>{ F(-1), F(2) };
}

static_assert(constexpr_division<float>());
static_assert(constexpr_division<double>());
static_assert(constexpr_division<long double>());
static_assert((complex<double>{ micron::numeric_limits<double>::max(), micron::numeric_limits<double>::max() }
               / complex<double>{ micron::numeric_limits<double>::max(), micron::numeric_limits<double>::max() })
              == complex<double>{ 1 });
#if defined(__micron_arch_amd64)
static_assert(constexpr_division<__float128>());
constexpr auto __quad_largest = micron::math::bits::bit_cast<__float128>((u128(0x7ffeffffffffffffULL) << 64) | 0xffffffffffffffffULL);
static_assert((complex<__float128>{ __quad_largest, __quad_largest } / complex<__float128>{ __quad_largest, __quad_largest })
              == complex<__float128>{ 1 });
#if defined(__micron_compiler_gcc) && defined(__micron_lang_cpp23)
static_assert(constexpr_division<_Float16>());
static_assert((complex<_Float16>{ 65504, 65504 } / complex<_Float16>{ 65504, 65504 }) == complex<_Float16>{ 1 });
static_assert(constexpr_division<_Float32>());
static_assert(constexpr_division<_Float64>());
static_assert(constexpr_division<_Float128>());
#endif
#endif

static u64 division_seed = 0xc27f416ab8e3509dULL;

static u32
division_random()
{
  division_seed ^= division_seed >> 12;
  division_seed ^= division_seed << 25;
  division_seed ^= division_seed >> 27;
  return u32((division_seed * 0x2545f4914f6cdd1dULL) >> 32);
}

static long double
division_sample(int low, int high)
{
  int exponent = low + int(division_random() % unsigned(high - low + 1));
  long double value = static_cast<long double>(256 + division_random() % 256) / 256;
  while ( exponent > 0 ) {
    value *= 2;
    --exponent;
  }
  while ( exponent < 0 ) {
    value /= 2;
    ++exponent;
  }
  return division_random() & 1 ? value : -value;
}

template<typename F>
static void
division_oracle()
{
  for ( unsigned pass = 0; pass < 3; ++pass ) {
    int low = -20, high = 20;
    if ( pass == 1 ) {
      if constexpr ( micron::is_same_v<F, float> ) {
#if defined(__micron_arch_arm32)
        continue;
#endif
        low = micron::numeric_limits<float>::min_exponent - micron::numeric_limits<float>::digits;
        high = micron::numeric_limits<float>::max_exponent - 1;
      } else if constexpr ( micron::is_same_v<F, double> && __micron_ldbl_max_exp > micron::numeric_limits<double>::max_exponent ) {
        low = micron::numeric_limits<double>::min_exponent - micron::numeric_limits<double>::digits;
        high = micron::numeric_limits<double>::max_exponent - 1;
      } else
        continue;
    } else if ( pass == 2 ) {
      low = micron::is_same_v<F, float> ? -16 : -128;
      high = micron::is_same_v<F, float> ? 15 : 127;
    }
    for ( unsigned trial = 0; trial < 4000; ++trial ) {
      volatile F a = F(division_sample(low, high)), b = F(division_sample(low, high));
      volatile F c = F(division_sample(low, high)), d = F(division_sample(low, high));
      long double ar = a, ai = b, cr = c, ci = d;
      long double divisor = cr * cr + ci * ci;
      long double re = (ar * cr + ai * ci) / divisor, im = (ai * cr - ar * ci) / divisor;
      const long double largest = [] {
        if constexpr ( micron::is_same_v<F, float> ) return static_cast<long double>(micron::numeric_limits<float>::max());
        if constexpr ( micron::is_same_v<F, double> ) return static_cast<long double>(micron::numeric_limits<double>::max());
        return micron::numeric_limits<long double>::max();
      }();
      if ( absolute(re) > largest || absolute(im) > largest ) continue;
      auto q = complex<F>{ a, b } / complex<F>{ c, d };
      volatile F actual_re = q.real, actual_im = q.imag;
      long double floor = 0;
#if !defined(__micron_arch_arm32)
      if constexpr ( micron::is_same_v<F, float> ) floor = micron::numeric_limits<float>::denorm_min() * 4.0L;
#endif
      if constexpr ( micron::is_same_v<F, double> ) floor = micron::numeric_limits<double>::denorm_min() * 4.0L;
      long double re_tolerance = (absolute(ar * cr) + absolute(ai * ci)) / divisor * epsilon<F>() * 32 + floor;
      long double im_tolerance = (absolute(ai * cr) + absolute(ar * ci)) / divisor * epsilon<F>() * 32 + floor;
      if ( !(absolute(static_cast<long double>(actual_re) - re) <= re_tolerance)
           || !(absolute(static_cast<long double>(actual_im) - im) <= im_tolerance) )
        micron::io::println("division oracle size ", sizeof(F), " pass ", pass, " trial ", trial, " a ", double(ar), " b ", double(ai),
                            " c ", double(cr), " d ", double(ci), " re ", double(re), " im ", double(im), " actual re ", double(actual_re),
                            " actual im ", double(actual_im));
      check(actual_re, re, re_tolerance);
      check(actual_im, im, im_tolerance);
    }
  }
}

template<typename F>
static bool
quotient_component(F actual, F expected, const char *label)
{
  volatile F rounded = actual;
  long double relative = absolute((static_cast<long double>(rounded) - expected) / (expected ? expected : F(1)));
  if ( !(relative <= epsilon<F>() * 16) ) {
    micron::io::println(label, ": actual ", rounded, " expected ", expected);
    return false;
  }
  return true;
}

template<typename F>
static bool
division_range()
{
  constexpr F largest = [] {
    if constexpr ( micron::is_same_v<F, float> ) return F(micron::numeric_limits<float>::max());
    if constexpr ( micron::is_same_v<F, double> ) return F(micron::numeric_limits<double>::max());
    return F(micron::numeric_limits<long double>::max());
  }();
  bool good = true;
  auto q = complex<F>{ largest, largest } / complex<F>{ largest, largest };
  good &= quotient_component(q.real, F(1), "finite large denominator real");
  good &= quotient_component(q.imag, F(0), "finite large denominator imaginary");
  q = complex<F>{ largest, largest } / complex<F>{ F(1), F(1) };
  good &= quotient_component(q.real / largest, F(1), "finite large numerator real");
  good &= quotient_component(q.imag, F(0), "finite large numerator imaginary");
  q = complex<F>{ largest, -largest } / complex<F>{ largest, -largest };
  good &= quotient_component(q.real, F(1), "finite large conjugate denominator real");
  good &= quotient_component(q.imag, F(0), "finite large conjugate denominator imaginary");
  q = complex<F>{ largest, largest } / complex<F>{ largest / F(2), largest };
  good &= quotient_component(q.real, F(1.2L), "finite large imaginary denominator real");
  good &= quotient_component(q.imag, F(-0.4L), "finite large imaginary denominator imaginary");

  // ARMv7 NEON and AArch64's freestanding binary128 runtime flush subnormals.
  if constexpr (
#if defined(__micron_arch_arm32)
      !micron::is_same_v<F, float>
#elif defined(__micron_arch_arm64)
      sizeof(F) <= sizeof(double)
#else
      true
#endif
  ) {
    constexpr F smallest = [] {
      if constexpr ( micron::is_same_v<F, float> ) return F(micron::numeric_limits<float>::min());
      if constexpr ( micron::is_same_v<F, double> ) return F(micron::numeric_limits<double>::min());
      return F(micron::numeric_limits<long double>::min());
    }();
    constexpr F tiny = [] {
      if constexpr ( micron::is_same_v<F, float> ) return F(micron::numeric_limits<float>::denorm_min());
      if constexpr ( micron::is_same_v<F, double> ) return F(micron::numeric_limits<double>::denorm_min());
      return F(__micron_ldbl_denorm_min);
    }();
    q = complex<F>{ F(0), tiny } / complex<F>{ smallest, smallest / F(2) };
    good &= quotient_component(q.real, (tiny / smallest) * F(0.4L), "finite small numerator real");
    good &= quotient_component(q.imag, (tiny / smallest) * F(0.8L), "finite small numerator imaginary");
    q = complex<F>{ F(0), largest } / complex<F>{ F(2), tiny };
    good &= quotient_component(q.real, (largest * tiny) / F(4), "finite subnormal ratio real");
    good &= quotient_component(q.imag / (largest / F(2)), F(1), "finite subnormal ratio imaginary");
    q = complex<F>{ tiny, F(0) } / complex<F>{ smallest / F(2), smallest };
    good &= quotient_component(q.real, (tiny / smallest) * F(0.4L), "finite small imaginary denominator real");
    good &= quotient_component(q.imag, (tiny / smallest) * F(-0.8L), "finite small imaginary denominator imaginary");
    q = complex<F>{ largest, F(0) } / complex<F>{ tiny, F(2) };
    good &= quotient_component(q.real, (largest * tiny) / F(4), "finite subnormal inverse ratio real");
    good &= quotient_component(q.imag / (largest / F(2)), F(-1), "finite subnormal inverse ratio imaginary");
  }
  return good;
}

int
main()
{
  sb::test_case("Complex division keeps representable finite results across the scalar range");
  bool good = division_range<float>();
  good &= division_range<double>();
  good &= division_range<long double>();
  sb::require_true(good);
  sb::end_test_case();
  sb::test_case("Seeded complex division against an independent sum-of-products oracle");
  division_oracle<float>();
  division_oracle<double>();
  division_oracle<long double>();
  sb::end_test_case();
  return 1;
}
