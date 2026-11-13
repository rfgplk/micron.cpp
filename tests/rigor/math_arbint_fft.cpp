//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/arbint.hpp"
#include "../snowball/snowball.hpp"
#include "../support/oracles.hpp"

namespace mpn = micron::math::mpn;
namespace solver = micron::math::solver;
using limbs = micron::vector<mpn::limb_t, micron::allocator_serial<>, false>;
constexpr usize guard = 3u;
constexpr mpn::limb_t canary = static_cast<mpn::limb_t>(0x9e3779b97f4a7c15ull);

constexpr bool
constant_product()
{
  using U = micron::math::arbuint<256, solver::fft>;
  U a(0x123456789abcdef0ull), b(0xfedcba9876543211ull);
  return static_cast<u64>(a * b) == 0x35a1df76f0d5adf0ull && static_cast<u64>(micron::math::sqr(a)) == 0xa5e20890f2a52100ull;
}

static_assert(constant_product());
static_assert(mpn::fft_itch(0u, 0u) == 0u);
static_assert(mpn::fft_itch(~usize(0), 1u) == ~usize(0));

void
same(const mpn::limb_t *a, const mpn::limb_t *b, usize n)
{
  for ( usize i = 0u; i < n; ++i ) sb::require(a[i], b[i]);
}

void
check_guard(const limbs &p, usize n)
{
  for ( usize i = 0u; i < guard; ++i ) {
    sb::require(p[i], canary);
    sb::require(p[guard + n + i], canary);
  }
}

void
fill(limbs &a, mtest::prng &rng, unsigned pattern)
{
  for ( usize i = 0u; i < a.size(); ++i ) {
    switch ( pattern ) {
    case 0:
      a[i] = 0u;
      break;
    case 1:
      a[i] = mpn::limb_max;
      break;
    case 2:
      a[i] = i == a.size() / 2u ? mpn::limb_msb : 0u;
      break;
    case 3:
      a[i] = (i & 1u) ? mpn::limb_max : 0u;
      break;
    case 4:
      a[i] = static_cast<mpn::limb_t>(0xaaaaaaaaaaaaaaaaull);
      break;
    default:
      a[i] = static_cast<mpn::limb_t>(rng.next());
      break;
    }
  }
}

// Independent base-2^16 schoolbook, including a different digit boundary from
// both the library limb kernels and its adaptive FFT packing.
void
oracle(mpn::limb_t *r, const limbs &a, const limbs &b)
{
  constexpr usize per = mpn::limb_bits / 16u;
  micron::vector<u32, micron::allocator_serial<>, false> words((a.size() + b.size()) * per);
  for ( usize i = 0u; i < a.size() * per; ++i ) {
    const u64 x = (a[i / per] >> (16u * (i % per))) & 65535u;
    u64 carry = 0u;
    for ( usize j = 0u; j < b.size() * per; ++j ) {
      const u64 y = (b[j / per] >> (16u * (j % per))) & 65535u;
      const u64 v = x * y + words[i + j] + carry;
      words[i + j] = u32(v & 65535u);
      carry = v >> 16u;
    }
    words[i + b.size() * per] = u32(carry);
  }
  for ( usize i = 0u; i < a.size() + b.size(); ++i ) {
    r[i] = 0u;
    for ( usize j = 0u; j < per; ++j ) r[i] |= mpn::limb_t(words[i * per + j]) << (16u * j);
  }
}

