// rigor_memcmp_ct.cpp -- cmemcmp / rcmemcmp, the compile-time-count siblings of memcmp.
//
// memcmp, rmemcmp and constexpr_memcmp were all re-pointed at __memcmp_scalar, which answers
// -1/0/1. Their two compile-time-count siblings were not: they kept
//
//     return (const byte *)&src[i] - (const byte *)&dest[i];
//
// in four unrolled copies each -- the distance between two UNRELATED objects, whose sign is
// whatever the stack or the allocator happened to give. cmemcmp<4, u32>(a, b) with a[0] < b[0] is
// required to be negative but came back POSITIVE whenever `a` sat above `b` in memory. Every
// in-tree caller passes T = byte and so takes the __memcmp_bytes branch, which is exactly why
// nothing caught it, and the new cmemory_constexpr.cpp gate exercises memcmp/constexpr_memcmp but
// never these two.
//
// They were also left non-constexpr while their siblings became constexpr, so the string tier
// could not reach them at compile time.
//
// ORACLE: an element-wise lexicographic compare written out longhand here. The property under
// test is only the SIGN, since that is the whole contract -- and the sign is precisely what the
// pointer-distance formula got wrong.

#include "../../src/memory/cmemory.hpp"
#include "../../src/types.hpp"

#include "../support/oracles.hpp"

using mtest::prng;
using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_FUZZ = 20000;
#else
constexpr static const usize N_FUZZ = 200000;
#endif

// the oracle: lexicographic over elements, reported as a sign
template<typename T>
static int
oracle(const T *a, const T *b, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    if ( a[i] < b[i] ) return -1;
    if ( a[i] > b[i] ) return 1;
  }
  return 0;
}

