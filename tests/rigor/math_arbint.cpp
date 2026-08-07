// math_arbint.cpp
// Snowball rigor for micron::math::arbuint / arbint — every operation diffed against the
// deliberately-stupid u32 oracle in tests/support/arbint_oracle.hpp.
//
// Seeds are fixed hex literals: a failure here reproduces on the next run, on every arch.

#include "../../src/math/arbint.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"
#include "../support/arbint_oracle.hpp"
#include "../support/oracles.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using micron::math::arbint;
using micron::math::arbuint;
using mtest::prng;
namespace ora = mtest::arbint_oracle;

// qemu runs these at a fraction of native speed and the oracle is quadratic-with-a-bit-loop, so the
// cross rows get a smaller sweep rather than a longer timeout
#if defined(ARBINT_RIGOR_LITE) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_TRIALS = 120;
constexpr static const usize MAX_LIMBS = 16;
#else
constexpr static const usize N_TRIALS = 900;
constexpr static const usize MAX_LIMBS = 40;
#endif

using U = arbuint<>;
using Z = arbint<>;

static void
build(U &a, ora::obn &o, const u64 *w, usize n)
{
  a.set_zero();
  ora::obn_zero(o);
  for ( usize i = n; i-- > 0; ) {
    a <<= 64;
    a += U(w[i]);
    ora::obn_shl(o, o, 64);
    ora::obn t;
    ora::obn_from_u64(t, w[i]);
    ora::obn_add(o, o, t);
  }
}

static void
gen(prng &rng, U &a, ora::obn &o, usize n, u32 pattern)
{
  u64 w[64];
  for ( usize i = 0; i < n; ++i ) {
    switch ( pattern ) {
    case 1:
      w[i] = ~0ull;
      break;
    case 2:
      w[i] = (i + 1u == n) ? (1ull << 63) : 0ull;
      break;
    case 3:
      w[i] = (i % 3u == 0u) ? rng.next() : 0ull;
      break;
    case 4:
      w[i] = (i & 1u) ? 0xAAAAAAAAAAAAAAAAull : 0x5555555555555555ull;
      break;
    default:
      w[i] = rng.next();
      break;
    }
  }
  if ( n > 0 && pattern == 3 ) w[n - 1u] |= 1ull;
  build(a, o, w, n);
}

static bool
same(const U &a, const ora::obn &o)
{
  return ora::obn_equals_limbs(o, a.limbs(), a.size());
}

