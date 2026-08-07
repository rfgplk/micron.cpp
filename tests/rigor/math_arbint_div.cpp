// math_arbint_div.cpp
// Division: the recursive divide-and-conquer tier against the schoolbook, and both against the
// invariant that actually defines division.

#include "../../src/math/arbint.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"
#include "../support/oracles.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using micron::math::arbuint;
using mtest::prng;
namespace mpn = micron::math::mpn;

#if defined(ARBINT_RIGOR_LITE) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_TRIALS = 3;
constexpr static const usize MAX_DN = 20;
constexpr static const usize MAX_NN = 64;
#else
constexpr static const usize N_TRIALS = 10;
constexpr static const usize MAX_DN = 40;
constexpr static const usize MAX_NN = 140;
#endif

constexpr static const usize CAP = 420;

static mpn::limb_t g_n[CAP], g_d[CAP];
static mpn::limb_t g_nw[CAP + 2], g_dw[CAP];
static mpn::limb_t g_nb[CAP + 2], g_nc[CAP + 2];
static mpn::limb_t g_q1[CAP], g_q2[CAP];
static mpn::limb_t g_sc[10 * CAP + 8192];

static mpn::limb_t g_nd[CAP + 2], g_q3[CAP];
static mpn::limb_t g_ip[CAP], g_prod[2 * CAP + 4], g_res[2 * CAP + 4];

static mpn::limb_t g_ip2[CAP];

static bool
newton_matches_basecase(const mpn::limb_t *dp, usize n)
{
  if ( n < 2u ) return true;
  mpn::invert_n_basecase(g_ip2, dp, n, g_sc);
  mpn::invert_n(g_ip, dp, n, g_sc);
  for ( usize i = 0; i < n; ++i )
    if ( g_ip[i] != g_ip2[i] ) return false;
  return true;
}

static bool
reciprocal_is_exact(const mpn::limb_t *dp, usize n)
{
  mpn::invert_n(g_ip, dp, n, g_sc);

  if ( n == 1 ) {
    mpn::mul_wide(dp[0], g_ip[0], g_prod[0], g_prod[1]);
  } else {
    mpn::mul(g_prod, dp, n, g_ip, n, g_sc);
  }

  if ( mpn::add_n(g_prod + n, g_prod + n, dp, n) != 0 ) return false;

  if ( mpn::neg(g_res, g_prod, 2u * n) == 0 ) return false;
  if ( !mpn::is_zero(g_res + n, n) ) return false;
  if ( mpn::is_zero(g_res, n) ) return false;
  return mpn::cmp(g_res, dp, n) <= 0;
}

static bool
all_tiers_agree(usize nn, usize dn)
{
  if ( g_d[dn - 1u] == 0 ) g_d[dn - 1u] = 1;

  const usize sh = mpn::limb_clz(g_d[dn - 1u]);
  if ( sh != 0 ) {
    (void)mpn::lshift(g_dw, g_d, dn, sh);
    g_nw[nn] = mpn::lshift(g_nw, g_n, nn, sh);
  } else {
    mpn::copyi(g_dw, g_d, dn);
    mpn::copyi(g_nw, g_n, nn);
    g_nw[nn] = 0;
  }
  const mpn::limb_t dinv = mpn::invert_pi1(g_dw[dn - 1u], g_dw[dn - 2u]);

  mpn::copyi(g_nb, g_nw, nn + 1u);
  (void)mpn::sbpi1_div_qr(g_q1, g_nb, nn + 1u, g_dw, dn, dinv);

  const usize qn = nn + 1u - dn;

  if ( dn >= 6u ) {
    mpn::copyi(g_nc, g_nw, nn + 1u);
    (void)mpn::dc_div_qr(g_q2, g_nc, nn + 1u, g_dw, dn, dinv, g_sc);
    for ( usize i = 0; i < qn; ++i )
      if ( g_q1[i] != g_q2[i] ) return false;
    for ( usize i = 0; i < dn; ++i )
      if ( g_nb[i] != g_nc[i] ) return false;
  }

  if ( qn == 0 ) return true;
  mpn::copyi(g_nd, g_nw, nn + 1u);
  (void)mpn::mu_div_qr(g_q3, g_nd, nn + 1u, g_dw, dn, g_sc);
  for ( usize i = 0; i < qn; ++i )
    if ( g_q1[i] != g_q3[i] ) return false;
  for ( usize i = 0; i < dn; ++i )
    if ( g_nb[i] != g_nd[i] ) return false;
  return true;
}

