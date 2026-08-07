// math_arbint_modular.cpp
// The modular layer: Montgomery and Barrett contexts, sliding-window powmod, mulmod.

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

using micron::math::arbuint;
using micron::math::barrett;
using micron::math::montgomery;
using mtest::prng;
namespace ora = mtest::arbint_oracle;
namespace mpn = micron::math::mpn;
namespace solver = micron::math::solver;

#if defined(ARBINT_RIGOR_LITE) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_TRIALS = 3;
constexpr static const usize MAX_N = 12;
constexpr static const usize ORACLE_MOD_BITS = 256;
#else
constexpr static const usize N_TRIALS = 12;
constexpr static const usize MAX_N = 40;
constexpr static const usize ORACLE_MOD_BITS = 1024;
#endif

static_assert(ORACLE_MOD_BITS * 2u <= ora::obn_bits, "the oracle cannot hold m^2 at this modulus size");

using U = arbuint<>;

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
  const usize anchors[] = { 1u,
                            2u,
                            3u,
                            4u,
                            5u,
                            6u,
                            8u,
                            16u,
                            17u,
                            mpn::threshold::mul_karatsuba,
                            mpn::threshold::sqr_karatsuba,
                            mpn::threshold::inv_newton,
                            mpn::threshold::div_dc,
                            MAX_N };
  constexpr usize n_anchors = sizeof(anchors) / sizeof(anchors[0]);
  const usize a = anchors[(idx / 3u) % n_anchors];
  const usize off = idx % 3u;
  usize v = (off == 0u) ? (a > 1u ? a - 1u : 1u) : (off == 1u ? a : a + 1u);
  if ( v < 1u ) v = 1u;
  if ( v > MAX_N ) v = MAX_N;
  return v;
}

constexpr static const usize n_sweep = 42;

static usize
sweep_exp_bits(usize idx) noexcept
{
  const usize anchors[] = {
    1u, 2u, 3u, 63u, 64u, 65u, mpn::threshold::powm_win3, mpn::threshold::powm_win4, mpn::threshold::powm_win5, 127u, 128u, 129u, 256u
  };
  constexpr usize n_anchors = sizeof(anchors) / sizeof(anchors[0]);
  const usize a = anchors[(idx / 3u) % n_anchors];
  const usize off = idx % 3u;
  usize v = (off == 0u) ? (a > 1u ? a - 1u : 1u) : (off == 1u ? a : a + 1u);
  if ( v < 1u ) v = 1u;
  if ( v > 512u ) v = 512u;
  return v;
}

constexpr static const usize n_exp_sweep = 39;

static U
pattern_modulus(prng &rng, usize n, u32 p, bool force_odd)
{
  U m;
  for ( usize i = 0; i < n; ++i ) {
    const bool top = (i == 0);
    const bool low = (i + 1u == n);
    m <<= 64;
    switch ( p ) {
    case 1:
      m += U(~0ull);
      break;
    case 2:
      m += U(top ? 1ull : 0ull);
      break;
    case 3:
      m += U(low ? 1ull : rng.next());
      break;
    case 4:
      m += U(low ? ~0ull : rng.next());
      break;
    default:
      m += U(rng.next());
      break;
    }
  }
  if ( m < U(3u) ) m = U(0x10001u);
  if ( force_odd && m.even() ) m += U(1u);
  return m;
}

