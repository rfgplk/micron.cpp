// math_arbint_mul.cpp
// Cross-tier differential for the multiplication ladder.
//
// Every tier that exists is forced onto identical inputs and must agree BIT FOR BIT — with each
// other, and with a schoolbook written inline here that shares no code with src/. The sweep is
// centred on the thresholds themselves (each one, and each one +/- 1), because a crossover is
// exactly where a dispatch bug hides: both neighbours are right and only the seam is wrong.

#include "../../src/math/arbint.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"
#include "../support/oracles.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using mtest::prng;
namespace mpn = micron::math::mpn;
namespace solver = micron::math::solver;

#if defined(ARBINT_RIGOR_LITE) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_TRIALS = 40;
#else
constexpr static const usize N_TRIALS = 300;
#endif

constexpr static const usize MAX_N = 72;

static mpn::limb_t g_a[MAX_N];
static mpn::limb_t g_b[MAX_N];
static mpn::limb_t g_ref[2 * MAX_N];
static mpn::limb_t g_bc[2 * MAX_N];
static mpn::limb_t g_cb[2 * MAX_N];
static mpn::limb_t g_kr[2 * MAX_N];
static mpn::limb_t g_tm[2 * MAX_N];
static mpn::limb_t g_t4[2 * MAX_N];

static mpn::limb_t g_sc[64 * MAX_N + 8192];

static void
ref_mul(mpn::limb_t *r, const mpn::limb_t *a, usize an, const mpn::limb_t *b, usize bn) noexcept
{
  for ( usize i = 0; i < an + bn; ++i ) r[i] = 0;
  for ( usize i = 0; i < an; ++i ) {
    mpn::limb_t cy = 0;
    for ( usize j = 0; j < bn; ++j ) {
      const mpn::dlimb_t t = static_cast<mpn::dlimb_t>(a[i]) * b[j] + r[i + j] + cy;
      r[i + j] = mpn::lo_half(t);
      cy = mpn::hi_half(t);
    }
    r[i + bn] = static_cast<mpn::limb_t>(r[i + bn] + cy);
  }
}

static bool
same(const mpn::limb_t *x, const mpn::limb_t *y, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( x[i] != y[i] ) return false;
  return true;
}

static void
fill(prng &rng, mpn::limb_t *p, usize n, u32 pattern) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    switch ( pattern ) {
    case 1:
      p[i] = mpn::limb_max;
      break;
    case 2:
      p[i] = (i + 1u == n) ? mpn::limb_msb : mpn::limb_t{ 0 };
      break;
    case 3:
      p[i] = mpn::limb_t{ 1 };
      break;
    default:
      p[i] = static_cast<mpn::limb_t>(rng.next());
      break;
    }
  }
  if ( n > 0 && p[n - 1u] == 0 ) p[n - 1u] = 1;
}

static usize
sweep_size(usize idx) noexcept
{
  const usize anchors[] = { 1u,
                            2u,
                            3u,
                            4u,
                            mpn::threshold::mul_comba,
                            mpn::threshold::mul_karatsuba,
                            mpn::threshold::mul_toom3,
                            mpn::threshold::mul_toom4,
                            mpn::threshold::sqr_karatsuba,
                            mpn::threshold::sqr_toom3,
                            mpn::threshold::sqr_toom4,
                            8u,
                            16u,
                            17u,
                            32u,
                            MAX_N };
  constexpr usize n_anchors = sizeof(anchors) / sizeof(anchors[0]);
  const usize a = anchors[(idx / 3u) % n_anchors];
  const usize off = idx % 3u;
  usize v = (off == 0u) ? (a > 1u ? a - 1u : 1u) : (off == 1u ? a : a + 1u);
  if ( v < 1u ) v = 1u;
  if ( v > MAX_N ) v = MAX_N;
  return v;
}

constexpr static usize n_sweep = 48;

