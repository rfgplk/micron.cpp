// math_arbint_gcd.cpp
// gcd, lcm and the modular inverse: every tier against the oracle, and against the identities that
// hold at any width the oracle cannot reach.

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
namespace mpn = micron::math::mpn;

#if defined(ARBINT_RIGOR_LITE) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_TRIALS = 4;
constexpr static const usize MAX_BITS = 320;
#else
constexpr static const usize N_TRIALS = 16;
constexpr static const usize MAX_BITS = 1024;
#endif

static_assert(MAX_BITS * 2u <= ora::obn_bits, "the oracle cannot hold the products this file forms");

using U = arbuint<>;
using SI = arbint<>;

static U
rand_bits(prng &rng, usize bits)
{
  U v;
  for ( usize i = 0; i < (bits + 63u) / 64u; ++i ) {
    v <<= 64;
    v += U(rng.next());
  }
  return v;
}

static bool
to_oracle(ora::obn &o, const U &a)
{
  return ora::obn_from_limbs(o, a.limbs(), a.size());
}

static usize
sweep_limbs(usize idx) noexcept
{
  const usize anchors[] = { 1u, 2u, 3u, 4u, 5u, 8u, 16u, mpn::threshold::gcd_lehmer, mpn::threshold::gcd_hgcd, 12u };
  constexpr usize n_anchors = sizeof(anchors) / sizeof(anchors[0]);
  const usize a = anchors[(idx / 3u) % n_anchors];
  const usize off = idx % 3u;
  usize v = (off == 0u) ? (a > 1u ? a - 1u : 1u) : (off == 1u ? a : a + 1u);
  if ( v < 1u ) v = 1u;
  if ( v > MAX_BITS / 64u ) v = MAX_BITS / 64u;
  return v;
}

constexpr static const usize n_sweep = 30;