static int
sgn(i64 v) noexcept
{
  return v < 0 ? -1 : (v > 0 ? 1 : 0);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compile-time gates. these are the half that could not even be written before: cmemcmp and
// rcmemcmp were not constexpr, so none of this compiled.

consteval bool
ct_checks()
{
  const u32 a[4] = { 1, 2, 3, 4 };
  const u32 b[4] = { 1, 2, 3, 5 };
  const u32 c[4] = { 1, 2, 3, 4 };
  if ( micron::cmemcmp<4, u32>(a, b) >= 0 ) return false;
  if ( micron::cmemcmp<4, u32>(b, a) <= 0 ) return false;
  if ( micron::cmemcmp<4, u32>(a, c) != 0 ) return false;

  // the non-multiple-of-4 arm takes the other loop
  const u16 p[3] = { 7, 7, 7 };
  const u16 q[3] = { 7, 8, 7 };
  if ( micron::cmemcmp<3, u16>(p, q) >= 0 ) return false;
  if ( micron::cmemcmp<3, u16>(q, p) <= 0 ) return false;

  // the sizeof(T) == 1 arm has to fold too -- it is behind `if !consteval`
  const char s1[4] = { 'a', 'b', 'c', 'd' };
  const char s2[4] = { 'a', 'b', 'c', 'e' };
  if ( micron::cmemcmp<4, char>(s1, s2) >= 0 ) return false;
  if ( micron::cmemcmp<4, char>(s1, s1) != 0 ) return false;
  return true;
}

static_assert(ct_checks(), "cmemcmp must be constexpr and correctly signed at compile time");

int
main()
{
  prng rng(0xbeefcafe0badf00dULL);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("the named reproducer: the sign must not depend on where the arrays landed");
  {
    // deliberately allocated so that `a` and `b` sit in both orders relative to each other. under
    // the pointer-distance formula exactly one of these two blocks reported the wrong sign.
    u32 a[4] = { 1, 0, 0, 0 };
    u32 b[4] = { 2, 0, 0, 0 };
    require_true(micron::cmemcmp<4, u32>(a, b) < 0);
    require_true(micron::cmemcmp<4, u32>(b, a) > 0);
    require_true(micron::cmemcmp<4, u32>(a, a) == 0);

    // and with the operand order in memory reversed
    static u32 hi[4] = { 2, 0, 0, 0 };
    static u32 lo[4] = { 1, 0, 0, 0 };
    require_true(micron::cmemcmp<4, u32>(lo, hi) < 0);
    require_true(micron::cmemcmp<4, u32>(hi, lo) > 0);

    require_true(micron::rcmemcmp<4, u32>(a, b) < 0);
    require_true(micron::rcmemcmp<4, u32>(b, a) > 0);
    require_true(micron::rcmemcmp<4, u32>(a, a) == 0);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("the answer is -1/0/1, matching memcmp's own contract");
  {
    u32 a[4] = { 1, 0, 0, 0 };
    u32 b[4] = { 9999, 0, 0, 0 };
    require_true(micron::cmemcmp<4, u32>(a, b) == -1);
    require_true(micron::cmemcmp<4, u32>(b, a) == 1);
    // a huge element gap must not scale the answer -- that was the other tell of the old formula
    u64 x[2] = { 0, 0 };
    u64 y[2] = { ~0ull, 0 };
    require_true(micron::cmemcmp<2, u64>(x, y) == -1);
    require_true(micron::cmemcmp<2, u64>(y, x) == 1);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: cmemcmp/rcmemcmp agree with the lexicographic oracle, T = u32 (M % 4 == 0)");
  {
    for ( usize i = 0; i < N_FUZZ; ++i ) {
      u32 a[8], b[8];
      for ( usize k = 0; k < 8; ++k ) {
        a[k] = static_cast<u32>(rng.next());
        // mostly-equal prefixes, so the first difference lands at every position often enough
        b[k] = (rng.next() % 4ull) ? a[k] : static_cast<u32>(rng.next());
      }
      require_true(sgn(micron::cmemcmp<8, u32>(a, b)) == oracle(a, b, 8));
      require_true(sgn(micron::cmemcmp<8, u32>(b, a)) == oracle(b, a, 8));
      require_true(sgn(micron::rcmemcmp<8, u32>(a, b)) == oracle(a, b, 8));
      require_true(sgn(micron::cmemcmp<4, u32>(a, b)) == oracle(a, b, 4));
      // and the runtime-count sibling must give the same sign for the same data
      require_true(sgn(micron::memcmp<u32, u32>(a, b, 8)) == oracle(a, b, 8));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: the non-multiple-of-4 loop, and the wider element types");
  {
    for ( usize i = 0; i < N_FUZZ; ++i ) {
      u16 a[6], b[6];
      for ( usize k = 0; k < 6; ++k ) {
        a[k] = static_cast<u16>(rng.next());
        b[k] = (rng.next() % 3ull) ? a[k] : static_cast<u16>(rng.next());
      }
      require_true(sgn(micron::cmemcmp<6, u16>(a, b)) == oracle(a, b, 6));
      require_true(sgn(micron::cmemcmp<3, u16>(a, b)) == oracle(a, b, 3));
      require_true(sgn(micron::rcmemcmp<3, u16>(a, b)) == oracle(a, b, 3));

      u64 x[3], y[3];
      for ( usize k = 0; k < 3; ++k ) {
        x[k] = rng.next();
        y[k] = (rng.next() % 3ull) ? x[k] : rng.next();
      }
      require_true(sgn(micron::cmemcmp<3, u64>(x, y)) == oracle(x, y, 3));
      require_true(sgn(micron::rcmemcmp<3, u64>(x, y)) == oracle(x, y, 3));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: the byte arm still agrees, run time and compile time alike");
  {
    for ( usize i = 0; i < N_FUZZ; ++i ) {
      byte a[16], b[16];
      for ( usize k = 0; k < 16; ++k ) {
        a[k] = static_cast<byte>(rng.next());
        b[k] = (rng.next() % 5ull) ? a[k] : static_cast<byte>(rng.next());
      }
      require_true(sgn(micron::cmemcmp<16, byte>(a, b)) == oracle(a, b, 16));
      require_true(sgn(micron::rcmemcmp<16, byte>(a, b)) == oracle(a, b, 16));
      require_true(sgn(micron::memcmp<byte, byte>(a, b, 16)) == oracle(a, b, 16));
    }
  }
  end_test_case();

  print("=== CMEMCMP RIGOR SUITE PASSED ===");
  return 1;
}
