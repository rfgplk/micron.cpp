// math_arbint_nussbaumer.cpp
// Exact coefficient-transform and terminal-tier differential tests.

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

#if defined(ARBINT_RIGOR_LITE)
constexpr usize MAX_COEFF_N = 256u;
#else
constexpr usize MAX_COEFF_N = 1024u;
#endif
constexpr usize MAX_LIMBS = 192u;
constexpr usize GUARD = 8u;
constexpr mpn::limb_t CANARY = static_cast<mpn::limb_t>(0x9e3779b97f4a7c15ull);
constexpr usize MAX_COEFF_ITCH = mpn::__nuss_mul_negacyclic_itch(MAX_COEFF_N);
constexpr usize MAX_MUL_ITCH = mpn::nussbaumer_itch(MAX_LIMBS, MAX_LIMBS);
constexpr usize MAX_SQR_ITCH = mpn::sqr_nussbaumer_itch(MAX_LIMBS);
constexpr usize MAX_ITCH = MAX_MUL_ITCH > MAX_SQR_ITCH ? MAX_MUL_ITCH : MAX_SQR_ITCH;

static mpn::limb_t g_ca[2u * MAX_COEFF_N];
static mpn::limb_t g_cb[2u * MAX_COEFF_N];
static mpn::limb_t g_ca_copy[2u * MAX_COEFF_N];
static mpn::limb_t g_cb_copy[2u * MAX_COEFF_N];
static mpn::limb_t g_cr[2u * MAX_COEFF_N];
static mpn::limb_t g_coeff_scratch[GUARD + MAX_COEFF_ITCH + GUARD];
static i64 g_coeff_ref[MAX_COEFF_N];

static mpn::limb_t g_a[MAX_LIMBS];
static mpn::limb_t g_b[MAX_LIMBS];
static mpn::limb_t g_a_copy[MAX_LIMBS];
static mpn::limb_t g_b_copy[MAX_LIMBS];
static mpn::limb_t g_ref[2u * MAX_LIMBS];
static mpn::limb_t g_got[GUARD + 2u * MAX_LIMBS + GUARD];
static mpn::limb_t g_scratch[GUARD + MAX_ITCH + GUARD];

#if !defined(ARBINT_RIGOR_LITE)
constexpr usize HIGH_LIMBS = 50000u;
constexpr usize HIGH_MUL_NUSS_ITCH = mpn::nussbaumer_itch(HIGH_LIMBS, HIGH_LIMBS);
constexpr usize HIGH_SQR_NUSS_ITCH = mpn::sqr_nussbaumer_itch(HIGH_LIMBS);
constexpr usize HIGH_MUL_TOOM_ITCH = mpn::toom4_itch(HIGH_LIMBS, true);
constexpr usize HIGH_SQR_TOOM_ITCH = mpn::sqr_toom4_itch(HIGH_LIMBS, true);
constexpr usize HIGH_ITCH_0 = HIGH_MUL_NUSS_ITCH > HIGH_SQR_NUSS_ITCH ? HIGH_MUL_NUSS_ITCH : HIGH_SQR_NUSS_ITCH;
constexpr usize HIGH_ITCH_1 = HIGH_MUL_TOOM_ITCH > HIGH_SQR_TOOM_ITCH ? HIGH_MUL_TOOM_ITCH : HIGH_SQR_TOOM_ITCH;
constexpr usize HIGH_ITCH = HIGH_ITCH_0 > HIGH_ITCH_1 ? HIGH_ITCH_0 : HIGH_ITCH_1;

alignas(64) static mpn::limb_t g_high_a[HIGH_LIMBS];
alignas(64) static mpn::limb_t g_high_b[HIGH_LIMBS];
alignas(64) static mpn::limb_t g_high_ref[GUARD + 2u * HIGH_LIMBS + GUARD];
alignas(64) static mpn::limb_t g_high_got[GUARD + 2u * HIGH_LIMBS + GUARD];
alignas(64) static mpn::limb_t g_high_scratch[GUARD + HIGH_ITCH + GUARD];
#endif