int
main()
{
  print("=== ARBINT GCD RIGOR ===");
  print("    gcd tiers built through: ", static_cast<u64>(static_cast<u8>(mpn::gcd_tiers_built)), ", cap ",
        static_cast<u64>(static_cast<u8>(mpn::gcd_tier_cap)));

  test_case("the builtin's edge table, at bignum width");
  {

    require_true(micron::math::gcd(U(12u), U(8u)) == U(4u));
    require_true(micron::math::gcd(U(100u), U(75u)) == U(25u));
    require_true(micron::math::gcd(U(17u), U(13u)) == U(1u));
    require_true(micron::math::gcd(U(1024u), U(768u)) == U(256u));
    require_true(micron::math::gcd(U(999999937u), U(999999929u)) == U(1u));
    require_true(micron::math::gcd(U(210u), U(165u)) == U(15u));
    require_true(micron::math::gcd(U(1u), U(1u)) == U(1u));
    require_true(micron::math::gcd(U(1u), U(999u)) == U(1u));
    require_true(micron::math::gcd(U(0u), U(7u)) == U(7u));
    require_true(micron::math::gcd(U(7u), U(0u)) == U(7u));
    require_true(micron::math::gcd(U(0u), U(0u)).is_zero());
    require_true(micron::math::gcd(U(7u), U(7u)) == U(7u));
    require_true(micron::math::gcd(U(128u), U(128u)) == U(128u));
    require_true(micron::math::gcd(U(1000000u), U(1000000u)) == U(1000000u));
    require_true(micron::math::gcd(U(256u), U(64u)) == U(64u));
    require_true(micron::math::gcd(U(1024u), U(32u)) == U(32u));
    require_true(micron::math::gcd(U(2u), U(1u)) == U(1u));
    require_true(micron::math::gcd(U(65536u), U(4096u)) == U(4096u));

    require_true(micron::math::gcd(SI(-12), SI(8)) == SI(4));
    require_true(micron::math::gcd(SI(12), SI(-8)) == SI(4));
    require_true(micron::math::gcd(SI(-12), SI(-8)) == SI(4));
    require_true(micron::math::gcd(SI(-100), SI(75)) == SI(25));
    require_true(micron::math::gcd(SI(0), SI(-5)) == SI(5));
    require_true(micron::math::gcd(SI(-5), SI(0)) == SI(5));

    require_true(micron::math::lcm(U(0u), U(5u)).is_zero());
    require_true(micron::math::lcm(U(5u), U(0u)).is_zero());
    require_true(micron::math::lcm(U(4u), U(6u)) == U(12u));
    require_true(micron::math::lcm(U(21u), U(6u)) == U(42u));
  }
  end_test_case();

  test_case("the bignum gcd agrees with the builtin one on 64-bit pairs");
  {

    prng rng(0x1B0B0BEEFCAFE001ull);
    for ( usize t = 0; t < 4000u; ++t ) {
      const u64 x = rng.next();
      const u64 y = rng.next();
      const u64 want = micron::math::gcd<u64>(x, y);
      require_true(micron::math::gcd(U(x), U(y)) == U(want));

      const u64 xs = x << 13, ys = y << 13;
      require_true(micron::math::gcd(U(xs), U(ys)) == U(micron::math::gcd<u64>(xs, ys)));
    }
  }
  end_test_case();

  test_case("gcd agrees with the oracle at every threshold seam");
  {
    prng rng(0x2C0FFEE5D0D0F00Dull);
    for ( usize si = 0; si < n_sweep; ++si ) {
      const usize n = sweep_limbs(si);
      for ( u32 p = 0; p < 4; ++p ) {
        U a = rand_bits(rng, n * 64u);
        U b = rand_bits(rng, n * 64u);
        switch ( p ) {
        case 1:
          b = a;
          break;
        case 2:
          b = a * U(rng.next() | 1ull);
          break;
        case 3:
          a <<= 100;
          b <<= 100;
          break;
        default:
          break;
        }
        ora::obn oa, ob, og;
        if ( !to_oracle(oa, a) || !to_oracle(ob, b) ) continue;
        ora::obn_gcd(og, oa, ob);
        const U g = micron::math::gcd(a, b);
        require_true(ora::obn_equals_limbs(og, g.limbs(), g.size()));
      }
    }
  }
  end_test_case();

  test_case("the identities need no oracle at all");
  {
    prng rng(0x3D0D0CAFE1234567ull);
    for ( usize t = 0; t < N_TRIALS * 4u; ++t ) {
      const usize bits = 64u + (t * 37u) % MAX_BITS;
      U a = rand_bits(rng, bits);
      U b = rand_bits(rng, bits);
      if ( a.is_zero() ) a = U(1u);
      if ( b.is_zero() ) b = U(1u);

      const U g = micron::math::gcd(a, b);
      require_true(!g.is_zero());
      require_true((a % g).is_zero());
      require_true((b % g).is_zero());
      require_true(micron::math::gcd(a / g, b / g) == U(1u));
      require_true(micron::math::gcd(a, b) == micron::math::gcd(b, a));
      require_true(micron::math::gcd(a, b) * micron::math::lcm(a, b) == a * b);

      if ( b > a ) require_true(micron::math::gcd(a, b) == micron::math::gcd(a, b - a));
    }
  }
  end_test_case();

  test_case("a constructed gcd comes back exactly");
  {

    prng rng(0x4E1234567890ABCDull);
    for ( usize t = 0; t < N_TRIALS * 2u; ++t ) {
      const usize bits = 64u + (t * 53u) % (MAX_BITS / 2u);
      U u = rand_bits(rng, bits) | U(1u);
      U v = rand_bits(rng, bits) | U(1u);
      if ( micron::math::gcd(u, v) != U(1u) ) continue;
      const U k = rand_bits(rng, bits / 2u + 1u) + U(1u);
      require_true(micron::math::gcd(u * k, v * k) == k);
    }
  }
  end_test_case();

  test_case("Fibonacci pairs: Euclid's worst case, and two free known answers");
  {

    constexpr usize NF = 200;
    static U fib[NF];
    fib[0] = U(0u);
    fib[1] = U(1u);
    for ( usize i = 2; i < NF; ++i ) fib[i] = fib[i - 1u] + fib[i - 2u];

    for ( usize i = 2; i < NF; ++i ) require_true(micron::math::gcd(fib[i], fib[i - 1u]) == U(1u));

    const usize ms[] = { 6u, 12u, 15u, 18u, 24u, 30u, 36u, 60u, 90u, 120u };
    for ( usize i = 0; i < sizeof(ms) / sizeof(ms[0]); ++i )
      for ( usize j = 0; j < sizeof(ms) / sizeof(ms[0]); ++j ) {
        const usize m = ms[i], n = ms[j];
        require_true(micron::math::gcd(fib[m], fib[n]) == fib[micron::math::gcd<usize>(m, n)]);
      }
  }
  end_test_case();

  test_case("invmod, against the oracle and against gcd");
  {
    prng rng(0x5F0F0F0F0BADBEEFull);
    for ( usize t = 0; t < N_TRIALS * 6u; ++t ) {
      const usize bits = 64u + (t * 41u) % (MAX_BITS / 2u);
      U m = rand_bits(rng, bits);
      if ( m < U(2u) ) m = U(1000003u);
      U a = rand_bits(rng, bits) % m;

      const auto r = micron::math::invmod(a, m);
      const U g = micron::math::gcd(a, m);

      require_true(r.exists == (g == U(1u)));
      if ( r.exists ) {
        require_true(r.value < m);
        require_true(micron::math::mulmod(a, r.value, m) == U(1u));

        const auto back = micron::math::invmod(r.value, m);
        require_true(back.exists);
        require_true(back.value == a % m);
      }

      ora::obn oa, om, oi;
      if ( !to_oracle(oa, a) || !to_oracle(om, m) ) continue;
      const int oracle = ora::obn_invmod(oi, oa, om);
      require_true(oracle >= 0);
      require_true((oracle == 1) == r.exists);
      if ( oracle == 1 ) require_true(ora::obn_equals_limbs(oi, r.value.limbs(), r.value.size()));
    }
  }
  end_test_case();

  test_case("invmod's edge cases");
  {
    const U m(1000003u);
    require_true(micron::math::invmod(U(1u), m).value == U(1u));
    require_true(micron::math::invmod(m - U(1u), m).value == m - U(1u));
    require_true(!micron::math::invmod(U(0u), m).exists);
    require_true(!micron::math::invmod(m, m).exists);

    const auto one = micron::math::invmod(U(7u), U(1u));
    require_true(one.exists);
    require_true(one.value.is_zero());

    const U c(1000u);
    require_true(!micron::math::invmod(U(2u), c).exists);
    require_true(!micron::math::invmod(U(5u), c).exists);
    require_true(micron::math::invmod(U(3u), c).exists);
    require_true(micron::math::mulmod(U(3u), micron::math::invmod(U(3u), c).value, c) == U(1u));

    const SI sm(1000003);
    const auto sr = micron::math::invmod(SI(-3), sm);
    require_true(sr.exists);
    require_true(sr.value.sign() >= 0);
    const U back = (micron::math::abs(sm).magnitude() - U(3u));
    require_true(micron::math::mulmod(back, sr.value.magnitude(), micron::math::abs(sm).magnitude()) == U(1u));
  }
  end_test_case();

  test_case("invmod closes the RSA loop it was standing in for");
  {

    const U p = U::power_of_two(521u) - U(1u);
    const U q = U::power_of_two(607u) - U(1u);
    const U n = p * q;
    const U phi = (p - U(1u)) * (q - U(1u));
    const U e(65537u);

    const auto d = micron::math::invmod(e, phi);
    require_true(d.exists);
    require_true(micron::math::mulmod(d.value, e, phi) == U(1u));

    prng rng(0x6A5A5A5A5A5A5A5Aull);
    for ( usize t = 0; t < 2; ++t ) {
      const U msg = rand_bits(rng, 900u) % n;
      require_true(micron::math::powmod(micron::math::powmod(msg, e, n), d.value, n) == msg);
    }
  }
  end_test_case();

  test_case("every gcd tier agrees, limb for limb, at every threshold seam");
  {

    constexpr usize CAP = 40u;
    static mpn::limb_t ub[CAP], vb[CAP], g1[CAP + 2], g2[CAP + 2];
    static mpn::limb_t sc[32u * CAP + 8192u];
    prng rng(0x9E11A7C0DEBADCAFull);

    for ( usize un = 1; un <= CAP; ++un ) {
      for ( usize vn = 1; vn <= un; ++vn ) {
        for ( u32 p = 0; p < 4; ++p ) {
          for ( usize i = 0; i < un; ++i ) ub[i] = static_cast<mpn::limb_t>(rng.next());
          for ( usize i = 0; i < vn; ++i ) vb[i] = static_cast<mpn::limb_t>(rng.next());
          switch ( p ) {
          case 1:
            for ( usize i = 0; i < un; ++i ) ub[i] = mpn::limb_max;
            break;
          case 2:
            for ( usize i = 0; i < vn; ++i ) vb[i] = ub[i];
            break;
          case 3:
            ub[0] &= ~static_cast<mpn::limb_t>(0xFFu);
            vb[0] &= ~static_cast<mpn::limb_t>(0xFFu);
            break;
          default:
            break;
          }
          ub[un - 1u] |= 1u;
          vb[vn - 1u] |= 1u;

          const usize n1 = mpn::gcd_with<mpn::gcd_algo::binary>(g1, ub, un, vb, vn, sc);
          const usize n2 = mpn::gcd_with<mpn::gcd_algo::lehmer>(g2, ub, un, vb, vn, sc);
          require_true(n1 == n2);
          for ( usize i = 0; i < n1; ++i ) require_true(g1[i] == g2[i]);

          const usize need = mpn::gcd_itch_with<mpn::gcd_algo::dc>(un, vn);
          require_true(need + vn <= sizeof(sc) / sizeof(sc[0]));
          sc[need] = 0xA5A5A5A5u;
          const usize n3 = mpn::gcd_with<mpn::gcd_algo::dc>(g1, ub, un, vb, vn, sc);
          require_true(sc[need] == 0xA5A5A5A5u);
          require_true(n3 == n2);
          for ( usize i = 0; i < n3; ++i ) require_true(g1[i] == g2[i]);
        }
      }
    }
  }
  end_test_case();

  test_case("the single-limb step never reports progress it did not make");
  {

    prng rng(0xAF1E5CA7DEC0DE99ull);
    for ( usize t = 0; t < 200000u; ++t ) {
      const mpn::limb_t A = static_cast<mpn::limb_t>(rng.next());
      mpn::limb_t B = static_cast<mpn::limb_t>(rng.next());
      if ( B > A ) B = A;
      mpn::mat1 m{};
      if ( mpn::hgcd2(m, A, B) ) {
        require_true(!(m.u00 == 1u && m.u01 == 0u && m.u10 == 0u && m.u11 == 1u));

        mpn::limb_t p0 = 0, p1 = 0, q0 = 0, q1 = 0;
        mpn::mul_wide(m.u00, m.u11, p0, p1);
        mpn::mul_wide(m.u01, m.u10, q0, q1);
        require_true(p1 == q1 && p0 == static_cast<mpn::limb_t>(q0 + 1u));
      } else {
        require_true(m.u00 == 1u && m.u01 == 0u && m.u10 == 0u && m.u11 == 1u);
      }
    }
  }
  end_test_case();

  test_case("an all-ones leading limb does not divide by zero");
  {

    static mpn::limb_t a[6], b[6], g[8], sc[8192];
    for ( usize n = 3; n <= 6; ++n ) {
      for ( usize i = 0; i < n; ++i ) {
        a[i] = mpn::limb_max;
        b[i] = mpn::limb_max;
      }
      b[0] = static_cast<mpn::limb_t>(mpn::limb_max - 1u);

      const usize n1 = mpn::gcd_with<mpn::gcd_algo::lehmer>(g, a, n, b, n, sc);
      require_true(n1 == 1 && g[0] == 1u);
      const usize n2 = mpn::gcd_with<mpn::gcd_algo::binary>(g, a, n, b, n, sc);
      require_true(n2 == 1 && g[0] == 1u);
      const usize n3 = mpn::gcd_with<mpn::gcd_algo::dc>(g, a, n, b, n, sc);
      require_true(n3 == 1 && g[0] == 1u);

      mpn::mat1 m{};
      (void)mpn::hgcd2(m, mpn::limb_max, mpn::limb_max);
      mpn::mat1 me{}, ne{};
      (void)mpn::hgcd2_ext(me, ne, mpn::limb_max, mpn::limb_max);
      (void)mpn::hgcd2_w2(m, mpn::limb_max, mpn::limb_max, mpn::limb_max, mpn::limb_max);
    }
  }
  end_test_case();

  test_case("the two-limb window simulates a PREFIX of the true quotient sequence");
  {

    prng rng(0xC0FFEE1234567890ull);
    for ( usize t = 0; t < 60000u; ++t ) {
      mpn::limb_t ah = static_cast<mpn::limb_t>(rng.next());
      mpn::limb_t al = static_cast<mpn::limb_t>(rng.next());
      mpn::limb_t bh = static_cast<mpn::limb_t>(rng.next());
      mpn::limb_t bl = static_cast<mpn::limb_t>(rng.next());
      if ( (t & 7u) == 0u ) bh >>= (rng.next() % mpn::limb_bits);
      ah |= mpn::limb_msb;
      mpn::dlimb_t A = mpn::join(ah, al);
      mpn::dlimb_t B = mpn::join(bh, bl);
      if ( A < B ) {
        const mpn::dlimb_t s = A;
        A = B;
        B = s;
      }

      mpn::mat1 m{};
      const bool ok = mpn::hgcd2_w2(m, mpn::hi_half(A), mpn::lo_half(A), mpn::hi_half(B), mpn::lo_half(B));

      if ( !ok ) {
        require_true(m.u00 == 1u && m.u01 == 0u && m.u10 == 0u && m.u11 == 1u);
        continue;
      }

      require_true(!(m.u00 == 1u && m.u01 == 0u && m.u10 == 0u && m.u11 == 1u));

      mpn::mat1 c = mpn::mat1_identity();
      mpn::dlimb_t x = A, y = B;
      bool found = false;
      for ( usize i = 0; i < 4u * mpn::limb_bits; ++i ) {
        if ( c.u00 == m.u00 && c.u01 == m.u01 && c.u10 == m.u10 && c.u11 == m.u11 ) {
          found = true;
          break;
        }
        if ( (i & 1u) == 0u ) {
          if ( y == 0 ) break;
          const mpn::dlimb_t q = x / y;
          if ( q == 0 ) break;
          const mpn::dlimb_t u01 = static_cast<mpn::dlimb_t>(c.u01) + q * static_cast<mpn::dlimb_t>(c.u00);
          const mpn::dlimb_t u11 = static_cast<mpn::dlimb_t>(c.u11) + q * static_cast<mpn::dlimb_t>(c.u10);
          if ( mpn::hi_half(u01) != 0 || mpn::hi_half(u11) != 0 ) break;
          c.u01 = mpn::lo_half(u01);
          c.u11 = mpn::lo_half(u11);
          x -= q * y;
        } else {
          if ( x == 0 ) break;
          const mpn::dlimb_t q = y / x;
          if ( q == 0 ) break;
          const mpn::dlimb_t u00 = static_cast<mpn::dlimb_t>(c.u00) + q * static_cast<mpn::dlimb_t>(c.u01);
          const mpn::dlimb_t u10 = static_cast<mpn::dlimb_t>(c.u10) + q * static_cast<mpn::dlimb_t>(c.u11);
          if ( mpn::hi_half(u00) != 0 || mpn::hi_half(u10) != 0 ) break;
          c.u00 = mpn::lo_half(u00);
          c.u10 = mpn::lo_half(u10);
          y -= q * x;
        }
      }
      require_true(found);

      mpn::limb_t p0 = 0, p1 = 0, q0 = 0, q1 = 0;
      mpn::mul_wide(m.u00, m.u11, p0, p1);
      mpn::mul_wide(m.u01, m.u10, q0, q1);
      require_true(p1 == q1 && p0 == static_cast<mpn::limb_t>(q0 + 1u));
    }
  }
  end_test_case();

  test_case("hgcd's postcondition, which is what every caller of it relies on");
  {

    constexpr usize NMAX = 48u;
    static mpn::limb_t ab[NMAX], bb[NMAX], oa[NMAX], ob[NMAX];
    static mpn::limb_t msp[4u * (NMAX / 2u + 4u)];
    static mpn::limb_t hsc[64u * NMAX + 16384u];
    static mpn::limb_t gg1[NMAX + 2], gg2[NMAX + 2];
    static mpn::limb_t gsc[64u * NMAX + 16384u];

    prng rng(0x1BADB0021DEAD00Dull);
    usize progressed = 0, ran = 0;

    for ( usize t = 0; t < N_TRIALS * 40u; ++t ) {
      const usize n = 2u + (t % (NMAX - 2u));
      for ( usize i = 0; i < n; ++i ) {
        ab[i] = static_cast<mpn::limb_t>(rng.next());
        bb[i] = static_cast<mpn::limb_t>(rng.next());
      }
      ab[n - 1u] |= mpn::limb_msb;
      if ( (t & 3u) == 0u ) bb[n - 1u] &= static_cast<mpn::limb_t>(mpn::limb_msb - 1u);
      if ( mpn::cmp_var(ab, n, bb, n) <= 0 ) continue;
      for ( usize i = 0; i < n; ++i ) {
        oa[i] = ab[i];
        ob[i] = bb[i];
      }

      const usize itch = mpn::hgcd_itch(n);
      require_true(itch < sizeof(hsc) / sizeof(hsc[0]));
      require_true(mpn::hgcd_mat_itch(n) <= sizeof(msp) / sizeof(msp[0]));
      hsc[itch] = 0x5A5A5A5Au;

      mpn::hgcd_mat M{};
      mpn::hgcd_mat_init(M, n, msp);
      const usize nn = mpn::hgcd(ab, bb, n, M, hsc);
      ++ran;

      require_true(hsc[itch] == 0x5A5A5A5Au);

      if ( nn == 0 ) {

        for ( usize i = 0; i < n; ++i ) require_true(ab[i] == oa[i] && bb[i] == ob[i]);
        continue;
      }
      ++progressed;
      require_true(nn <= n);

      U A, B, a2, b2;
      A.__assign_limbs(oa, mpn::normalize(oa, n));
      B.__assign_limbs(ob, mpn::normalize(ob, n));
      a2.__assign_limbs(ab, mpn::normalize(ab, nn));
      b2.__assign_limbs(bb, mpn::normalize(bb, nn));

      require_true(a2 > b2);

      U m00, m01, m10, m11;
      m00.__assign_limbs(M.p[0][0], mpn::normalize(M.p[0][0], M.n));
      m01.__assign_limbs(M.p[0][1], mpn::normalize(M.p[0][1], M.n));
      m10.__assign_limbs(M.p[1][0], mpn::normalize(M.p[1][0], M.n));
      m11.__assign_limbs(M.p[1][1], mpn::normalize(M.p[1][1], M.n));

      require_true(m00 * m11 - m01 * m10 == U(1u));
      require_true(m00 * a2 + m01 * b2 == A);
      require_true(m10 * a2 + m11 * b2 == B);

      require_true(M.n + 2u <= (n + 1u) / 2u + 4u);
      for ( usize i = 0; i < 2u; ++i )
        for ( usize j = 0; j < 2u; ++j )
          for ( usize k = M.n; k < (n + 1u) / 2u + 2u; ++k ) require_true(M.p[i][j][k] == 0);

      const usize k1 = mpn::gcd(gg1, oa, n, ob, n, gsc);
      const usize k2 = mpn::gcd(gg2, ab, nn, bb, nn, gsc);
      require_true(k1 == k2);
      for ( usize i = 0; i < k1; ++i ) require_true(gg1[i] == gg2[i]);
    }

    require_true(progressed * 4u > ran);
  }
  end_test_case();

  test_case("hgcd's degenerate shapes, which a random sweep reaches only by luck");
  {
    constexpr usize NMAX = 16u;
    static mpn::limb_t ab[NMAX], bb[NMAX];
    static mpn::limb_t msp[4u * (NMAX / 2u + 4u)];
    static mpn::limb_t hsc[64u * NMAX + 16384u];

    for ( u32 shape = 0; shape < 6u; ++shape ) {
      for ( usize n = 2u; n <= NMAX; ++n ) {
        for ( usize i = 0; i < n; ++i ) {
          ab[i] = mpn::limb_max;
          bb[i] = 0;
        }
        switch ( shape ) {
        case 0:
          break;
        case 1:
          bb[0] = 1u;
          break;
        case 2:
          for ( usize i = 0; i < n; ++i ) bb[i] = ab[i];
          bb[0] = static_cast<mpn::limb_t>(ab[0] - 1u);
          break;
        case 3:
          for ( usize i = 0; i < n; ++i ) bb[i] = ab[i];
          bb[n - 1u] = static_cast<mpn::limb_t>(ab[n - 1u] - 1u);
          break;
        case 4:
          for ( usize i = 0; i < n; ++i ) ab[i] = 0;
          ab[n - 1u] = mpn::limb_msb;
          for ( usize i = 0; i < n; ++i ) bb[i] = mpn::limb_max;
          bb[n - 1u] = static_cast<mpn::limb_t>(mpn::limb_msb - 1u);
          break;
        default:
          for ( usize i = 0; i + 1u < n; ++i ) bb[i] = mpn::limb_max;
          break;
        }
        if ( mpn::cmp_var(ab, n, bb, n) <= 0 ) continue;

        const usize itch = mpn::hgcd_itch(n);
        hsc[itch] = 0x33333333u;
        mpn::hgcd_mat M{};
        mpn::hgcd_mat_init(M, n, msp);
        const usize nn = mpn::hgcd(ab, bb, n, M, hsc);
        require_true(hsc[itch] == 0x33333333u);
        require_true(nn <= n);

        U m00, m01, m10, m11;
        m00.__assign_limbs(M.p[0][0], mpn::normalize(M.p[0][0], M.n));
        m01.__assign_limbs(M.p[0][1], mpn::normalize(M.p[0][1], M.n));
        m10.__assign_limbs(M.p[1][0], mpn::normalize(M.p[1][0], M.n));
        m11.__assign_limbs(M.p[1][1], mpn::normalize(M.p[1][1], M.n));
        require_true(m00 * m11 - m01 * m10 == U(1u));
      }
    }
  }
  end_test_case();

  test_case("the ladder never names a tier it cannot run");
  {
    static_assert(static_cast<u8>(mpn::gcd_tier_cap) <= static_cast<u8>(mpn::gcd_tiers_built));
    for ( usize n = 1; n <= 200000u; n = n * 3u / 2u + 1u ) {
      const mpn::gcd_algo picked = mpn::pick_gcd(n, n);
      const mpn::gcd_algo run = mpn::clamp_to(picked, mpn::gcd_tier_cap);
      require_true(static_cast<u8>(run) <= static_cast<u8>(mpn::gcd_tier_cap));
      require_true(static_cast<u8>(run) <= static_cast<u8>(picked));
    }

    mpn::gcd_algo prev = mpn::pick_gcd(1, 1);
    for ( usize n = 1; n <= 200000u; n = n * 2u ) {
      const mpn::gcd_algo cur = mpn::pick_gcd(n, n);
      require_true(static_cast<u8>(cur) >= static_cast<u8>(prev));
      prev = cur;
    }
  }
  end_test_case();

  test_case("the scratch promise covers what the tiers actually carve");
  {

    constexpr usize CAP = 48u;
    static mpn::limb_t ub[CAP], vb[CAP], gb[CAP + 2];
    static mpn::limb_t sc[32u * CAP + 8192u];
    constexpr mpn::limb_t canary = static_cast<mpn::limb_t>(0xA5A5A5A5A5A5A5A5ull);

    prng rng(0x7B00B1E5CA7CA71Aull);
    for ( usize un = 1; un <= CAP; ++un ) {
      for ( usize vn = 1; vn <= un; vn = vn * 2u + 1u ) {
        for ( usize i = 0; i < un; ++i ) ub[i] = static_cast<mpn::limb_t>(rng.next());
        for ( usize i = 0; i < vn; ++i ) vb[i] = static_cast<mpn::limb_t>(rng.next());
        ub[un - 1u] |= 1u;
        vb[vn - 1u] |= 1u;

        const usize itch = mpn::gcd_itch(un, vn);
        require_true(itch + 1u < sizeof(sc) / sizeof(sc[0]));
        sc[itch] = canary;
        const usize gn = mpn::gcd(gb, ub, un, vb, vn, sc);
        require_true(sc[itch] == canary);
        require_true(gn <= (un < vn ? un : vn));

        const usize iitch = mpn::invmod_itch(un, vn);
        require_true(iitch + 1u < sizeof(sc) / sizeof(sc[0]));
        sc[iitch] = canary;
        usize rn = 0;
        (void)mpn::invmod(gb, rn, ub, un, vb, vn, sc);
        require_true(sc[iitch] == canary);
      }
    }
  }
  end_test_case();

  test_case("a bounded type reaches the same answers with no allocator at all");
  {

    using BU = arbuint<512>;
    prng rng(0x8C0FFEEB0BB1E123ull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      BU a, b;
      U da, db;
      for ( usize i = 0; i < 8; ++i ) {
        const u64 x = rng.next(), y = rng.next();
        a <<= 64;
        a += BU(x);
        b <<= 64;
        b += BU(y);
        da <<= 64;
        da += U(x);
        db <<= 64;
        db += U(y);
      }
      const BU g = micron::math::gcd(a, b);
      const U dg = micron::math::gcd(da, db);
      require_true(g.size() == dg.size());
      for ( usize i = 0; i < g.size(); ++i ) require_true(g[i] == dg[i]);

      BU m = b | BU(1u);
      if ( m < BU(3u) ) m = BU(1000003u);
      const auto r = micron::math::invmod(a, m);
      if ( r.exists ) require_true(micron::math::mulmod(a % m, r.value, m) == BU(1u));
    }
  }
  end_test_case();

  print("=== ARBINT GCD RIGOR PASSED ===");
  return 1;
}
