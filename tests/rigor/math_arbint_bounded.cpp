// math_arbint_bounded.cpp
// The bounded width: exact mod-2^N semantics, compile-time evaluation, and a structural proof that
// it never reaches the allocator.

#include "../../src/math/arbint.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"
#include "../support/oracles.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using micron::math::arbint;
using micron::math::arbuint;
using mtest::prng;
namespace mpn = micron::math::mpn;

struct poison_allocator {
};

using PU = arbuint<256, micron::math::solver::automatic, poison_allocator>;
using PZ = arbint<512, micron::math::solver::automatic, poison_allocator>;

static_assert(PU::bounded, "a bounded width must not be heap backed");
static_assert(PZ::bounded, "a bounded width must not be heap backed");
static_assert(PU::cap_limbs * mpn::limb_bits == 256, "arbuint<256> must be exactly four 64-bit or eight 32-bit limbs");
static_assert(micron::is_trivially_copyable_v<PU>, "a bounded magnitude should be trivially copyable");

constexpr u64
c_mul()
{
  arbuint<256> a(1234567891011ull);
  a *= arbuint<256>(9876543210ull);
  a /= arbuint<256>(9876543210ull);
  return static_cast<u64>(a);
}

static_assert(c_mul() == 1234567891011ull, "bounded multiply/divide must fold at compile time");

constexpr bool
c_wrap()
{
  arbuint<256> m(0u);
  m -= arbuint<256>(1u);
  if ( m.bit_length() != 256 ) return false;
  if ( m.popcount() != 256 ) return false;
  m += arbuint<256>(1u);
  return m.is_zero();
}

static_assert(c_wrap(), "bounded arithmetic must be exactly mod 2^N at compile time");

constexpr bool
c_narrow()
{

  arbuint<100> w(0u);
  w -= arbuint<100>(1u);
  return w.bit_length() == 100 && w.popcount() == 100;
}

static_assert(c_narrow(), "a partial-limb width must mask its slack");

constexpr i64
c_signed()
{
  arbint<128> a(-1000000);
  a *= arbint<128>(7);
  a += arbint<128>(6);
  return static_cast<i64>(a);
}

static_assert(c_signed() == -6999994, "bounded signed arithmetic must fold at compile time");

constexpr bool
c_shift()
{
  arbuint<256> a = arbuint<256>::power_of_two(255);
  if ( a.bit_length() != 256 ) return false;
  if ( !(a << 1).is_zero() ) return false;
  return (a >> 255) == arbuint<256>(1u);
}

static_assert(c_shift(), "bounded shifts must respect the width at compile time");

int
main()
{
  print("=== ARBINT BOUNDED / CONSTEXPR ===");

  test_case("no allocator is ever named");
  {

    PU a(123456789u);
    PU b(987654321u);
    require_true((a + b) == PU(1111111110u));
    require_true((a * b) / b == a);
    require_true((a * b) % b == PU(0u));
    PU m(0u);
    m -= PU(1u);
    require(m.bit_length(), usize(256));
    require_true((~PU(0u)) == m);

    PZ s(-4242424242ll);
    require(s.sign(), -1);
    require_true(micron::math::abs(s) == PZ(4242424242ll));
    require_true(s / PZ(7) * PZ(7) + s % PZ(7) == s);
  }
  end_test_case();

  test_case("the width really is a ceiling, on every partial-limb size");
  {

    const auto probe = []<usize N>() {
      arbuint<N> m(0u);
      m -= arbuint<N>(1u);
      require(m.bit_length(), N);
      require(m.popcount(), N);
      require_true((m + arbuint<N>(1u)).is_zero());
      require_true(!m.testbit(N));
      require_true(m.testbit(N - 1u));

      const arbuint<N> h = arbuint<N>::power_of_two(N - 1u);
      require_true((h * arbuint<N>(2u)).is_zero());
      require_true((h << 1).is_zero());

      require_true(arbuint<N>::power_of_two(N).is_zero());
    };
    probe.template operator()<1>();
    probe.template operator()<7>();
    probe.template operator()<32>();
    probe.template operator()<33>();
    probe.template operator()<63>();
    probe.template operator()<64>();
    probe.template operator()<65>();
    probe.template operator()<100>();
    probe.template operator()<127>();
    probe.template operator()<128>();
    probe.template operator()<255>();
    probe.template operator()<256>();
    probe.template operator()<521>();
    probe.template operator()<2048>();
  }
  end_test_case();

  test_case("bounded agrees with unbounded wherever the value fits");
  {
    prng rng(0xABCDEF0123456789ull);
    using B = arbuint<2048>;
    using D = arbuint<>;
    for ( usize t = 0; t < 400; ++t ) {
      const usize an = 1u + rng.next_in(15);
      const usize bn = 1u + rng.next_in(15);
      B ba, bb;
      D da, db;
      for ( usize i = 0; i < an; ++i ) {
        const u64 w = rng.next();
        ba <<= 64;
        ba += B(w);
        da <<= 64;
        da += D(w);
      }
      for ( usize i = 0; i < bn; ++i ) {
        const u64 w = rng.next();
        bb <<= 64;
        bb += B(w);
        db <<= 64;
        db += D(w);
      }
      if ( db.is_zero() ) continue;

      const B bp = ba * bb;
      const D dp = da * db;
      require(bp.size(), dp.size());
      for ( usize i = 0; i < bp.size(); ++i ) require_true(bp.limbs()[i] == dp.limbs()[i]);

      const B bq = ba / bb;
      const D dq = da / db;
      require(bq.size(), dq.size());
      for ( usize i = 0; i < bq.size(); ++i ) require_true(bq.limbs()[i] == dq.limbs()[i]);
    }
  }
  end_test_case();

  test_case("a pinned solver is a compile-time property, not a runtime one");
  {
    using Cb = arbuint<256, micron::math::solver::comba>;
    using Bc = arbuint<256, micron::math::solver::basecase>;
    static_assert(micron::is_same_v<Cb::solver_type, micron::math::solver::comba>);
    static_assert(micron::is_same_v<Bc::solver_type, micron::math::solver::basecase>);

    prng rng(0xDEADBEEFCAFEBABEull);
    for ( usize t = 0; t < 400; ++t ) {
      Cb x, y;
      Bc p, q;
      for ( usize i = 0; i < 4; ++i ) {
        const u64 w = rng.next();
        x <<= 64;
        x += Cb(w);
        p <<= 64;
        p += Bc(w);
      }
      for ( usize i = 0; i < 4; ++i ) {
        const u64 w = rng.next();
        y <<= 64;
        y += Cb(w);
        q <<= 64;
        q += Bc(w);
      }
      const Cb r1 = x * y;
      const Bc r2 = p * q;
      require(r1.size(), r2.size());
      for ( usize i = 0; i < r1.size(); ++i ) require_true(r1.limbs()[i] == r2.limbs()[i]);
    }
  }
  end_test_case();

  print("=== ARBINT BOUNDED / CONSTEXPR PASSED ===");
  return 1;
}