[[nodiscard]] static constexpr mpn::__nuss_coeff
from_i64(i64 v) noexcept
{
  const bool neg = v < 0;
  const u64 mag = neg ? static_cast<u64>(-(v + 1)) + 1u : static_cast<u64>(v);
  mpn::__nuss_coeff r{ static_cast<mpn::limb_t>(mag), 0u };
  return neg ? mpn::__nuss_coeff_neg(r) : r;
}

[[nodiscard]] static i64
to_i64(mpn::__nuss_coeff v) noexcept
{
  const bool neg = mpn::__nuss_coeff_negative(v);
  if ( neg ) v = mpn::__nuss_coeff_neg(v);
  require_true(v.hi == 0u);
  const i64 mag = static_cast<i64>(v.lo);
  return neg ? -mag : mag;
}

[[nodiscard]] static mpn::__nuss_coeff
ref_coeff_product(mpn::__nuss_coeff a, mpn::__nuss_coeff b) noexcept
{
  const mpn::limb_t asign = static_cast<mpn::limb_t>(a.hi >> (mpn::limb_bits - 1u));
  const mpn::limb_t bsign = static_cast<mpn::limb_t>(b.hi >> (mpn::limb_bits - 1u));
  const mpn::limb_t amask = static_cast<mpn::limb_t>(0u - asign);
  const mpn::limb_t bmask = static_cast<mpn::limb_t>(0u - bsign);
  const mpn::limb_t amag = static_cast<mpn::limb_t>((a.lo ^ amask) + asign);
  const mpn::limb_t bmag = static_cast<mpn::limb_t>((b.lo ^ bmask) + bsign);
  mpn::__nuss_coeff r{};
  mpn::mul_wide(amag, bmag, r.lo, r.hi);
  const mpn::limb_t sign = asign ^ bsign;
  const mpn::limb_t mask = static_cast<mpn::limb_t>(0u - sign);
  r.lo ^= mask;
  r.hi ^= mask;
  const mpn::limb_t cy = mpn::addc(r.lo, sign, 0u, r.lo);
  (void)mpn::addc(r.hi, 0u, cy, r.hi);
  return r;
}

static void
ref_coeff_mul(i64 *rp, const mpn::limb_t *ap, const mpn::limb_t *bp, usize n) noexcept
{
  const auto a = mpn::__nuss_coeff_span(ap, n);
  const auto b = mpn::__nuss_coeff_span(bp, n);
  for ( usize i = 0u; i < n; ++i ) rp[i] = 0;
  for ( usize i = 0u; i < n; ++i ) {
    const i64 ai = to_i64(mpn::__nuss_coeff_load(a, i));
    for ( usize j = 0u; j < n; ++j ) {
      const i64 p = ai * to_i64(mpn::__nuss_coeff_load(b, j));
      const usize ij = i + j;
      if ( ij < n )
        rp[ij] += p;
      else
        rp[ij - n] -= p;
    }
  }
}

static void
ref_mul(mpn::limb_t *rp, const mpn::limb_t *ap, usize an, const mpn::limb_t *bp, usize bn) noexcept
{
  for ( usize i = 0u; i < an + bn; ++i ) rp[i] = 0u;
  for ( usize i = 0u; i < an; ++i ) {
    mpn::limb_t cy = 0u;
    for ( usize j = 0u; j < bn; ++j ) {
      const mpn::dlimb_t p = static_cast<mpn::dlimb_t>(ap[i]) * static_cast<mpn::dlimb_t>(bp[j]) + static_cast<mpn::dlimb_t>(rp[i + j])
                             + static_cast<mpn::dlimb_t>(cy);
      rp[i + j] = mpn::lo_half(p);
      cy = mpn::hi_half(p);
    }
    rp[i + bn] = static_cast<mpn::limb_t>(rp[i + bn] + cy);
  }
}

[[nodiscard]] static bool
same(const mpn::limb_t *ap, const mpn::limb_t *bp, usize n) noexcept
{
  for ( usize i = 0u; i < n; ++i )
    if ( ap[i] != bp[i] ) return false;
  return true;
}