int
main()
{
  print("=== ARBINT MUL LADDER DIFFERENTIAL ===");
  print("    tiers built through: toom-3");

  test_case("mul: every tier agrees with the reference at every threshold seam");
  {
    prng rng(0x9E3779B97F4A7C15ull);
    for ( usize si = 0; si < n_sweep; ++si ) {
      const usize an = sweep_size(si);
      for ( usize sj = 0; sj < n_sweep; ++sj ) {
        const usize bn = sweep_size(sj);
        if ( bn > an ) continue;
        for ( u32 p = 0; p < 4; ++p ) {
          fill(rng, g_a, an, p);
          fill(rng, g_b, bn, (p + 1u) % 4u);

          ref_mul(g_ref, g_a, an, g_b, bn);
          mpn::mul_with<mpn::algo::basecase>(g_bc, g_a, an, g_b, bn, g_sc);
          mpn::mul_with<mpn::algo::comba>(g_cb, g_a, an, g_b, bn, g_sc);
          mpn::mul_with<mpn::algo::karatsuba>(g_kr, g_a, an, g_b, bn, g_sc);
          mpn::mul_with<mpn::algo::toom3>(g_tm, g_a, an, g_b, bn, g_sc);

          require_true(same(g_ref, g_bc, an + bn));
          require_true(same(g_ref, g_cb, an + bn));
          require_true(same(g_ref, g_kr, an + bn));
          require_true(same(g_ref, g_tm, an + bn));

          mpn::mul_with<mpn::algo::toom4>(g_t4, g_a, an, g_b, bn, g_sc);
          require_true(same(g_ref, g_t4, an + bn));

          mpn::mul(g_bc, g_a, an, g_b, bn, g_sc);
          require_true(same(g_ref, g_bc, an + bn));
        }
      }
    }
  }
  end_test_case();

  test_case("sqr: every tier agrees, and with the general product of a by itself");
  {
    prng rng(0xC0FFEE0DDF00Dull);
    for ( usize si = 0; si < n_sweep; ++si ) {
      const usize n = sweep_size(si);
      for ( u32 p = 0; p < 4; ++p ) {
        fill(rng, g_a, n, p);

        ref_mul(g_ref, g_a, n, g_a, n);
        mpn::sqr_with<mpn::algo::basecase>(g_bc, g_a, n, g_sc);
        mpn::sqr_with<mpn::algo::comba>(g_cb, g_a, n, g_sc);
        mpn::sqr_with<mpn::algo::karatsuba>(g_kr, g_a, n, g_sc);
        mpn::sqr_with<mpn::algo::toom3>(g_tm, g_a, n, g_sc);
        mpn::sqr_with<mpn::algo::toom4>(g_t4, g_a, n, g_sc);

        require_true(same(g_ref, g_bc, 2u * n));
        require_true(same(g_ref, g_cb, 2u * n));
        require_true(same(g_ref, g_kr, 2u * n));

        require_true(same(g_ref, g_tm, 2u * n));
        require_true(same(g_ref, g_t4, 2u * n));

        mpn::sqr(g_bc, g_a, n, g_sc);
        require_true(same(g_ref, g_bc, 2u * n));
      }
    }
  }
  end_test_case();

  test_case("the unrolled fixed-width kernels match the runtime ones");
  {
    prng rng(0xF4F4F4F4F4ull);

    const auto check = [&rng]<usize AN, usize BN>() {
      for ( usize t = 0; t < 200; ++t ) {
        fill(rng, g_a, AN, static_cast<u32>(t % 4u));
        fill(rng, g_b, BN, static_cast<u32>((t + 1u) % 4u));
        ref_mul(g_ref, g_a, AN, g_b, BN);
        mpn::mul_comba_fixed<AN, BN>(g_cb, g_a, g_b);
        require_true(same(g_ref, g_cb, AN + BN));

        ref_mul(g_ref, g_a, AN, g_a, AN);
        mpn::sqr_comba_fixed<AN>(g_cb, g_a);
        require_true(same(g_ref, g_cb, 2u * AN));
      }
    };
    check.template operator()<1, 1>();
    check.template operator()<2, 2>();
    check.template operator()<3, 2>();
    check.template operator()<4, 4>();
    check.template operator()<8, 8>();
    check.template operator()<8, 3>();
    check.template operator()<16, 16>();
    check.template operator()<16, 5>();
  }
  end_test_case();

  test_case("a pinned solver on the type reaches the same value as the automatic one");
  {
    using Auto = micron::math::arbuint<>;
    using Base = micron::math::arbuint<0, solver::basecase>;
    using Comb = micron::math::arbuint<0, solver::comba>;

    prng rng(0x5EEDF00D5EEDF00Dull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {
      const usize an = 1u + rng.next_in(48);
      const usize bn = 1u + rng.next_in(48);
      Auto a0, b0;
      Base a1, b1;
      Comb a2, b2;
      for ( usize i = 0; i < an; ++i ) {
        const u64 w = rng.next();
        a0 <<= 64;
        a0 += Auto(w);
        a1 <<= 64;
        a1 += Base(w);
        a2 <<= 64;
        a2 += Comb(w);
      }
      for ( usize i = 0; i < bn; ++i ) {
        const u64 w = rng.next();
        b0 <<= 64;
        b0 += Auto(w);
        b1 <<= 64;
        b1 += Base(w);
        b2 <<= 64;
        b2 += Comb(w);
      }
      const Auto r0 = a0 * b0;
      const Base r1 = a1 * b1;
      const Comb r2 = a2 * b2;

      require(r0.size(), r1.size());
      require(r0.size(), r2.size());
      for ( usize i = 0; i < r0.size(); ++i ) {
        require_true(r0.limbs()[i] == r1.limbs()[i]);
        require_true(r0.limbs()[i] == r2.limbs()[i]);
      }
    }
  }
  end_test_case();

  test_case("the ladder never names a tier it cannot run");
  {

    static_assert(static_cast<u8>(mpn::sqr_tier_cap) <= static_cast<u8>(mpn::tiers_built));
    static_assert(static_cast<u8>(mpn::mul_tier_cap) <= static_cast<u8>(mpn::tiers_built));
    for ( usize n = 1; n <= 4096u; n = n * 3u / 2u + 1u ) {
      const mpn::algo picked = mpn::pick_mul(n, n);
      const mpn::algo run = mpn::clamp_algo(picked);
      require_true(static_cast<u8>(run) <= static_cast<u8>(mpn::mul_tier_cap));
      require_true(static_cast<u8>(run) <= static_cast<u8>(picked));

      const mpn::algo sq = mpn::clamp_to(mpn::pick_sqr(n), mpn::sqr_tier_cap);
      require_true(static_cast<u8>(sq) <= static_cast<u8>(mpn::sqr_tier_cap));
    }

    mpn::algo prev = mpn::pick_mul(1, 1);
    for ( usize n = 1; n <= 200000u; n = n * 2u ) {
      const mpn::algo cur = mpn::pick_mul(n, n);
      require_true(static_cast<u8>(cur) >= static_cast<u8>(prev));
      prev = cur;
    }

    require_true(mpn::pick_mul(100000u, 1u) == mpn::pick_mul(1u, 1u));
  }
  end_test_case();

  test_case("the scratch promise covers what the recursion actually carves");
  {

    struct need {
      static constexpr usize
      karat_n(usize n, usize cutoff)
      {
        usize total = 0;
        while ( n >= cutoff && n >= 2u ) {
          const usize k = (n + 1u) / 2u;
          total += 6u * k + 1u;
          n = k;
        }
        return total;
      }

      static constexpr usize
      mul(usize an, usize bn, usize cutoff)
      {
        if ( an < bn ) {
          const usize t = an;
          an = bn;
          bn = t;
        }
        if ( bn < cutoff || bn < 2u ) return 0;
        if ( an == bn ) return karat_n(an, cutoff);
        const usize take = an % bn;
        const usize below = karat_n(bn, cutoff) > mul(bn, take, cutoff) ? karat_n(bn, cutoff) : mul(bn, take, cutoff);
        return 2u * bn + below;
      }
    };

    for ( usize an = 1; an <= 64u; ++an )
      for ( usize bn = 1; bn <= an; ++bn ) {
        require_true(mpn::mul_itch_forced(an, bn) >= need::mul(an, bn, mpn::karatsuba_force));
        require_true(mpn::mul_itch(an, bn) >= need::mul(an, bn, mpn::threshold::mul_karatsuba));
      }

    require_true(mpn::mul_itch_forced(39u, 20u) >= need::mul(39u, 20u, mpn::karatsuba_force));

    for ( usize n = 3; n <= 64u; ++n )
      if ( mpn::toom3_applies(n, n) ) require_true(mpn::mul_itch_forced(n, n) >= mpn::toom3_itch(n, true));
  }
  end_test_case();

  print("=== ARBINT MUL LADDER DIFFERENTIAL PASSED ===");
  return 1;
}