void
check(mtest::prng &rng, usize an, usize bn, unsigned pattern, bool square = false)
{
  limbs a(an), b(bn), ref(an + bn);
  fill(a, rng, pattern);
  fill(b, rng, pattern == 1u ? 1u : (pattern + 3u) % 6u);
  if ( square ) b = a;
  const limbs acopy(a), bcopy(b);
  if ( an <= 256u ) {
    oracle(ref.data(), a, b);
  } else {
    limbs work(mpn::mul_itch_with<mpn::algo::toom4>(an, bn));
    mpn::mul_with<mpn::algo::toom4>(ref.data(), a.data(), an, b.data(), bn, work.data());
  }
  for ( unsigned mode = 0u; mode < 3u; ++mode ) {
    const usize itch = mode == 2u ? (square ? mpn::sqr_itch(an) : mpn::mul_itch(an, bn)) : mpn::fft_itch(an, bn);
    limbs out(an + bn + 2u * guard, canary), work(itch + 2u * guard, canary);
    auto *r = out.data() + guard, *s = work.data() + guard;
    if ( mode == 0u ) {
#if defined(__micron_arbint_fft)
      // A correct fallback must not hide a broken FFT.
      const bool ok
          = square ? mpn::__fft_run<true>(r, a.data(), an, a.data(), an, s) : mpn::__fft_run<false>(r, a.data(), an, b.data(), bn, s);
      sb::require_true(ok);
#else
      mpn::mul_fft(r, a.data(), an, b.data(), bn, s);
#endif
    } else if ( mode == 1u ) {
      if ( square )
        mpn::sqr_with<mpn::algo::fft>(r, a.data(), an, s);
      else
        mpn::mul_with<mpn::algo::fft>(r, a.data(), an, b.data(), bn, s);
    } else {
      if ( square )
        mpn::sqr(r, a.data(), an, s);
      else
        mpn::mul(r, a.data(), an, b.data(), bn, s);
    }
    same(ref.data(), r, an + bn);
    same(a.data(), acopy.data(), an);
    same(b.data(), bcopy.data(), bn);
    check_guard(out, an + bn);
    check_guard(work, itch);
  }
}

void
check_reciprocal(mtest::prng &rng, usize n)
{
  limbs d(n), inverse(n), product(2u * n), remainder(2u * n);
  fill(d, rng, 5u);
  d[n - 1u] |= mpn::limb_msb;
  const usize itch = mpn::invert_n_itch(n);
  limbs work(itch + 2u * guard, canary);
  mpn::invert_n(inverse.data(), d.data(), n, work.data() + guard);
  check_guard(work, itch);

  // Independently check 0 < B^(2n) - D*(B^n+inverse) <= D using integer Toom.
  limbs exact(mpn::mul_itch_with<mpn::algo::toom4>(n, n));
  mpn::mul_with<mpn::algo::toom4>(product.data(), d.data(), n, inverse.data(), n, exact.data());
  sb::require(mpn::add_n(product.data() + n, product.data() + n, d.data(), n), mpn::limb_t(0u));
  sb::require_true(mpn::neg(remainder.data(), product.data(), 2u * n) != 0u);
  sb::require_true(mpn::is_zero(remainder.data() + n, n));
  sb::require_true(!mpn::is_zero(remainder.data(), n));
  sb::require_true(mpn::cmp(remainder.data(), d.data(), n) <= 0);
}