static void
fill_limbs(prng &rng, mpn::limb_t *p, usize n, u32 pattern) noexcept
{
  for ( usize i = 0u; i < n; ++i ) {
    switch ( pattern ) {
    case 0u:
      p[i] = 0u;
      break;
    case 1u:
      p[i] = mpn::limb_max;
      break;
    case 2u:
      p[i] = i + 1u == n ? mpn::limb_msb : 0u;
      break;
    case 3u:
      p[i] = (i & 1u) != 0u ? static_cast<mpn::limb_t>(0xaaaaaaaaaaaaaaaaull) : static_cast<mpn::limb_t>(0x5555555555555555ull);
      break;
    case 4u:
      p[i] = i == n / 2u ? mpn::limb_msb : 0u;
      break;
    default:
      p[i] = static_cast<mpn::limb_t>(rng.next());
      break;
    }
  }
}

static void
guard(mpn::limb_t *p, usize n) noexcept
{
  for ( usize i = 0u; i < n; ++i ) p[i] = CANARY;
}

static void
check_guards(const mpn::limb_t *p, usize used) noexcept
{
  for ( usize i = 0u; i < GUARD; ++i ) {
    require_true(p[i] == CANARY);
    require_true(p[GUARD + used + i] == CANARY);
  }
}

static void
check_limb_case(prng &rng, usize an, usize bn, u32 pa, u32 pb) noexcept
{
  fill_limbs(rng, g_a, an, pa);
  fill_limbs(rng, g_b, bn, pb);
  mpn::copyi(g_a_copy, g_a, an);
  mpn::copyi(g_b_copy, g_b, bn);
  ref_mul(g_ref, g_a, an, g_b, bn);

  const usize itch = mpn::nussbaumer_itch(an, bn);
  guard(g_got, GUARD + an + bn + GUARD);
  guard(g_scratch, GUARD + itch + GUARD);
  mpn::mul_with<mpn::algo::nussbaumer>(g_got + GUARD, g_a, an, g_b, bn, g_scratch + GUARD);
  require_true(same(g_ref, g_got + GUARD, an + bn));
  require_true(same(g_a, g_a_copy, an));
  require_true(same(g_b, g_b_copy, bn));
  check_guards(g_got, an + bn);
  check_guards(g_scratch, itch);
}

constexpr bool
bounded_nussbaumer_constexpr() noexcept
{
  using U = micron::math::arbuint<256, solver::nussbaumer>;
  U a(0x123456789abcdef0ull);
  U b(0xfedcba9876543211ull);
  const U p = a * b;
  const U s = micron::math::sqr(a);
  return static_cast<u64>(p) == 0x35a1df76f0d5adf0ull && static_cast<u64>(s) == 0xa5e20890f2a52100ull;
}

static_assert(bounded_nussbaumer_constexpr(), "pinned Nussbaumer must remain constexpr-capable");

