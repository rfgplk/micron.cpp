// math_arbint_kernels.cpp

// What the sweep is shaped around:
//
//   .. n from 1 up, so every residue class mod the block width is hit and every entry into and out
//      of the block loop is exercised — a kernel that consumes 4 limbs at a time and finishes the
//      rest in C has a seam at each of those, and a seam is where an off-by-one lives.
//   .. n straddling kern_min_limbs, below which the block loop is deliberately not taken at all.
//   .. rp pre-loaded with all-ones, because addmul_1's carry-out and submul_1's borrow-out are only
//      reachable when the destination limb is at its maximum. random data essentially never gets
//      there, which is why the patterns are enumerated rather than sampled.
//   .. multipliers 0, 1, 2, limb_max: limb_max is the one that drives the high half to 2^w - 2, the
//      exact value the "cannot overflow" argument in every carry fold depends on.
//   .. rp == ap for add_n/sub_n, which the mpn contract permits and the kernels must honour.
//
// Seeds are fixed hex literals: a failure here reproduces on the next run and on every arch.

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

#if defined(__micron_arbint_kern_addmul_1)
#define ARBINT_KERNELS_MUL 1
#endif
#if defined(__micron_arbint_kern_add_n)
#define ARBINT_KERNELS_AORS 1
#endif

#if defined(ARBINT_KERNELS_MUL) || defined(ARBINT_KERNELS_AORS)
constexpr static const usize KERN_MIN = mpn::__kern::kern_min_limbs;
#else
constexpr static const usize KERN_MIN = 0;
#endif

#if defined(ARBINT_RIGOR_LITE) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize MAX_N = 96;
constexpr static const usize N_TRIALS = 3;
#else
constexpr static const usize MAX_N = 200;
constexpr static const usize N_TRIALS = 6;
#endif

constexpr static const usize PAD = MAX_N + 4;

static mpn::limb_t g_a[PAD];
static mpn::limb_t g_b[PAD];
static mpn::limb_t g_k[PAD];
static mpn::limb_t g_p[PAD];

static void
fill(mpn::limb_t *p, usize n, usize pattern, prng &rng) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    switch ( pattern ) {
    case 1:
      p[i] = mpn::limb_max;
      break;
    case 2:
      p[i] = 0;
      break;
    case 3:
      p[i] = (i & 1u) ? mpn::limb_max : 0;
      break;
    case 4:
      p[i] = mpn::limb_msb;
      break;
    default:
      p[i] = static_cast<mpn::limb_t>(rng.next());
      break;
    }
  }
}

[[nodiscard]] static bool
same(const mpn::limb_t *x, const mpn::limb_t *y, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( x[i] != y[i] ) return false;
  return true;
}

static mpn::limb_t
pick_mul_by(usize which, prng &rng) noexcept
{
  switch ( which ) {
  case 1:
    return 0;
  case 2:
    return 1;
  case 3:
    return 2;
  case 4:
    return mpn::limb_max;
  case 5:
    return mpn::limb_msb;
  default:
    return static_cast<mpn::limb_t>(rng.next());
  }
}

