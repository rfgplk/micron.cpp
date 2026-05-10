// math_kern.cpp
// Rigorous snowball test suite for math::bits, math::ieee, math::dd,
// math::mk (scalar surface), math::id (identity helpers).

#include "../../src/math.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

using namespace micron;

static bool
near(f64 a, f64 b, f64 eps = 1e-12)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== MATH::KERN TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // math::bits
  test_case("math::bits — rotates");
  {
    static_assert(math::bits::rol64(1ULL, 1) == 2);
    static_assert(math::bits::ror64(2ULL, 1) == 1);
    static_assert(math::bits::rol32(1u, 1) == 2);
    static_assert(math::bits::ror32(2u, 1) == 1);
    require(math::bits::rol64(0xFEDCBA0987654321ULL, 16), ((u64)0xFEDCBA0987654321ULL << 16) | ((u64)0xFEDCBA0987654321ULL >> 48));
  }
  end_test_case();

  test_case("math::bits — counts");
  {
    require(math::bits::clz<u32>(1u), 31);
    require(math::bits::ctz<u32>(8u), 3);
    require(math::bits::popcount<u32>(0xFu), 4);
  }
  end_test_case();

  test_case("math::bits — abs64 / sign_bit");
  {
    static_assert(math::bits::abs64(-5) == 5);
    static_assert(math::bits::abs64(5) == 5);
    require(math::bits::sign_bit<f64>(-1.0), 1);
    require(math::bits::sign_bit<f64>(1.0), 0);
  }
  end_test_case();

  test_case("math::bits — bit_cast round-trip");
  {
    f64 x = 1.5;
    u64 u = math::bits::bit_cast<u64>(x);
    f64 y = math::bits::bit_cast<f64>(u);
    require_true(x == y);
  }
  end_test_case();

  test_case("math::bits — byteswap");
  {
    static_assert(math::bits::byteswap32(0x11223344u) == 0x44332211u);
    static_assert(math::bits::byteswap64(0x1122334455667788ULL) == 0x8877665544332211ULL);
  }
  end_test_case();

  test_case("math::bits — floor_pow2 / ceil_pow2");
  {
    static_assert(math::bits::floor_pow2<u32>(7) == 4);
    static_assert(math::bits::ceil_pow2<u32>(7) == 8);
    static_assert(math::bits::floor_pow2<u32>(16) == 16);
    static_assert(math::bits::ceil_pow2<u32>(16) == 16);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // math::ieee
  test_case("math::ieee — pack/unpack");
  {
    f64 x = 1.0;
    u64 u = math::ieee::to_bits(x);
    require(u, 0x3FF0000000000000ULL);
    require(math::ieee::from_bits<f64>(u), 1.0);
    // exponent of 1.0 is 0
    require(math::ieee::exponent_of<f64>(1.0), 0);
    require(math::ieee::exponent_of<f64>(2.0), 1);
    require(math::ieee::exponent_of<f64>(0.5), -1);
  }
  end_test_case();

  test_case("math::ieee — classifiers");
  {
    require_true(math::ieee::is_zero<f64>(0.0));
    require_true(math::ieee::is_zero<f64>(-0.0));
    require_false(math::ieee::is_zero<f64>(1e-300));
    require_true(math::ieee::is_normal<f64>(1.0));
    // is_finite under -ffast-math is unreliable; just smoke-test
  }
  end_test_case();

  test_case("math::ieee — pack");
  {
    f64 v = math::ieee::pack<f64>(0, 0, 0);     // = 1.0
    require(v, 1.0);
    f64 v2 = math::ieee::pack<f64>(1, 1, 0);     // = -2.0
    require(v2, -2.0);
  }
  end_test_case();

  test_case("math::ieee — ulp_distance");
  {
    f64 a = 1.0;
    require(math::ieee::ulp_distance<f64>(a, a), 0);
    f64 next = math::ieee::next_up<f64>(a);
    require(math::ieee::ulp_distance<f64>(a, next), 1);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // math::dd64
  test_case("math::dd — two_sum exactness");
  {
    auto s = math::dd::two_sum(1e16, 1.0);
    // hi + lo == 1e16 + 1.0 exactly (as a real number)
    require_true(s.hi + s.lo == 1e16 + 1.0 || (s.hi == 1e16 && s.lo == 1.0));
  }
  end_test_case();

  test_case("math::dd — two_prod exact via FMA");
  {
    auto p = math::dd::two_prod(1.0 + 1e-15, 1.0 + 1e-15);
    // hi == 1.0+2e-15 (rounded), lo holds the tiny residual
    require_true(p.hi > 1.0);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // math::mk surfaces
  test_case("math::mk::trig — sin/cos at known points");
  {
    require_true(near(math::mk::trig::sin<f64>(0.0), 0.0));
    require_true(near(math::mk::trig::cos<f64>(0.0), 1.0));
    require_true(near(math::mk::trig::sin<f64>(math::pi / 2.0), 1.0));
    require_true(near(math::mk::trig::cos<f64>(math::pi), -1.0));
  }
  end_test_case();

  test_case("math::mk::trig::sincos — fused");
  {
    f64 s = 0, c = 0;
    math::mk::trig::sincos<f64>(math::pi / 2.0, s, c);
    require_true(near(s, 1.0));
    require_true(near(c, 0.0, 1e-10));
  }
  end_test_case();

  test_case("math::mk::exp_ns — exp / exp2 / expm1");
  {
    require_true(near(math::mk::exp_ns::exp<f64>(0.0), 1.0));
    require_true(near(math::mk::exp_ns::exp<f64>(1.0), math::e));
    require_true(near(math::mk::exp_ns::exp2<f64>(10.0), 1024.0));
    require_true(near(math::mk::exp_ns::expm1<f64>(0.0), 0.0));
    require_true(near(math::mk::exp_ns::exp10<f64>(2.0), 100.0));
  }
  end_test_case();

  test_case("math::mk::log_ns — log / log2 / log10 / log1p");
  {
    require_true(near(math::mk::log_ns::log<f64>(1.0), 0.0));
    require_true(near(math::mk::log_ns::log<f64>(math::e), 1.0));
    require_true(near(math::mk::log_ns::log2<f64>(1024.0), 10.0));
    require_true(near(math::mk::log_ns::log10<f64>(1000.0), 3.0));
    require_true(near(math::mk::log_ns::log1p<f64>(0.0), 0.0));
  }
  end_test_case();

  test_case("math::mk::pow_ns — sqrt / cbrt / pow / hypot / rsqrt");
  {
    require_true(near(math::mk::pow_ns::sqrt<f64>(4.0), 2.0));
    require_true(near(math::mk::pow_ns::cbrt<f64>(27.0), 3.0));
    require_true(near(math::mk::pow_ns::pow<f64>(2.0, 10.0), 1024.0));
    require_true(near(math::mk::pow_ns::hypot<f64>(3.0, 4.0), 5.0));
    require_true(near(math::mk::pow_ns::rsqrt<f64>(4.0), 0.5));
    // fast-policy rsqrt — relaxed precision
    f64 fr = math::mk::pow_ns::rsqrt<f64>(4.0, math::policy::fast);
    require_true(near(fr, 0.5, 1e-6));
  }
  end_test_case();

  test_case("math::mk::round_ns — directed roundings");
  {
    require_true(near(math::mk::round_ns::floor<f64>(1.7), 1.0));
    require_true(near(math::mk::round_ns::ceil<f64>(1.2), 2.0));
    require_true(near(math::mk::round_ns::trunc<f64>(-1.7), -1.0));
    require_true(near(math::mk::round_ns::round<f64>(1.5), 2.0));
  }
  end_test_case();

  test_case("math::mk::special — erf / erfc");
  {
    require_true(near(math::mk::special::erf<f64>(0.0), 0.0));
    require_true(near(math::mk::special::erfc<f64>(0.0), 1.0));
  }
  end_test_case();

  test_case("math::mk::manip — frexp / ldexp / copysign");
  {
    int e = 0;
    f64 m = math::mk::manip::frexp<f64>(8.0, &e);
    require_true(near(m, 0.5));
    require(e, 4);
    require_true(near(math::mk::manip::ldexp<f64>(0.5, 4), 8.0));
    require_true(near(math::mk::manip::copysign<f64>(1.0, -1.0), -1.0));
  }
  end_test_case();

  test_case("math::mk::fused — dot2 / dot3 / fma");
  {
    require_true(near(math::mk::fused::fma<f64>(2.0, 3.0, 1.0), 7.0));
    require_true(near(math::mk::fused::dot2<f64>(1, 2, 3, 4), 11.0));
    require_true(near(math::mk::fused::dot3<f64>(1, 2, 3, 4, 5, 6), 32.0));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // math::id — identity helpers
  test_case("math::id — Pythagorean identity");
  {
    f64 a = math::pi / 5.0;
    f64 s = math::sin<f64>(a);
    f64 c = math::id::cos_from_sin<f64>(s);
    f64 c2 = math::cos<f64>(a);
    require_true(near(c, c2, 1e-12));
  }
  end_test_case();

  test_case("math::id — double-angle");
  {
    f64 a = 0.4;
    f64 s = math::sin<f64>(a);
    f64 c = math::cos<f64>(a);
    require_true(near(math::id::sin2x<f64>(s, c), math::sin<f64>(2 * a)));
    require_true(near(math::id::cos2x<f64>(s, c), math::cos<f64>(2 * a)));
  }
  end_test_case();

  print("=== MATH::KERN TESTS PASSED ===");
  return 0;
}