int
main()
{
  print("=== ARBINT MODULAR RIGOR ===");
  print("    powm window bands (exponent bits): ", static_cast<u64>(mpn::threshold::powm_win3), " ",
        static_cast<u64>(mpn::threshold::powm_win4), " ", static_cast<u64>(mpn::threshold::powm_win5), " ",
        static_cast<u64>(mpn::threshold::powm_win6), ", cap ", static_cast<u64>(mpn::threshold::powm_window_cap));
  print("    powm engine gate: ", static_cast<u64>(mpn::threshold::powm_engine), " (0 auto, 1 montgomery, 2 barrett)");

  test_case("montgomery round-trips every residue");
  {
    prng rng(0x4D01A7C0FFEE1234ull);
    for ( usize si = 0; si < n_sweep; ++si ) {
      const usize n = sweep_limbs(si);
      for ( u32 p = 0; p < 5; ++p ) {
        const U m = pattern_modulus(rng, n, p, true);
        const montgomery<U> mg(m);
        for ( usize t = 0; t < 3; ++t ) {
          const U a = rand_bits(rng, n * 64u);
          const U red = a % m;
          const U f = mg.to_form(a);
          require_true(f < m);
          require_true(mg.from_form(f) == red);

          U shifted = red;
          shifted <<= (mg.width() * mpn::limb_bits);
          require_true(f == shifted % m);
        }
      }
    }
  }
  end_test_case();

  test_case("montgomery mul and sqr are mulmod and sqrmod");
  {
    prng rng(0x5EED0FB1A5CDEF77ull);
    for ( usize si = 0; si < n_sweep; ++si ) {
      const usize n = sweep_limbs(si);
      for ( u32 p = 0; p < 5; ++p ) {
        const U m = pattern_modulus(rng, n, p, true);
        const montgomery<U> mg(m);
        for ( usize t = 0; t < 3; ++t ) {
          const U a = rand_bits(rng, n * 64u) % m;
          const U b = rand_bits(rng, n * 64u) % m;
          const U fa = mg.to_form(a);
          const U fb = mg.to_form(b);

          require_true(mg.from_form(mg.mul(fa, fb)) == micron::math::mulmod(a, b, m));
          require_true(mg.from_form(mg.sqr(fa)) == micron::math::sqrmod(a, m));

          require_true(mg.mul(fa, fa) == mg.sqr(fa));
        }
      }
    }
  }
  end_test_case();

  test_case("barrett reduce is the remainder, and barrett mul is mulmod");
  {
    prng rng(0x7EA1BEEF5AD1CE33ull);
    for ( usize si = 0; si < n_sweep; ++si ) {
      const usize n = sweep_limbs(si);
      for ( u32 p = 0; p < 5; ++p ) {
        const U m = pattern_modulus(rng, n, p, false);
        const barrett<U> br(m);
        require_true(br.width() == m.size());
        for ( usize t = 0; t < 3; ++t ) {
          const U a = rand_bits(rng, n * 64u);
          require_true(br.reduce(a) == a % m);
          const U ar = a % m;
          const U b = rand_bits(rng, n * 64u) % m;
          require_true(br.mul(ar, b) == micron::math::mulmod(ar, b, m));
          require_true(br.sqr(ar) == micron::math::sqrmod(ar, m));
        }
      }
    }
  }
  end_test_case();

  test_case("both reducers and plain division agree on the same inputs");
  {

    prng rng(0x5A11EDBA5EC0DE41ull);
    for ( usize si = 0; si < n_sweep; ++si ) {
      const usize n = sweep_limbs(si);
      const U m = pattern_modulus(rng, n, si % 5u, true);
      const montgomery<U> mg(m);
      const barrett<U> br(m);
      for ( usize t = 0; t < 4; ++t ) {
        const U a = rand_bits(rng, n * 64u) % m;
        const U b = rand_bits(rng, n * 64u) % m;
        const U viadiv = micron::math::mulmod(a, b, m);
        require_true(mg.from_form(mg.mul(mg.to_form(a), mg.to_form(b))) == viadiv);
        require_true(br.mul(a, b) == viadiv);
      }
    }
  }
  end_test_case();

  test_case("powmod is pow-then-mod for small exponents");
  {

    prng rng(0x0AC1E5DEADBEE123ull);
    for ( usize t = 0; t < N_TRIALS * 4u; ++t ) {
      const U a = rand_bits(rng, 48u);
      const U m = (rand_bits(rng, 40u) | U(1u)) + U(2u);
      for ( u64 e = 0; e <= 12u; ++e ) {
        const U want = micron::math::pow(a, e) % m;
        require_true(micron::math::powmod(a, U(e), m) == want);
        require_true(micron::math::powmod(a, e, m) == want);
      }
    }
  }
  end_test_case();

  test_case("powmod agrees with the oracle, odd and even moduli, across the window bands");
  {

    prng rng(0xFE13A7C0DEBABE11ull);
    for ( usize ei = 0; ei < n_exp_sweep; ei += 2u ) {
      const usize ebits = sweep_exp_bits(ei);
      for ( usize si = 0; si < 4u; ++si ) {
        const usize mbits = (si + 1u) * (ORACLE_MOD_BITS / 8u);
        for ( u32 odd = 0; odd < 2u; ++odd ) {
          U m = rand_bits(rng, mbits);
          if ( m.is_zero() ) m = U(5u);
          if ( odd != 0 && m.even() ) m += U(1u);
          if ( odd == 0 && m.odd() ) m += U(1u);
          if ( m < U(2u) ) m = U(4u) + U(odd);

          const U a = rand_bits(rng, mbits);
          const U e = rand_bits(rng, ebits);

          ora::obn om, oa, oe, orr;
          if ( !to_oracle(om, m) || !to_oracle(oa, a) || !to_oracle(oe, e) ) continue;

          require_true(ora::obn_fits_modexp(om));
          require_true(ora::obn_powmod(orr, oa, oe, om));

          const U got = micron::math::powmod(a, e, m);
          require_true(ora::obn_equals_limbs(orr, got.limbs(), got.size()));
        }
      }
    }
  }
  end_test_case();

  test_case("Fermat holds for Mersenne primes and fails for a Mersenne composite");
  {

    const usize mp_exps[] = { 127u, 521u, 607u, 1279u, 2203u };
    prng rng(0x2A5BEEF00D7B1D99ull);
    for ( usize i = 0; i < sizeof(mp_exps) / sizeof(mp_exps[0]); ++i ) {
      const U p = U::power_of_two(mp_exps[i]) - U(1u);
      const U pm1 = p - U(1u);
      for ( usize t = 0; t < 3; ++t ) {
        U a = rand_bits(rng, mp_exps[i] - 1u) % p;
        if ( a.is_zero() ) a = U(2u);
        require_true(micron::math::powmod(a, pm1, p) == U(1u));
      }
    }

    const U c = U::power_of_two(11u) - U(1u);
    usize failures = 0;
    for ( u64 a = 2; a < 40u; ++a )
      if ( micron::math::powmod(U(a), c - U(1u), c) != U(1u) ) ++failures;
    require_true(failures > 20u);
  }
  end_test_case();

  test_case("the RSA round trip");
  {

    const U p = U::power_of_two(521u) - U(1u);
    const U q = U::power_of_two(607u) - U(1u);
    const U n = p * q;
    const U phi = (p - U(1u)) * (q - U(1u));
    const U e(65537u);

    ora::obn oe, ophi, od;
    require_true(to_oracle(oe, e));
    require_true(to_oracle(ophi, phi));
    require_true(ora::obn_invmod(od, oe, ophi) == 1);

    U d;
    {

      for ( usize i = ora::obn_limbs; i-- > 0; ) {
        d <<= 32;
        d += U(static_cast<u64>(od.l[i]));
      }
    }
    require_true(micron::math::mulmod(d, e, phi) == U(1u));

    prng rng(0xDE6E4EA7EDCA5E13ull);
    for ( usize t = 0; t < 2; ++t ) {
      const U msg = rand_bits(rng, 900u) % n;
      const U ct = micron::math::powmod(msg, e, n);
      require_true(micron::math::powmod(ct, d, n) == msg);
    }
  }
  end_test_case();

  test_case("degenerate moduli, bases and exponents");
  {
    const U one(1u);
    prng rng(0xB00DEDC5CAB1EDCFull);

    for ( u64 b = 0; b < 5u; ++b )
      for ( u64 e = 0; e < 5u; ++e ) require_true(micron::math::powmod(U(b), U(e), one).is_zero());

    const U m(1000003u);
    require_true(micron::math::powmod(U(0u), U(0u), m) == one);
    require_true(micron::math::powmod(U(0u), U(5u), m).is_zero());
    require_true(micron::math::powmod(U(1u), rand_bits(rng, 200u), m) == one);
    require_true(micron::math::powmod(m, U(7u), m).is_zero());
    require_true(micron::math::powmod(m + one, U(7u), m) == one);
    require_true(micron::math::powmod(U(2u), U(0u), m) == one);

    require_true(micron::math::powmod(U(3u), U(101u), U(2u)) == one);
    require_true(micron::math::powmod(U(3u), U(20u), U(1024u)) == U(913u));

    const U wide = rand_bits(rng, 700u);
    ora::obn om, oa, oe, orr;
    require_true(to_oracle(om, m));
    require_true(to_oracle(oa, wide));
    require_true(to_oracle(oe, wide));
    require_true(ora::obn_powmod(orr, oa, oe, om));
    const U got = micron::math::powmod(wide, wide, m);
    require_true(ora::obn_equals_limbs(orr, got.limbs(), got.size()));
  }
  end_test_case();

  test_case("a bounded modulus never produces a value above itself");
  {

    using BU = arbuint<512>;
    prng rng(0x91ABCDE501DEF155ull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      BU m;
      for ( usize i = 0; i < 8; ++i ) {
        m <<= 64;
        m += BU(rng.next());
      }
      if ( m.even() ) m += BU(1u);
      if ( m < BU(3u) ) m = BU(1000003u);

      BU a, e;
      for ( usize i = 0; i < 8; ++i ) {
        a <<= 64;
        a += BU(rng.next());
        e <<= 64;
        e += BU(rng.next());
      }
      const montgomery<BU> mg(m);
      const BU r = micron::math::powmod(a, e, m);
      require_true(r < m);
      require_true(mg.pow(a, e) == r);
      require_true(mg.to_form(a) < m);
      require_true(mg.mul(mg.to_form(a), mg.to_form(a % m)) < m);

      U dm, da, de;
      for ( usize i = m.size(); i-- > 0; ) {
        dm <<= mpn::limb_bits;
        dm += U(static_cast<u64>(m[i]));
      }
      for ( usize i = a.size(); i-- > 0; ) {
        da <<= mpn::limb_bits;
        da += U(static_cast<u64>(a[i]));
      }
      for ( usize i = e.size(); i-- > 0; ) {
        de <<= mpn::limb_bits;
        de += U(static_cast<u64>(e[i]));
      }
      const U dr = micron::math::powmod(da, de, dm);
      require_true(ora::obn_equals_limbs(
          [&] {
            ora::obn o;
            (void)to_oracle(o, dr);
            return o;
          }(),
          r.limbs(), r.size()));
    }
  }
  end_test_case();

  test_case("a pinned solver reaches the same value as the automatic one");
  {

    using Base = arbuint<0, solver::basecase>;
    using Kara = arbuint<0, solver::karatsuba>;
    prng rng(0x5CABBADCAFE01777ull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      U a, e, m;
      Base ba, be, bm;
      Kara ka, ke, km;
      for ( usize i = 0; i < 6; ++i ) {
        const u64 xa = rng.next(), xe = rng.next(), xm = rng.next();
        a <<= 64;
        a += U(xa);
        e <<= 64;
        e += U(xe);
        m <<= 64;
        m += U(xm);
        ba <<= 64;
        ba += Base(xa);
        be <<= 64;
        be += Base(xe);
        bm <<= 64;
        bm += Base(xm);
        ka <<= 64;
        ka += Kara(xa);
        ke <<= 64;
        ke += Kara(xe);
        km <<= 64;
        km += Kara(xm);
      }
      if ( m.even() ) {
        m += U(1u);
        bm += Base(1u);
        km += Kara(1u);
      }
      const U r = micron::math::powmod(a, e, m);
      const Base rb = micron::math::powmod(ba, be, bm);
      const Kara rk = micron::math::powmod(ka, ke, km);
      require_true(r.size() == rb.size() && r.size() == rk.size());
      for ( usize i = 0; i < r.size(); ++i ) require_true(r[i] == rb[i] && r[i] == rk[i]);
    }
  }
  end_test_case();

  test_case("the scratch promise covers what the window actually carves");
  {

    constexpr usize CAP = 64u;
    static mpn::limb_t sc[64u * CAP + 65536u];
    constexpr mpn::limb_t canary = static_cast<mpn::limb_t>(0xA5A5A5A5A5A5A5A5ull);

    for ( usize n = 1; n <= CAP; n = n * 3u / 2u + 1u ) {
      for ( usize ei = 0; ei < n_exp_sweep; ++ei ) {
        const usize ebits = sweep_exp_bits(ei);
        const usize k = mpn::powm_window_for(ebits, n, mpn::threshold::powm_table_limbs);
        require_true(k >= 1u && k <= 8u);

        const usize floorwant = (usize{ 1 } << (k - 1u)) * n + 2u * n;
        require_true(mpn::powm_itch(n, n, ebits) >= floorwant);

        require_true((usize{ 1 } << (k - 1u)) * n <= mpn::threshold::powm_table_limbs || k == 1u);
      }
    }

    prng rng(0x3D6E1EAFACE33D17ull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      const usize n = 1u + (t % 20u);
      static mpn::limb_t mb[CAP], ab[CAP], eb[CAP], rb[CAP];
      for ( usize i = 0; i < n; ++i ) {
        mb[i] = static_cast<mpn::limb_t>(rng.next());
        ab[i] = static_cast<mpn::limb_t>(rng.next());
        eb[i] = static_cast<mpn::limb_t>(rng.next());
      }
      mb[n - 1u] |= 1u;
      mb[0] |= 1u;
      const usize ebits = mpn::bitlen(eb, n);
      const usize itch = mpn::powm_itch(n, n, ebits);
      require_true(itch + 1u < sizeof(sc) / sizeof(sc[0]));
      sc[itch] = canary;
      mpn::powm(rb, ab, n, eb, n, mb, n, sc);
      require_true(sc[itch] == canary);
    }
  }
  end_test_case();

  test_case("the pinned engines agree, on an odd modulus where both are legal");
  {

    constexpr usize CAP = 24u;
    static mpn::limb_t mb[CAP], ab[CAP], eb[CAP], r1[CAP], r2[CAP];
    static mpn::limb_t sc[64u * CAP + 32768u];
    prng rng(0x6C0FFEE5B1A5DA77ull);
    for ( usize n = 1; n <= CAP; ++n ) {
      for ( usize t = 0; t < 3; ++t ) {
        for ( usize i = 0; i < n; ++i ) {
          mb[i] = static_cast<mpn::limb_t>(rng.next());
          ab[i] = static_cast<mpn::limb_t>(rng.next());
          eb[i] = static_cast<mpn::limb_t>(rng.next());
        }
        mb[n - 1u] |= 1u;
        mb[0] |= 1u;
        if ( n == 1 && mb[0] == 1u ) mb[0] = 3u;

        mpn::powm_with<mpn::modalgo::redc>(r1, ab, n, eb, n, mb, n, sc);
        mpn::powm_with<mpn::modalgo::barrett>(r2, ab, n, eb, n, mb, n, sc);
        for ( usize i = 0; i < n; ++i ) require_true(r1[i] == r2[i]);
      }
    }
  }
  end_test_case();

  print("=== ARBINT MODULAR RIGOR PASSED ===");
  return 1;
}