int
main()
{
  print("=== ARBINT KERNEL DIFFERENTIAL ===");
#if defined(MICRON_ARBINT_NO_ASM)
  print("built with MICRON_ARBINT_NO_ASM: both sides are the portable loop, so this run proves the");
  print("fallback still compiles and still agrees with itself -- which is the point of the cell.");
#endif
  print("    limb bits: ", static_cast<u64>(mpn::limb_bits), "   kernel minimum: ", static_cast<u64>(KERN_MIN), " limbs");

#if defined(ARBINT_KERNELS_MUL)
  test_case("mul_1 / addmul_1 / submul_1 agree with the portable loops");
  {
    prng rng(0x9E3779B97F4A7C15ull);
    for ( usize n = 1; n <= MAX_N; ++n ) {
      for ( usize pat = 0; pat < 5; ++pat ) {
        for ( usize mw = 0; mw < 6; ++mw ) {
          for ( usize t = 0; t < N_TRIALS; ++t ) {
            fill(g_a, n, pat, rng);
            const mpn::limb_t b = pick_mul_by(mw, rng);

            mpn::zero(g_k, PAD);
            mpn::zero(g_p, PAD);
            const mpn::limb_t c1 = mpn::__kern::mul_1(g_k, g_a, n, b);
            const mpn::limb_t c2 = mpn::__portable::mul_1(g_p, g_a, n, b);
            require_true(c1 == c2);
            require_true(same(g_k, g_p, PAD));

            fill(g_k, n, (pat + 1u) % 5u, rng);
            mpn::copyi(g_p, g_k, n);
            g_k[n] = g_p[n] = 0;
            const mpn::limb_t c3 = mpn::__kern::addmul_1(g_k, g_a, n, b);
            const mpn::limb_t c4 = mpn::__portable::addmul_1(g_p, g_a, n, b);
            require_true(c3 == c4);
            require_true(same(g_k, g_p, n + 1u));

            fill(g_k, n, (pat + 1u) % 5u, rng);
            mpn::copyi(g_p, g_k, n);
            g_k[n] = g_p[n] = 0;
            const mpn::limb_t c5 = mpn::__kern::submul_1(g_k, g_a, n, b);
            const mpn::limb_t c6 = mpn::__portable::submul_1(g_p, g_a, n, b);
            require_true(c5 == c6);
            require_true(same(g_k, g_p, n + 1u));
          }
        }
      }
    }
  }
  end_test_case();
#endif

#if defined(ARBINT_KERNELS_AORS)
  test_case("add_n / sub_n agree with the portable loops, including in place");
  {
    prng rng(0xC0FFEE0DDF00Dull);
    for ( usize n = 1; n <= MAX_N; ++n ) {
      for ( usize pat = 0; pat < 5; ++pat ) {
        for ( usize t = 0; t < N_TRIALS; ++t ) {
          fill(g_a, n, pat, rng);
          fill(g_b, n, (pat + 2u) % 5u, rng);

          mpn::zero(g_k, PAD);
          mpn::zero(g_p, PAD);
          require_true(mpn::__kern::add_n(g_k, g_a, g_b, n) == mpn::__portable::add_n(g_p, g_a, g_b, n));
          require_true(same(g_k, g_p, PAD));

          mpn::zero(g_k, PAD);
          mpn::zero(g_p, PAD);
          require_true(mpn::__kern::sub_n(g_k, g_a, g_b, n) == mpn::__portable::sub_n(g_p, g_a, g_b, n));
          require_true(same(g_k, g_p, PAD));

          mpn::copyi(g_k, g_a, n);
          mpn::copyi(g_p, g_a, n);
          require_true(mpn::__kern::add_n(g_k, g_k, g_b, n) == mpn::__portable::add_n(g_p, g_p, g_b, n));
          require_true(same(g_k, g_p, n));

          mpn::copyi(g_k, g_a, n);
          mpn::copyi(g_p, g_a, n);
          require_true(mpn::__kern::sub_n(g_k, g_k, g_b, n) == mpn::__portable::sub_n(g_p, g_p, g_b, n));
          require_true(same(g_k, g_p, n));
        }
      }
    }
  }
  end_test_case();
#endif

  test_case("the public bridges route to the kernel and still answer the same");
  {

    prng rng(0xF4F4F4F4F4ull);
    for ( usize n = 1; n <= MAX_N; ++n ) {
      fill(g_a, n, n % 5u, rng);
      fill(g_b, n, (n + 3u) % 5u, rng);
      const mpn::limb_t b = pick_mul_by(n % 6u, rng);

      mpn::zero(g_k, PAD);
      mpn::zero(g_p, PAD);
      require_true(mpn::mul_1(g_k, g_a, n, b) == mpn::__portable::mul_1(g_p, g_a, n, b));
      require_true(same(g_k, g_p, PAD));

      mpn::zero(g_k, PAD);
      mpn::zero(g_p, PAD);
      require_true(mpn::add_n(g_k, g_a, g_b, n) == mpn::__portable::add_n(g_p, g_a, g_b, n));
      require_true(same(g_k, g_p, PAD));

      mpn::zero(g_k, PAD);
      mpn::zero(g_p, PAD);
      require_true(mpn::sub_n(g_k, g_a, g_b, n) == mpn::__portable::sub_n(g_p, g_a, g_b, n));
      require_true(same(g_k, g_p, PAD));

      fill(g_k, n, 1u, rng);
      mpn::copyi(g_p, g_k, n);
      g_k[n] = g_p[n] = 0;
      require_true(mpn::addmul_1(g_k, g_a, n, b) == mpn::__portable::addmul_1(g_p, g_a, n, b));
      require_true(same(g_k, g_p, n + 1u));

      fill(g_k, n, 1u, rng);
      mpn::copyi(g_p, g_k, n);
      g_k[n] = g_p[n] = 0;
      require_true(mpn::submul_1(g_k, g_a, n, b) == mpn::__portable::submul_1(g_p, g_a, n, b));
      require_true(same(g_k, g_p, n + 1u));
    }
  }
  end_test_case();

  test_case("shifts agree, at every count and in place");
  {
    prng rng(0x5EEDF00D5EEDF00Dull);
    for ( usize n = 1; n <= MAX_N; ++n ) {
      for ( usize pat = 0; pat < 5; ++pat ) {
        fill(g_a, n, pat, rng);
        for ( usize cnt = 1; cnt < mpn::limb_bits; ++cnt ) {
          mpn::zero(g_k, PAD);
          mpn::zero(g_p, PAD);
          require_true(mpn::lshift(g_k, g_a, n, cnt) == mpn::__portable::lshift(g_p, g_a, n, cnt));
          require_true(same(g_k, g_p, PAD));

          mpn::zero(g_k, PAD);
          mpn::zero(g_p, PAD);
          require_true(mpn::rshift(g_k, g_a, n, cnt) == mpn::__portable::rshift(g_p, g_a, n, cnt));
          require_true(same(g_k, g_p, PAD));

          mpn::copyi(g_k, g_a, n);
          mpn::copyi(g_p, g_a, n);
          require_true(mpn::lshift(g_k, g_k, n, cnt) == mpn::__portable::lshift(g_p, g_p, n, cnt));
          require_true(same(g_k, g_p, n));
        }
      }
    }
  }
  end_test_case();

#if defined(__micron_arbint_have_simd_mul_experiment)
  test_case("the SIMD multiply experiment is correct, so its timing means something");
  {
    prng rng(0xDEADBEEFCAFEBABEull);
    for ( usize n = 1; n <= MAX_N; ++n ) {
      for ( usize pat = 0; pat < 5; ++pat ) {
        for ( usize mw = 0; mw < 6; ++mw ) {
          fill(g_a, n, pat, rng);
          const mpn::limb_t b = pick_mul_by(mw, rng);
          mpn::zero(g_k, PAD);
          mpn::zero(g_p, PAD);
          require_true(mpn::__kern::mul_1_avx2(g_k, g_a, n, b) == mpn::__portable::mul_1(g_p, g_a, n, b));
          require_true(same(g_k, g_p, PAD));
        }
      }
    }
  }
  end_test_case();
#endif

  test_case("the constexpr path still folds, with the kernels compiled in");
  {
    constexpr auto folded = []() constexpr {
      mpn::limb_t a[4] = { mpn::limb_max, mpn::limb_max, 3u, 0u };
      mpn::limb_t r[5] = {};
      const mpn::limb_t cy = mpn::mul_1(r, a, 4, 5u);
      mpn::limb_t acc = cy;
      for ( usize i = 0; i < 5; ++i ) acc = static_cast<mpn::limb_t>(acc ^ r[i]);
      return acc;
    }();
    static_assert(folded == folded, "arbint: the mpn bridges must still fold under constant evaluation");
    require_true(true);
  }
  end_test_case();

  print("=== ARBINT KERNEL DIFFERENTIAL PASSED ===");
  return 1;
}