int
main()
{
  print("=== ARBINT DIVISION RIGOR ===");
  print("    div_dc threshold: ", static_cast<u64>(mpn::threshold::div_dc), " limbs");

  test_case("divide-and-conquer agrees with the schoolbook, limb for limb");
  {
    prng rng(0xC0FFEE1234567ull);
    for ( usize t = 0; t < N_TRIALS; ++t ) {

      for ( usize dn = 6; dn <= MAX_DN; ++dn ) {
        for ( usize nn = dn; nn <= MAX_NN; ++nn ) {
          for ( usize i = 0; i < nn; ++i ) g_n[i] = static_cast<mpn::limb_t>(rng.next());
          for ( usize i = 0; i < dn; ++i ) g_d[i] = static_cast<mpn::limb_t>(rng.next());
          switch ( t % 4u ) {
          case 1:
            for ( usize i = 0; i < dn; ++i ) g_d[i] = mpn::limb_max;
            break;
          case 2:
            for ( usize i = 0; i < dn; ++i ) g_d[i] = 0;
            g_d[dn - 1u] = 1;
            break;
          case 3:
            for ( usize i = 0; i < nn; ++i ) g_n[i] = mpn::limb_max;
            break;
          default:
            break;
          }
          require_true(all_tiers_agree(nn, dn));
        }
      }
    }
  }
  end_test_case();

  test_case("the recursion nests, not just bottoms out");
  {

    prng rng(0x5EEDF00D5EEDF00Dull);
    const usize dns[] = { 48u, 64u, 100u, 128u, 199u };
    for ( usize di = 0; di < 5; ++di ) {
      const usize dn = dns[di];
      const usize nns[] = { dn, 2u * dn, 2u * dn + 1u, 3u * dn, 3u * dn + 7u, 4u * dn - 1u, 400u };
      for ( usize ni = 0; ni < 7; ++ni ) {
        const usize nn = nns[ni];
        if ( nn < dn || nn > CAP - 2u ) continue;
        for ( usize i = 0; i < nn; ++i ) g_n[i] = static_cast<mpn::limb_t>(rng.next());
        for ( usize i = 0; i < dn; ++i ) g_d[i] = static_cast<mpn::limb_t>(rng.next());
        require_true(all_tiers_agree(nn, dn));
      }
    }
  }
  end_test_case();

  test_case("the n-limb reciprocal is exact at every width");
  {

    prng rng(0x1DEA5CAFF01DEA5Dull);
    const usize widths[] = { 1u,
                             2u,
                             3u,
                             4u,
                             5u,
                             6u,
                             7u,
                             8u,
                             15u,
                             16u,
                             17u,
                             31u,
                             32u,
                             33u,
                             mpn::threshold::inv_newton - 1u,
                             mpn::threshold::inv_newton,
                             mpn::threshold::inv_newton + 1u,
                             CAP / 2u };
    for ( usize wi = 0; wi < sizeof(widths) / sizeof(widths[0]); ++wi ) {
      const usize n = widths[wi];
      if ( n < 1u || n > CAP / 2u ) continue;
      for ( u32 p = 0; p < 5; ++p ) {
        for ( usize i = 0; i < n; ++i ) g_d[i] = static_cast<mpn::limb_t>(rng.next());
        switch ( p ) {
        case 1:
          for ( usize i = 0; i + 1u < n; ++i ) g_d[i] = 0;
          g_d[n - 1u] = mpn::limb_msb;
          break;
        case 2:
          for ( usize i = 0; i < n; ++i ) g_d[i] = mpn::limb_max;
          break;
        case 3:
          g_d[n - 1u] = mpn::limb_msb;
          break;
        case 4:
          for ( usize i = 0; i + 1u < n; ++i ) g_d[i] = 0;
          g_d[n - 1u] = mpn::limb_max;
          break;
        default:
          break;
        }
        g_d[n - 1u] |= mpn::limb_msb;
        require_true(reciprocal_is_exact(g_d, n));
        require_true(newton_matches_basecase(g_d, n));
      }
    }
  }
  end_test_case();

  test_case("the Newton reciprocal agrees with the basecase at every width and every seam");
  {

    prng rng(0x2E77011F1DE0FAC1ull);
    for ( usize n = 2; n <= CAP / 2u; ++n ) {
      for ( u32 p = 0; p < 4; ++p ) {
        for ( usize i = 0; i < n; ++i ) g_d[i] = static_cast<mpn::limb_t>(rng.next());
        switch ( p ) {
        case 1:
          for ( usize i = 0; i + 1u < n; ++i ) g_d[i] = 0;
          g_d[n - 1u] = mpn::limb_msb;
          break;
        case 2:
          for ( usize i = 0; i < n; ++i ) g_d[i] = mpn::limb_max;
          break;
        case 3:
          for ( usize i = 0; i + 1u < n; ++i ) g_d[i] = 0;
          g_d[n - 1u] = mpn::limb_max;
          break;
        default:
          break;
        }
        g_d[n - 1u] |= mpn::limb_msb;
        require_true(newton_matches_basecase(g_d, n));
        require_true(reciprocal_is_exact(g_d, n));
      }
    }
  }
  end_test_case();

  test_case("the reciprocal's scratch promise covers what the recursion carves");
  {
    constexpr mpn::limb_t canary = static_cast<mpn::limb_t>(0x5A5A5A5A5A5A5A5Aull);
    prng rng(0x1CEB00DA5CAFE011ull);
    for ( usize n = 2; n <= CAP / 2u; ++n ) {
      for ( usize i = 0; i < n; ++i ) g_d[i] = static_cast<mpn::limb_t>(rng.next());
      g_d[n - 1u] |= mpn::limb_msb;
      const usize itch = mpn::invert_n_itch(n);
      require_true(itch + 1u < sizeof(g_sc) / sizeof(g_sc[0]));
      g_sc[itch] = canary;
      mpn::invert_n(g_ip, g_d, n, g_sc);
      require_true(g_sc[itch] == canary);
    }
  }
  end_test_case();

  test_case("the Barrett rung agrees at every block shape");
  {

    prng rng(0x00BAD5EEDBAD5EEDull);
    const usize dns[] = { 2u, 3u, 6u, 7u, 15u, 16u, 33u, 64u, 100u };
    for ( usize di = 0; di < sizeof(dns) / sizeof(dns[0]); ++di ) {
      const usize dn = dns[di];
      for ( usize qn = 1u; qn <= 4u * dn + 3u; ++qn ) {
        const usize nn = qn + dn;
        if ( nn > CAP - 2u ) break;
        for ( usize i = 0; i < nn; ++i ) g_n[i] = static_cast<mpn::limb_t>(rng.next());
        for ( usize i = 0; i < dn; ++i ) g_d[i] = static_cast<mpn::limb_t>(rng.next());
        require_true(all_tiers_agree(nn, dn));
      }
    }
  }
  end_test_case();

  test_case("the tiered divrem answers the same whichever rung it lands on");
  {

    prng rng(0x7E57ED0DEC1DED77ull);
    for ( usize t = 0; t < N_TRIALS * 3u; ++t ) {
      const usize dn = 2u + rng.next_in(60);
      const usize nn = dn + 1u + rng.next_in(200);
      if ( nn > CAP - 2u ) continue;
      for ( usize i = 0; i < nn; ++i ) g_n[i] = static_cast<mpn::limb_t>(rng.next());
      for ( usize i = 0; i < dn; ++i ) g_d[i] = static_cast<mpn::limb_t>(rng.next());
      if ( g_d[dn - 1u] == 0 ) g_d[dn - 1u] = 1;

      const usize qn = nn - dn + 1u;
      mpn::divrem(g_q1, g_nb, g_n, nn, g_d, dn, g_sc);
      mpn::divrem_with<mpn::divalgo::sbpi1>(g_q2, g_nc, g_n, nn, g_d, dn, g_sc);
      for ( usize i = 0; i < qn; ++i ) require_true(g_q1[i] == g_q2[i]);
      for ( usize i = 0; i < dn; ++i ) require_true(g_nb[i] == g_nc[i]);

      mpn::divrem_with<mpn::divalgo::dc>(g_q2, g_nc, g_n, nn, g_d, dn, g_sc);
      for ( usize i = 0; i < qn; ++i ) require_true(g_q1[i] == g_q2[i]);
      for ( usize i = 0; i < dn; ++i ) require_true(g_nb[i] == g_nc[i]);

      mpn::divrem_with<mpn::divalgo::mu>(g_q3, g_nd, g_n, nn, g_d, dn, g_sc);
      for ( usize i = 0; i < qn; ++i ) require_true(g_q1[i] == g_q3[i]);
      for ( usize i = 0; i < dn; ++i ) require_true(g_nb[i] == g_nd[i]);
    }
  }
  end_test_case();

  test_case("the scratch promise covers what each rung actually carves");
  {

    prng rng(0x5CA7C4EDBEEF5CA7ull);
    constexpr mpn::limb_t canary = static_cast<mpn::limb_t>(0xA5A5A5A5A5A5A5A5ull);
    for ( usize t = 0; t < N_TRIALS * 3u; ++t ) {
      const usize dn = 2u + rng.next_in(40);
      const usize nn = dn + 1u + rng.next_in(150);
      if ( nn > CAP - 2u ) continue;
      for ( usize i = 0; i < nn; ++i ) g_n[i] = static_cast<mpn::limb_t>(rng.next());
      for ( usize i = 0; i < dn; ++i ) g_d[i] = static_cast<mpn::limb_t>(rng.next());
      if ( g_d[dn - 1u] == 0 ) g_d[dn - 1u] = 1;

      const usize itch = mpn::divrem_itch(nn, dn);
      require_true(itch + 1u < sizeof(g_sc) / sizeof(g_sc[0]));
      g_sc[itch] = canary;
      mpn::divrem(g_q1, g_nb, g_n, nn, g_d, dn, g_sc);
      require_true(g_sc[itch] == canary);

      const usize mitch = mpn::div_itch_with<mpn::divalgo::mu>(nn, dn);
      require_true(mitch + 1u < sizeof(g_sc) / sizeof(g_sc[0]));
      g_sc[mitch] = canary;
      mpn::divrem_with<mpn::divalgo::mu>(g_q3, g_nd, g_n, nn, g_d, dn, g_sc);
      require_true(g_sc[mitch] == canary);
    }
  }
  end_test_case();

#if defined(MICRON_ARBINT_MU_AUDIT)
  test_case("the Barrett estimate's correction loops stay bounded");
  {

    mpn::mu_corrections_down = 0;
    mpn::mu_corrections_up = 0;
    mpn::mu_saturations = 0;
    usize blocks = 0;

    prng rng(0xC0DEC0FFEEBADA55ull);
    for ( usize t = 0; t < 40; ++t ) {
      const usize dn = 2u + rng.next_in(60);
      const usize nn = dn + 1u + rng.next_in(200);
      if ( nn > CAP - 2u ) continue;
      for ( usize i = 0; i < nn; ++i ) g_n[i] = static_cast<mpn::limb_t>(rng.next());
      for ( usize i = 0; i < dn; ++i ) g_d[i] = static_cast<mpn::limb_t>(rng.next());
      g_d[dn - 1u] |= mpn::limb_msb;
      mpn::copyi(g_nd, g_n, nn);
      g_nd[nn] = 0;
      (void)mpn::mu_div_qr(g_q3, g_nd, nn, g_d, dn, g_sc);
      const usize in = mpn::mu_block_size(nn - dn, dn);
      blocks += (nn - dn) / in;
    }
    print("    mu blocks: ", static_cast<u64>(blocks), "  corrections down: ", static_cast<u64>(mpn::mu_corrections_down),
          "  up: ", static_cast<u64>(mpn::mu_corrections_up), "  saturations: ", static_cast<u64>(mpn::mu_saturations));

    require_true(mpn::mu_corrections_down + mpn::mu_corrections_up <= 8u * (blocks + 1u));
  }
  end_test_case();
#endif

  test_case("q*d + r == n and 0 <= r < d through the public type");
  {
    using U = arbuint<>;
    prng rng(0xABCDEF0123456789ull);
    for ( usize t = 0; t < N_TRIALS * 8u; ++t ) {

      const usize dwords = 8u + rng.next_in(56);
      const usize nwords = dwords + rng.next_in(120);
      U a, b;
      for ( usize i = 0; i < nwords; ++i ) {
        a <<= 64;
        a += U(rng.next());
      }
      for ( usize i = 0; i < dwords; ++i ) {
        b <<= 64;
        b += U(rng.next());
      }
      if ( b.is_zero() ) continue;

      const auto qr = micron::math::divmod(a, b);
      require_true(qr.quot * b + qr.rem == a);
      require_true(qr.rem < b);

      const U prod = a * b;
      require_true(prod / b == a);
      require_true((prod % b).is_zero());
    }
  }
  end_test_case();

  test_case("single-limb and degenerate divisors still take the right path");
  {
    using U = arbuint<>;
    prng rng(0xDEADBEEFCAFEBABEull);
    for ( usize t = 0; t < 200; ++t ) {
      U a;
      for ( usize i = 0; i < 1u + rng.next_in(40); ++i ) {
        a <<= 64;
        a += U(rng.next());
      }
      const u64 dv = 1u + rng.next();
      const U b(dv);
      const auto qr = micron::math::divmod(a, b);
      require_true(qr.quot * b + qr.rem == a);
      require_true(qr.rem < b);

      require_true((a / U(1u)) == a);
      require_true((a % U(1u)).is_zero());
      require_true((a / (a + U(1u))).is_zero());
      require_true((a % (a + U(1u))) == a);
      if ( !a.is_zero() ) {
        require_true((a / a) == U(1u));
        require_true((a % a).is_zero());
      }
    }
  }
  end_test_case();

  print("=== ARBINT DIVISION RIGOR PASSED ===");
  return 1;
}