int
main()
{
  mtest::prng rng(0x243f6a8885a308d3ull);
  sb::test_case("FFT planner and cap scratch bounds include every smaller shape");
  for ( usize cap : { 16u, 65u, 192u } ) {
    const usize bound = mpn::mul_solver_cap_itch<solver::fft>(cap);
    const usize automatic = mpn::mul_solver_cap_itch<solver::automatic>(cap);
    for ( usize a = 1u; a <= cap; ++a ) {
      for ( usize b = 1u; b <= a; ++b ) {
        sb::require_true(mpn::fft_itch(a, b) <= bound);
        sb::require_true(mpn::mul_itch(a, b) <= automatic);
        const auto p = mpn::__fft_pick_shape(a, b).plan;
        sb::require_true(p.valid && (p.n & (p.n - 1u)) == 0u);
        sb::require_true(2u * p.digit_bits + mpn::__nuss_log2(p.n) <= 35u);
      }
      sb::require_true(mpn::sqr_itch(a) <= mpn::sqr_solver_cap_itch<solver::automatic>(cap));
    }
  }
  sb::end_test_case();

  sb::test_case("division and modular workspace bounds survive changing FFT shapes");
  for ( usize cap64 : { 8192u, 16385u, 38321u, 57345u, 90000u } ) {
    const usize cap = cap64 * (64u / mpn::limb_bits);
    const usize mulcap = mpn::mul_solver_cap_itch<solver::automatic>(cap);
    const usize sqrcap = mpn::sqr_solver_cap_itch<solver::automatic>(cap);
    const usize divcap = mpn::divrem_cap_itch(2u * cap, cap);
    const usize montcap = mpn::mont_op_itch(cap), barcap = mpn::barrett_op_itch(cap);
    for ( usize n = cap; n > 0u; n = n > cap / 19u ? n - cap / 19u : 0u ) {
      sb::require_true(mpn::mul_itch(n, n - 1u) <= mulcap);
      sb::require_true(mpn::sqr_itch(n) <= sqrcap);
      sb::require_true(mpn::divrem_itch(2u * cap, n) <= divcap);
      sb::require_true(mpn::mont_op_itch(n) <= montcap);
      sb::require_true(mpn::barrett_op_itch(n) <= barcap);
      sb::require_true(mpn::matrix22_mul_itch(n, n / 2u) <= mpn::matrix22_mul_itch(cap, cap));
    }
  }
  check_reciprocal(rng, 210u);
#if !defined(ARBINT_RIGOR_LITE)
  if constexpr ( mpn::threshold::fft_max_log >= 16u ) check_reciprocal(rng, 38321u * (64u / mpn::limb_bits));
#endif
  sb::end_test_case();

  sb::test_case("exact convolution, carry chains, zero padding, and scratch alignment");
  for ( usize n : { 1u, 2u, 3u, 7u, 15u, 16u, 17u, 31u, 32u, 33u, 63u, 64u, 65u, 127u } ) {
    for ( unsigned pattern = 0u; pattern < 6u; ++pattern ) {
      check(rng, n, n, pattern);
      check(rng, n, n, pattern, true);
    }
  }
#if defined(ARBINT_RIGOR_LITE)
  constexpr usize trials = 25u;
#else
  constexpr usize trials = 150u;
#endif
  for ( usize i = 0u; i < trials; ++i ) {
    const usize a = 1u + rng.next_in(192u), b = 1u + rng.next_in(a);
    check(rng, a, b, 5u);
  }
  for ( usize n : { 7u, 8u, 9u, 31u, 32u, 33u } ) check(rng, 5u * n, 5u, 1u);
  sb::end_test_case();

  sb::test_case("automatic FFT crossover and transform padding seams");
#if defined(__micron_arbint_fft)
  if constexpr ( MICRON_ARBINT_MUL_TIER_CAP >= 6 && mpn::threshold::fft_balanced )
    sb::require_true(mpn::pick_mul(mpn::__fft_block_limbs, mpn::__fft_block_limbs) == mpn::algo::fft);
  else
    sb::require_true(mpn::pick_mul(mpn::__fft_block_limbs, mpn::__fft_block_limbs) != mpn::algo::fft);
  if constexpr ( MICRON_ARBINT_SQR_TIER_CAP >= 6 && mpn::threshold::fft_balanced )
    sb::require_true(mpn::pick_sqr(mpn::__fft_block_limbs) == mpn::algo::fft);
  else
    sb::require_true(mpn::pick_sqr(mpn::__fft_block_limbs) != mpn::algo::fft);
#else
  sb::require_true(mpn::pick_mul(mpn::__fft_block_limbs, mpn::__fft_block_limbs) != mpn::algo::fft);
#endif
  if constexpr ( mpn::threshold::fft_max_log >= 16u ) {
    const usize centers[] = { mpn::threshold::mul_fft, mpn::threshold::sqr_fft, 1024u, 2816u, 5632u };
    for ( usize n : centers ) {
      for ( usize delta = 0u; delta < 3u; ++delta ) {
        check(rng, n - 1u + delta, n - 1u + delta, delta == 0u ? 1u : 5u);
        check(rng, n - 1u + delta, n - 1u + delta, delta == 0u ? 1u : 5u, true);
      }
    }
    check(rng, mpn::threshold::mul_fft + 1u, mpn::threshold::mul_fft, 5u);
  }
  sb::end_test_case();

#if !defined(ARBINT_RIGOR_LITE)
  sb::test_case("largest direct convolution and two-dimensional overlap-add");
  if constexpr ( mpn::threshold::fft_max_log >= 16u ) {
    const usize n = mpn::__fft_block_limbs;
    check(rng, n, n, 1u);
    check(rng, n + 1u, n + 1u, 5u);
    check(rng, n + 3u, n + 3u, 5u);
    check(rng, n + 1u, n + 1u, 1u, true);
  }
  sb::end_test_case();
#endif

  sb::test_case("pinned public signed, bounded, dynamic, and aliased products");
  using B = micron::math::arbuint<513, solver::fft>;
  using D = micron::math::arbuint<0, solver::fft>;
  B a = B::power_of_two(384u) + B(0xabcdef0123456789ull), b(0xfedcba9876543211ull);
  D da = D::power_of_two(384u) + D(0xabcdef0123456789ull), db(0xfedcba9876543211ull);
  const auto p = a * b;
  const auto dp = da * db;
  sb::require(p.size(), dp.size());
  same(p.limbs(), dp.limbs(), p.size());
  const auto square = micron::math::sqr(da);
  da *= da;
  sb::require_true(da == square);
  const auto trunc = a * a;
  const auto dsquare = micron::math::sqr(D::power_of_two(384u) + D(0xabcdef0123456789ull));
  for ( usize i = 0u; i < trunc.size(); ++i ) {
    const auto mask = i + 1u == B::cap_limbs ? mpn::limb_t(1u) : mpn::limb_max;
    sb::require(trunc.limbs()[i], dsquare.limbs()[i] & mask);
  }
  using I = micron::math::arbint<0, solver::fft>;
  const I neg(D::power_of_two(511u) + D(1u), true), pos(D(3u), false);
  sb::require_true((neg * pos).negative());
  sb::require_true(!(neg * neg).negative());
  sb::require_true((neg * I(0u)).is_zero());
  sb::end_test_case();

  sb::test_case("public automatic multiplication and aliasing at the actual FFT cutoff");
  if constexpr ( mpn::threshold::fft_max_log >= 16u ) {
    using U = micron::math::arbuint<>;
    usize n = mpn::threshold::mul_fft > mpn::threshold::sqr_fft ? mpn::threshold::mul_fft : mpn::threshold::sqr_fft;
#if defined(__micron_arbint_fft)
    if constexpr ( MICRON_ARBINT_MUL_TIER_CAP >= 6 && MICRON_ARBINT_SQR_TIER_CAP >= 6 && mpn::threshold::fft_balanced )
      while ( !mpn::__fft_auto_shape(n, n) ) ++n;
#endif
    const usize bits = n * mpn::limb_bits;
    U v = U::power_of_two(bits) - U(1u);
    const U expect = U::power_of_two(2u * bits) - U::power_of_two(bits + 1u) + U(1u);
    const U power = U::power_of_two(bits);
    sb::require_true(v * power == U::power_of_two(2u * bits) - power);
    sb::require_true(micron::math::sqr(v) == expect);
    const U copy(v);
    sb::require_true(v * copy == expect);
    v *= v;
    sb::require_true(v == expect);
  }
  sb::end_test_case();
  sb::print("=== ARBINT FFT PASSED ===");
  return 1;
}