int
main()
{
  print("=== ARBINT RIGOR ===");

  test_case("construction round-trips through the oracle");
  {
    prng rng(0x9E3779B97F4A7C15ull);
    for ( usize n = 1; n <= MAX_LIMBS; ++n )
      for ( u32 p = 0; p < 5; ++p ) {
        U a;
        ora::obn o;
        gen(rng, a, o, n, p);
        require_true(same(a, o));
        require(a.bit_length(), ora::obn_bitlen(o));
        require(a.popcount(), ora::obn_popcount(o));
      }
  }
  end_test_case();

  test_case("add / sub vs oracle");
  {
    prng rng(0xC0FFEE0DDF00Dull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      const usize an = 1u + rng.next_in(MAX_LIMBS);
      const usize bn = 1u + rng.next_in(MAX_LIMBS);
      U a, b;
      ora::obn oa, ob, oc;
      gen(rng, a, oa, an, static_cast<u32>(t % 5u));
      gen(rng, b, ob, bn, static_cast<u32>((t / 5u) % 5u));

      ora::obn_add(oc, oa, ob);
      require_true(same(a + b, oc));
      require_true(same(b + a, oc));

      if ( ora::obn_cmp(oa, ob) >= 0 ) {
        ora::obn_sub(oc, oa, ob);
        require_true(same(a - b, oc));
        require_true((b - a).is_zero());
      } else {
        ora::obn_sub(oc, ob, oa);
        require_true(same(b - a, oc));
        require_true((a - b).is_zero());
      }

      require_true((a - a).is_zero() && (a - a).size() == 0);
    }
  }
  end_test_case();

  test_case("mul / sqr vs oracle");
  {
    prng rng(0xF4F4F4F4F4ull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      const usize an = 1u + rng.next_in(MAX_LIMBS);
      const usize bn = 1u + rng.next_in(MAX_LIMBS);
      U a, b;
      ora::obn oa, ob, oc;
      gen(rng, a, oa, an, static_cast<u32>(t % 5u));
      gen(rng, b, ob, bn, static_cast<u32>((t / 3u) % 5u));
      if ( !ora::obn_fits_product(oa, ob) ) continue;

      ora::obn_mul(oc, oa, ob);
      require_true(same(a * b, oc));
      require_true(same(b * a, oc));

      if ( ora::obn_fits_product(oa, oa) ) {
        ora::obn_mul(oc, oa, oa);
        require_true(same(micron::math::sqr(a), oc));
        require_true(same(a * a, oc));
      }

      require_true((a * U(0u)).is_zero());
      require_true(a * U(1u) == a);
    }
  }
  end_test_case();

  test_case("divmod vs oracle, and q*d + r == n");
  {
    prng rng(0x5EEDF00D5EEDF00Dull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      const usize an = 1u + rng.next_in(MAX_LIMBS);
      const usize bn = 1u + rng.next_in(an);
      U a, b;
      ora::obn oa, ob, oq, orr;
      gen(rng, a, oa, an, static_cast<u32>(t % 5u));
      gen(rng, b, ob, bn, static_cast<u32>((t / 7u) % 5u));
      if ( b.is_zero() ) continue;

      ora::obn_divmod(oq, orr, oa, ob);
      const auto qr = micron::math::divmod(a, b);
      require_true(same(qr.quot, oq));
      require_true(same(qr.rem, orr));
      require_true(qr.quot * b + qr.rem == a);
      require_true(qr.rem < b);

      const U prod = a * b;
      require_true(prod / b == a);
      require_true((prod % b).is_zero());
    }
  }
  end_test_case();

  test_case("shifts vs oracle");
  {
    prng rng(0xABCDEF0123456789ull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      const usize an = 1u + rng.next_in(MAX_LIMBS);
      U a;
      ora::obn oa, oc;
      gen(rng, a, oa, an, static_cast<u32>(t % 5u));
      const usize k = rng.next_in(300);
      if ( ora::obn_bitlen(oa) + k > ora::obn_bits ) continue;

      ora::obn_shl(oc, oa, k);
      require_true(same(a << k, oc));
      ora::obn_shr(oc, oa, k);
      require_true(same(a >> k, oc));

      require_true(((a << k) >> k) == a);
      require_true((a >> (a.bit_length() + 1u)).is_zero());
    }
  }
  end_test_case();

  test_case("bitwise vs oracle");
  {
    prng rng(0x1234ABCD8765EF01ull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      const usize an = 1u + rng.next_in(MAX_LIMBS);
      const usize bn = 1u + rng.next_in(MAX_LIMBS);
      U a, b;
      ora::obn oa, ob, oc;
      gen(rng, a, oa, an, static_cast<u32>(t % 5u));
      gen(rng, b, ob, bn, static_cast<u32>((t / 5u) % 5u));

      ora::obn_and(oc, oa, ob);
      require_true(same(a & b, oc));
      ora::obn_ior(oc, oa, ob);
      require_true(same(a | b, oc));
      ora::obn_xor(oc, oa, ob);
      require_true(same(a ^ b, oc));
      require_true((a ^ a).is_zero());
      require_true((a & a) == a && (a | a) == a);
    }
  }
  end_test_case();

  test_case("ordering is total and agrees with the oracle");
  {
    prng rng(0xDEADBEEFCAFEBABEull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      const usize an = 1u + rng.next_in(MAX_LIMBS);
      const usize bn = 1u + rng.next_in(MAX_LIMBS);
      U a, b;
      ora::obn oa, ob;
      gen(rng, a, oa, an, static_cast<u32>(t % 5u));
      gen(rng, b, ob, bn, static_cast<u32>((t / 5u) % 5u));

      const int want = ora::obn_cmp(oa, ob);
      const int got = micron::math::cmp(a, b);
      require((got < 0) ? -1 : (got > 0 ? 1 : 0), want);
      require_true((a < b) == (want < 0));
      require_true((a > b) == (want > 0));
      require_true((a == b) == (want == 0));
      require_true((a <= b) == (want <= 0));
      require_true((a >= b) == (want >= 0));
      require_true((a != b) == (want != 0));
    }
  }
  end_test_case();

  test_case("signed identities hold against the unsigned magnitude");
  {
    prng rng(0x0F1E2D3C4B5A6978ull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      const usize an = 1u + rng.next_in(MAX_LIMBS / 2u + 1u);
      const usize bn = 1u + rng.next_in(MAX_LIMBS / 2u + 1u);
      U ma, mb;
      ora::obn oa, ob;
      gen(rng, ma, oa, an, static_cast<u32>(t % 5u));
      gen(rng, mb, ob, bn, static_cast<u32>((t / 5u) % 5u));
      if ( mb.is_zero() ) continue;

      for ( int sa = 0; sa < 2; ++sa )
        for ( int sb = 0; sb < 2; ++sb ) {
          const Z a(ma, sa != 0);
          const Z b(mb, sb != 0);

          require_true((a + b) - b == a);
          require_true((a - b) + b == a);
          require_true(-(-a) == a);
          require_true(micron::math::abs(a).magnitude() == ma);
          require_true((a * b).magnitude() == ma * mb);
          require_true((a * b).sign() == (a.is_zero() || b.is_zero() ? 0 : (a.sign() * b.sign())));

          const auto qr = micron::math::divmod(a, b);
          require_true(qr.quot * b + qr.rem == a);
          require_true(qr.rem.magnitude() < mb);
          require_true(qr.rem.is_zero() || qr.rem.sign() == a.sign());

          require_true(~a == -a - Z(1));
        }
    }
  }
  end_test_case();

  test_case("zero has exactly one representation");
  {
    const Z z0;
    const Z z1(0);
    const Z z2 = Z(5) - Z(5);
    const Z z3 = -Z(0);
    const Z z4 = Z(7) * Z(0);
    require_true(z0 == z1 && z1 == z2 && z2 == z3 && z3 == z4);
    require(z0.sign(), 0);
    require(z3.sign(), 0);
    require_true(!z3.negative());
    require(z4.size(), usize(0));
    require_true(!static_cast<bool>(z0));
  }
  end_test_case();

  print("=== ARBINT RIGOR PASSED ===");
  return 1;
}