int
main()
{
  print("=== ARBINT NUSSBAUMER RIGOR ===");

  test_case("SIMD coefficient primitives match scalar two-limb arithmetic across vector tails");
  {
    prng rng(0x6a09e667f3bcc909ull);
    const usize sizes[] = { 1u, 2u, 3u, 4u, 7u, 8u, 15u, 16u, 31u, 33u, 65u };
    for ( usize si = 0u; si < sizeof(sizes) / sizeof(sizes[0]); ++si ) {
      const usize n = sizes[si];
      const auto a = mpn::__nuss_coeff_span(g_ca, n);
      const auto b = mpn::__nuss_coeff_span(g_cb, n);
      const auto sum = mpn::__nuss_coeff_span(g_ca_copy, n);
      const auto diff = mpn::__nuss_coeff_span(g_cb_copy, n);
      for ( usize i = 0u; i < n; ++i ) {
        const mpn::__nuss_coeff av{ static_cast<mpn::limb_t>(rng.next()), static_cast<mpn::limb_t>(rng.next()) };
        const mpn::__nuss_coeff bv{ static_cast<mpn::limb_t>(rng.next()), static_cast<mpn::limb_t>(rng.next()) };
        mpn::__nuss_coeff_store(a, i, av);
        mpn::__nuss_coeff_store(b, i, bv);
        mpn::__nuss_coeff_store(sum, i, mpn::__nuss_coeff_add(av, bv));
        mpn::__nuss_coeff_store(diff, i, mpn::__nuss_coeff_sub(av, bv));
      }
      mpn::__nuss_butterfly(a, b);
      require_true(same(g_ca, g_ca_copy, 2u * n));
      require_true(same(g_cb, g_cb_copy, 2u * n));
    }
  }
  end_test_case();

  test_case("out-of-place SIMD twiddles match the independent cycle permutation");
  {
    prng rng(0xbb67ae8584caa73bull);
    for ( usize n = 1u; n <= 64u; n <<= 1u ) {
      const auto a = mpn::__nuss_coeff_span(g_ca, n);
      for ( usize i = 0u; i < n; ++i )
        mpn::__nuss_coeff_store(a, i, { static_cast<mpn::limb_t>(rng.next()), static_cast<mpn::limb_t>(rng.next()) });
      const usize exponents[] = { 0u, 1u, n - 1u, n, n + 1u, 2u * n - 1u };
      for ( usize ei = 0u; ei < sizeof(exponents) / sizeof(exponents[0]); ++ei ) {
        mpn::copyi(g_ca_copy, g_ca, 2u * n);
        mpn::__nuss_twiddle(mpn::__nuss_coeff_span(g_ca_copy, n), exponents[ei]);
        mpn::__nuss_twiddle_copy(mpn::__nuss_coeff_span(g_cr, n), mpn::__nuss_as_const(mpn::__nuss_coeff_span(g_ca, n)), exponents[ei]);
        require_true(same(g_cr, g_ca_copy, 2u * n));
      }
    }
  }
  end_test_case();

  test_case("SIMD transpose networks and scalar tails preserve both coefficient planes");
  {
    const usize shapes[][2] = { { 1u, 1u }, { 2u, 3u }, { 3u, 5u }, { 4u, 4u }, { 7u, 11u }, { 11u, 7u }, { 8u, 8u }, { 16u, 16u } };
    for ( usize si = 0u; si < sizeof(shapes) / sizeof(shapes[0]); ++si ) {
      const usize rows = shapes[si][0];
      const usize cols = shapes[si][1];
      const usize n = rows * cols;
      const auto a = mpn::__nuss_coeff_span(g_ca, n);
      for ( usize i = 0u; i < n; ++i )
        mpn::__nuss_coeff_store(a, i, { static_cast<mpn::limb_t>(3u * i + 1u), static_cast<mpn::limb_t>(~i) });
      mpn::copyi(g_ca_copy, g_ca, 2u * n);
      guard(g_coeff_scratch, GUARD + 2u * n + GUARD);
      const auto out = mpn::__nuss_coeff_span(g_coeff_scratch + GUARD, n);
      mpn::__nuss_transpose(out, mpn::__nuss_as_const(a), rows, cols);
      for ( usize row = 0u; row < rows; ++row ) {
        for ( usize col = 0u; col < cols; ++col ) {
          const mpn::__nuss_coeff got = mpn::__nuss_coeff_load(out, col * rows + row);
          const mpn::__nuss_coeff expected = mpn::__nuss_coeff_load(a, row * cols + col);
          require_true(got.lo == expected.lo && got.hi == expected.hi);
        }
      }
      require_true(same(g_ca, g_ca_copy, 2u * n));
      check_guards(g_coeff_scratch, 2u * n);
    }
  }
  end_test_case();

  test_case("SIMD exact division matches scalar signed shifts for true and false inputs");
  {
    prng rng(0x3c6ef372fe94f82bull);
    const usize sizes[] = { 1u, 3u, 8u, 17u, 65u };
    const usize shifts[] = { 1u, 5u, mpn::limb_bits / 2u, mpn::limb_bits - 1u };
    for ( usize si = 0u; si < sizeof(sizes) / sizeof(sizes[0]); ++si ) {
      const usize n = sizes[si];
      for ( usize qi = 0u; qi < sizeof(shifts) / sizeof(shifts[0]); ++qi ) {
        const usize shift = shifts[qi];
        const mpn::limb_t mask = static_cast<mpn::limb_t>((mpn::limb_t{ 1 } << shift) - 1u);
        for ( usize exact_case = 0u; exact_case < 2u; ++exact_case ) {
          const auto a = mpn::__nuss_coeff_span(g_ca, n);
          const auto ref = mpn::__nuss_coeff_span(g_ca_copy, n);
          for ( usize i = 0u; i < n; ++i ) {
            const mpn::__nuss_coeff v{ static_cast<mpn::limb_t>(rng.next()) & ~mask, static_cast<mpn::limb_t>(rng.next()) };
            mpn::__nuss_coeff_store(a, i, v);
            mpn::__nuss_coeff_store(ref, i, v);
          }
          if ( exact_case == 0u ) {
            a.lo[n / 2u] |= 1u;
            ref.lo[n / 2u] |= 1u;
          }
          bool expected_exact = true;
          for ( usize i = 0u; i < n; ++i ) {
            mpn::__nuss_coeff v = mpn::__nuss_coeff_load(ref, i);
            expected_exact = mpn::__nuss_coeff_divexact_pow2(v, shift) && expected_exact;
            mpn::__nuss_coeff_store(ref, i, v);
          }
          require_true(mpn::__nuss_divexact_coeffs(a, shift) == expected_exact);
          require_true(same(g_ca, g_ca_copy, 2u * n));
        }
      }
    }
  }
  end_test_case();

  test_case("signed widening leaves agree with a magnitude oracle at the planner bound");
  {
    const mpn::limb_t bound = static_cast<mpn::limb_t>((mpn::limb_t{ 1 } << (mpn::limb_bits - 2u)) - 1u);
    const mpn::__nuss_coeff values[]
        = { { 0u, 0u }, { 1u, 0u }, { bound, 0u }, mpn::__nuss_coeff_neg({ 1u, 0u }), mpn::__nuss_coeff_neg({ bound, 0u }) };
    for ( usize i = 0u; i < sizeof(values) / sizeof(values[0]); ++i ) {
      for ( usize j = 0u; j < sizeof(values) / sizeof(values[0]); ++j ) {
        const mpn::__nuss_coeff got = mpn::__nuss_coeff_mul(values[i], values[j]);
        const mpn::__nuss_coeff expected = ref_coeff_product(values[i], values[j]);
        require_true(got.lo == expected.lo && got.hi == expected.hi);
      }
    }
  }
  end_test_case();

  test_case("coefficient negacyclic multiply and square match a signed quadratic oracle");
  {
    prng rng(0x4e5553534241554dull);
    for ( usize n = 2u; n <= MAX_COEFF_N; n <<= 1u ) {
      const usize itch = mpn::__nuss_mul_negacyclic_itch(n);
      const usize trials = n <= 256u ? 12u : 2u;
      for ( usize trial = 0u; trial < trials; ++trial ) {
        const auto ca = mpn::__nuss_coeff_span(g_ca, n);
        const auto cb = mpn::__nuss_coeff_span(g_cb, n);
        const auto cr = mpn::__nuss_coeff_span(g_cr, n);
        for ( usize i = 0u; i < n; ++i ) {
          const i64 av = static_cast<i64>(rng.next_in(65u)) - 32;
          const i64 bv = static_cast<i64>(rng.next_in(65u)) - 32;
          mpn::__nuss_coeff_store(ca, i, from_i64(av));
          mpn::__nuss_coeff_store(cb, i, from_i64(bv));
        }
        mpn::copyi(g_ca_copy, g_ca, 2u * n);
        mpn::copyi(g_cb_copy, g_cb, 2u * n);
        ref_coeff_mul(g_coeff_ref, g_ca, g_cb, n);
        guard(g_coeff_scratch, GUARD + itch + GUARD);
        require_true(mpn::__nuss_mul_negacyclic(g_cr, g_ca, g_cb, n, g_coeff_scratch + GUARD));
        for ( usize i = 0u; i < n; ++i ) require(to_i64(mpn::__nuss_coeff_load(cr, i)), g_coeff_ref[i]);
        require_true(same(g_ca, g_ca_copy, 2u * n));
        require_true(same(g_cb, g_cb_copy, 2u * n));
        check_guards(g_coeff_scratch, itch);

        ref_coeff_mul(g_coeff_ref, g_ca, g_ca, n);
        const usize sitch = mpn::__nuss_sqr_negacyclic_itch(n);
        guard(g_coeff_scratch, GUARD + sitch + GUARD);
        require_true(mpn::__nuss_sqr_negacyclic(g_cr, g_ca, n, g_coeff_scratch + GUARD));
        for ( usize i = 0u; i < n; ++i ) require(to_i64(mpn::__nuss_coeff_load(cr, i)), g_coeff_ref[i]);
        check_guards(g_coeff_scratch, sitch);
      }
    }
  }
  end_test_case();

#if !defined(ARBINT_RIGOR_LITE)
  test_case("large blocked transforms agree with Toom immediately after a planner seam");
  {
    prng rng(0x510e527fade682d1ull);
    fill_limbs(rng, g_high_a, HIGH_LIMBS, 5u);
    fill_limbs(rng, g_high_b, HIGH_LIMBS, 5u);
    guard(g_high_ref, GUARD + 2u * HIGH_LIMBS + GUARD);
    guard(g_high_got, GUARD + 2u * HIGH_LIMBS + GUARD);
    guard(g_high_scratch, GUARD + HIGH_ITCH + GUARD);

    mpn::mul_with<mpn::algo::toom4>(g_high_ref + GUARD, g_high_a, HIGH_LIMBS, g_high_b, HIGH_LIMBS, g_high_scratch + GUARD);
    mpn::mul_with<mpn::algo::nussbaumer>(g_high_got + GUARD, g_high_a, HIGH_LIMBS, g_high_b, HIGH_LIMBS, g_high_scratch + GUARD);
    require_true(same(g_high_ref + GUARD, g_high_got + GUARD, 2u * HIGH_LIMBS));
    check_guards(g_high_ref, 2u * HIGH_LIMBS);
    check_guards(g_high_got, 2u * HIGH_LIMBS);

    mpn::sqr_with<mpn::algo::toom4>(g_high_ref + GUARD, g_high_a, HIGH_LIMBS, g_high_scratch + GUARD);
    mpn::sqr_with<mpn::algo::nussbaumer>(g_high_got + GUARD, g_high_a, HIGH_LIMBS, g_high_scratch + GUARD);
    require_true(same(g_high_ref + GUARD, g_high_got + GUARD, 2u * HIGH_LIMBS));
    check_guards(g_high_ref, 2u * HIGH_LIMBS);
    check_guards(g_high_got, 2u * HIGH_LIMBS);
    check_guards(g_high_scratch, HIGH_ITCH);
  }
  end_test_case();
#endif

  test_case("planner keeps the digit and transform bounds exact at every seam");
  {
    usize previous_n = 0u;
    for ( usize an = 1u; an <= MAX_LIMBS; ++an ) {
      for ( usize bn = 1u; bn <= an; ++bn ) {
        const mpn::__nuss_plan p = mpn::__nuss_make_plan(an, bn);
        if ( !p.valid ) continue;
        require_true(p.digit_bits + p.log_n <= mpn::limb_bits - 2u);
        require_true((p.n & (p.n - 1u)) == 0u);
        require_true(p.a_digits + p.b_digits - 1u <= p.n);
        require_true(p.a_digits * p.digit_bits >= an * mpn::limb_bits);
        require_true(p.b_digits * p.digit_bits >= bn * mpn::limb_bits);
        if ( an == bn ) {
          require_true(p.n >= previous_n);
          previous_n = p.n;
        }
      }
    }
    require_true(mpn::__nuss_max_balanced_limbs() >= 1u);
  }
  end_test_case();

  test_case("a cap-sized solver buffer bounds every smaller multiplication shape");
  {
    constexpr usize cap = MAX_LIMBS;
    constexpr usize auto_cap = mpn::mul_solver_cap_itch<solver::automatic>(cap);
    constexpr usize nuss_cap = mpn::mul_solver_cap_itch<solver::nussbaumer>(cap);
    constexpr usize karat_cap = mpn::mul_solver_cap_itch<solver::karatsuba>(cap);
    constexpr usize toom_cap = mpn::mul_solver_cap_itch<solver::toom>(cap);
    constexpr usize auto_sqr_cap = mpn::sqr_solver_cap_itch<solver::automatic>(cap);
    constexpr usize nuss_sqr_cap = mpn::sqr_solver_cap_itch<solver::nussbaumer>(cap);
    for ( usize an = 1u; an <= cap; ++an ) {
      for ( usize bn = 1u; bn <= an; ++bn ) {
        require_true(mpn::mul_solver_itch<solver::automatic>(an, bn) <= auto_cap);
        require_true(mpn::mul_solver_itch<solver::nussbaumer>(an, bn) <= nuss_cap);
        require_true(mpn::mul_solver_itch<solver::karatsuba>(an, bn) <= karat_cap);
        require_true(mpn::mul_solver_itch<solver::toom>(an, bn) <= toom_cap);
      }
      require_true(mpn::sqr_solver_itch<solver::automatic>(an) <= auto_sqr_cap);
      require_true(mpn::sqr_solver_itch<solver::nussbaumer>(an) <= nuss_sqr_cap);
    }
  }
  end_test_case();

  test_case("forced limb multiplication crosses digit, transform, carry, ratio, and tile seams");
  {
    prng rng(0x243f6a8885a308d3ull);
    const usize sizes[] = { 1u, 2u, 3u, 4u, 7u, 8u, 9u, 15u, 16u, 17u, 23u, 24u, 25u, 31u, 32u, 33u, 47u, 63u, 64u, 65u };
    for ( usize si = 0u; si < sizeof(sizes) / sizeof(sizes[0]); ++si ) {
      const usize n = sizes[si];
      for ( u32 pattern = 0u; pattern < 6u; ++pattern ) check_limb_case(rng, n, n, pattern, (pattern + 2u) % 6u);
    }

    const usize ratios[] = { 1u, 2u, 7u, 8u, 9u, 32u };
    for ( usize i = 0u; i < sizeof(ratios) / sizeof(ratios[0]); ++i ) {
      const usize bn = 5u;
      const usize an = ratios[i] * bn;
      check_limb_case(rng, an, bn, 1u, 5u);
      check_limb_case(rng, an, bn, 3u, 1u);
    }
    check_limb_case(rng, 127u, 97u, 5u, 5u);
    check_limb_case(rng, 192u, 113u, 1u, 3u);
  }
  end_test_case();

  test_case("the dedicated square path agrees with the independent product oracle");
  {
    prng rng(0x13198a2e03707344ull);
    const usize sizes[] = { 1u, 2u, 4u, 8u, 15u, 16u, 17u, 31u, 32u, 33u, 63u, 64u, 65u, 97u, 127u, 192u };
    for ( usize si = 0u; si < sizeof(sizes) / sizeof(sizes[0]); ++si ) {
      const usize n = sizes[si];
      for ( u32 pattern = 0u; pattern < 6u; ++pattern ) {
        fill_limbs(rng, g_a, n, pattern);
        mpn::copyi(g_a_copy, g_a, n);
        ref_mul(g_ref, g_a, n, g_a, n);
        const usize itch = mpn::sqr_nussbaumer_itch(n);
        guard(g_got, GUARD + 2u * n + GUARD);
        guard(g_scratch, GUARD + itch + GUARD);
        mpn::sqr_with<mpn::algo::nussbaumer>(g_got + GUARD, g_a, n, g_scratch + GUARD);
        require_true(same(g_ref, g_got + GUARD, 2u * n));
        require_true(same(g_a, g_a_copy, n));
        check_guards(g_got, 2u * n);
        check_guards(g_scratch, itch);
      }
    }
  }
  end_test_case();

  test_case("bounded and dynamic pinned solvers normalize the same terminal-tier result");
  {
    using B = micron::math::arbuint<1024, solver::nussbaumer>;
    using D = micron::math::arbuint<0, solver::nussbaumer>;
    B ba = B::power_of_two(511u) + B(0xffffffffffffffffull);
    B bb = B::power_of_two(383u) + B(0x5555555555555555ull);
    D da = D::power_of_two(511u) + D(0xffffffffffffffffull);
    D db = D::power_of_two(383u) + D(0x5555555555555555ull);
    const B bp = ba * bb;
    const D dp = da * db;
    require(bp.size(), dp.size());
    require_true(same(bp.limbs(), dp.limbs(), bp.size()));
    require_true((ba * ba).size() == micron::math::sqr(ba).size());
  }
  end_test_case();

  print("=== ARBINT NUSSBAUMER PASSED ===");
  return 1;
}
