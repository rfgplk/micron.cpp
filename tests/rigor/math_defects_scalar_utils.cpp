// math_defects_scalar_utils.cpp
// Regression gate for the scalar utility layer: micron::countl_zero / countr_zero / bitcount
// (bits.hpp), math::bits rotates + ceil_pow2, math::arith::saturating::neg, math::comb and
// math::median. Every assertion below is a value that the pre-fix tree got wrong.

#include "../../src/bits.hpp"
#include "../../src/math/arith.hpp"
#include "../../src/math/bits.hpp"
#include "../../src/math/numeric.hpp"
#include "../../src/math/reduce.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using namespace micron;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// oracles -- bit loops and Pascal's triangle, nothing borrowed from the code under test

template<typename T>
static u64
o_bits(T x)
{
  return static_cast<u64>(static_cast<i64>(x)) & (~0ULL >> (64 - sizeof(T) * 8));
}

template<typename T>
static int
o_clz(T x)
{
  const int w = static_cast<int>(sizeof(T) * 8);
  const u64 u = o_bits<T>(x);
  for ( int i = w - 1; i >= 0; --i )
    if ( (u >> i) & 1ULL ) return w - 1 - i;
  return w;
}

template<typename T>
static int
o_ctz(T x)
{
  const int w = static_cast<int>(sizeof(T) * 8);
  const u64 u = o_bits<T>(x);
  for ( int i = 0; i < w; ++i )
    if ( (u >> i) & 1ULL ) return i;
  return w;
}

template<typename T>
static int
o_pop(T x)
{
  const int w = static_cast<int>(sizeof(T) * 8);
  const u64 u = o_bits<T>(x);
  int c = 0;
  for ( int i = 0; i < w; ++i ) c += static_cast<int>((u >> i) & 1ULL);
  return c;
}

template<typename T>
static T
o_rot(T x, int r, bool left)
{
  const int w = static_cast<int>(sizeof(T) * 8);
  const int s = r & (w - 1);
  const u64 u = o_bits<T>(x);
  u64 out = 0;
  for ( int i = 0; i < w; ++i )
    if ( (u >> i) & 1ULL ) out |= 1ULL << (left ? ((i + s) % w) : (((i - s) % w + w) % w));
  return static_cast<T>(out);
}

template<typename T>
static T
o_ceil_pow2(T x)
{
  if ( x <= T(1) ) return T(1);
  u64 c = 1;
  while ( c < static_cast<u64>(x) ) c <<= 1;
  return (c >> (sizeof(T) * 8 - 1)) > 1ULL ? T(0) : T(c);      // 2^w is not representable
}

// C(n,k) by addition only: never overflows unless the entry itself does
static constexpr int __pascal_n = 68;
static u64 __pascal[__pascal_n][__pascal_n];
static bool __pascal_ok[__pascal_n][__pascal_n];

static void
o_pascal(void)
{
  for ( int n = 0; n < __pascal_n; ++n )
    for ( int k = 0; k <= n; ++k ) {
      if ( k == 0 || k == n ) {
        __pascal[n][k] = 1;
        __pascal_ok[n][k] = true;
        continue;
      }
      u64 s = 0;
      const bool ok = __pascal_ok[n - 1][k - 1] && __pascal_ok[n - 1][k]
                      && !__builtin_add_overflow(__pascal[n - 1][k - 1], __pascal[n - 1][k], &s);
      __pascal[n][k] = s;
      __pascal_ok[n][k] = ok;
    }
}

static u64 __rng_state = 0x9E3779B97F4A7C15ULL;

static u64
o_rng(void)
{
  __rng_state ^= __rng_state << 13;
  __rng_state ^= __rng_state >> 7;
  __rng_state ^= __rng_state << 17;
  return __rng_state;
}

int
main()
{
  print("=== MATH SCALAR-UTIL DEFECT GATE ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // #15 -- countl_zero widened a narrow signed value without masking: clz<i8>(-1) answered -56
  test_case("bits.hpp — countl_zero / countr_zero / popcount over every narrow value");
  {
    require(micron::countl_zero<i8>(i8(-1)), 0);
    require(micron::countl_zero<i16>(i16(-1)), 0);
    require(micron::countl_zero<i8>(i8(-128)), 0);
    require(micron::popcount<i8>(i8(-1)), 8);
    require(micron::popcount<i16>(i16(-1)), 16);
    require(micron::bit_width<i8>(i8(-1)), 8);

    int bad = 0;
    for ( int v = -128; v < 128; ++v ) {
      const i8 s = static_cast<i8>(v);
      const u8 u = static_cast<u8>(v & 0xFF);
      if ( micron::countl_zero<i8>(s) != o_clz<i8>(s) ) ++bad;
      if ( micron::countr_zero<i8>(s) != o_ctz<i8>(s) ) ++bad;
      if ( micron::popcount<i8>(s) != o_pop<i8>(s) ) ++bad;
      if ( micron::countl_zero<u8>(u) != o_clz<u8>(u) ) ++bad;
      if ( micron::countr_zero<u8>(u) != o_ctz<u8>(u) ) ++bad;
      if ( micron::popcount<u8>(u) != o_pop<u8>(u) ) ++bad;
    }
    for ( long v = -32768; v < 32768; ++v ) {
      const i16 s = static_cast<i16>(v);
      const u16 u = static_cast<u16>(v & 0xFFFF);
      if ( micron::countl_zero<i16>(s) != o_clz<i16>(s) ) ++bad;
      if ( micron::countr_zero<i16>(s) != o_ctz<i16>(s) ) ++bad;
      if ( micron::popcount<i16>(s) != o_pop<i16>(s) ) ++bad;
      if ( micron::countl_zero<u16>(u) != o_clz<u16>(u) ) ++bad;
      if ( micron::popcount<u16>(u) != o_pop<u16>(u) ) ++bad;
    }
    for ( int i = 0; i < 20000; ++i ) {
      const i32 s = static_cast<i32>(o_rng());
      const u64 w = o_rng();
      if ( micron::countl_zero<i32>(s) != o_clz<i32>(s) ) ++bad;
      if ( micron::popcount<i32>(s) != o_pop<i32>(s) ) ++bad;
      if ( micron::countl_zero<u64>(w) != o_clz<u64>(w) ) ++bad;
      if ( micron::countr_zero<u64>(w) != o_ctz<u64>(w) ) ++bad;
      if ( micron::popcount<u64>(w) != o_pop<u64>(w) ) ++bad;
    }
    require(bad, 0);
  }
  end_test_case();

#if defined(__micron_arch_amd64)
  // the same widen-and-truncate fault above 64 bits: the low word is all the builtin ever saw
  test_case("bits.hpp — counts over a 128-bit integer");
  {
    using u128 = unsigned __int128;
    volatile int sh = 100;      // keep the operand out of the constant folder
    const u128 one = 1;
    u128 x = one << sh;
    require(micron::countl_zero<u128>(x), 27);
    require(micron::countr_zero<u128>(x), 100);
    require(micron::popcount<u128>(x), 1);
    sh = 127;
    x = (one << sh) | one;
    require(micron::countl_zero<u128>(x), 0);
    require(micron::countr_zero<u128>(x), 0);
    require(micron::popcount<u128>(x), 2);
    sh = 0;
    x = one - one + static_cast<u128>(sh);
    require(micron::countl_zero<u128>(x), 128);
    require(micron::countr_zero<u128>(x), 128);
    sh = 64;
    x = (static_cast<u128>(0xFFFFFFFFFFFFFFFFULL) << sh) | 0xFFULL;
    require(micron::popcount<u128>(x), 72);
    require(micron::countl_zero<u128>(x), 0);
    require(micron::countr_zero<u128>(x), 0);
  }
  end_test_case();
#endif

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // #22 -- the right half of the rotate was an arithmetic shift, so a negative T flooded with sign
  test_case("math::bits — rol/ror are rotates for signed T");
  {
    require(static_cast<u32>(math::bits::ror<i32>(-2, 1)), 0x7FFFFFFFu);
    require(static_cast<u32>(math::bits::rol<i32>(static_cast<i32>(0x80000000u), 1)), 1u);
    require(static_cast<u32>(static_cast<u8>(math::bits::rol<i8>(static_cast<i8>(0x81), 1))), 3u);
    require(static_cast<u32>(math::bits::ror<u16>(0x8001u, 1)), 0xC000u);

    int bad = 0;
    for ( int v = -128; v < 128; ++v )
      for ( int r = 0; r < 32; ++r ) {
        const i8 s = static_cast<i8>(v);
        if ( math::bits::rol<i8>(s, r) != o_rot<i8>(s, r, true) ) ++bad;
        if ( math::bits::ror<i8>(s, r) != o_rot<i8>(s, r, false) ) ++bad;
      }
    for ( int i = 0; i < 20000; ++i ) {
      const i32 s = static_cast<i32>(o_rng());
      const i64 l = static_cast<i64>(o_rng());
      const int r = static_cast<int>(o_rng() & 127u);
      if ( math::bits::rol<i32>(s, r) != o_rot<i32>(s, r, true) ) ++bad;
      if ( math::bits::ror<i32>(s, r) != o_rot<i32>(s, r, false) ) ++bad;
      if ( math::bits::rol<i64>(l, r) != o_rot<i64>(l, r, true) ) ++bad;
      if ( math::bits::ror<i64>(l, r) != o_rot<i64>(l, r, false) ) ++bad;
    }
    require(bad, 0);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // #23 -- above 2^(w-1) the old form shifted by exactly w: UB, and armv7-a disagreed with x86
  test_case("math::bits — ceil_pow2 never shifts by the type width");
  {
    static_assert(math::bits::ceil_pow2<u32>(7) == 8u);
    static_assert(math::bits::ceil_pow2<u32>(16) == 16u);

    volatile u32 v32 = 0x80000001u;
    require(math::bits::ceil_pow2<u32>(static_cast<u32>(v32)), 0u);
    volatile u32 vmax = 0xFFFFFFFFu;
    require(math::bits::ceil_pow2<u32>(static_cast<u32>(vmax)), 0u);
    volatile u64 v64 = 0x8000000000000001ULL;
    require(math::bits::ceil_pow2<u64>(static_cast<u64>(v64)), 0ULL);

    int bad = 0;
    for ( int i = 0; i < 256; ++i ) {
      const u8 x = static_cast<u8>(i);
      if ( math::bits::ceil_pow2<u8>(x) != o_ceil_pow2<u8>(x) ) ++bad;
    }
    for ( long i = 0; i < 65536; ++i ) {
      const u16 x = static_cast<u16>(i);
      if ( math::bits::ceil_pow2<u16>(x) != o_ceil_pow2<u16>(x) ) ++bad;
    }
    require(bad, 0);
    require(math::bits::floor_pow2<u32>(0x80000001u), 0x80000000u);

    // bits.hpp's STL-compat sibling carried the identical shift, and bit_floor's own shift count
    // came from the countl_zero above
    volatile u32 b32 = 0x80000001u;
    require(micron::bit_ceil<u32>(static_cast<u32>(b32)), 0u);
    volatile u32 b7 = 7;
    require(micron::bit_ceil<u32>(static_cast<u32>(b7)), 8u);
    require(static_cast<int>(micron::bit_floor<i8>(i8(-1))), -128);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // #14 -- negating a nonzero unsigned underflows past T_min = 0, not up to T_max
  test_case("math::arith — saturating::neg clamps an unsigned underflow to zero");
  {
    require(math::arith::saturating::neg<u32>(1u), 0u);
    require(math::arith::saturating::neg<u32>(0xFFFFFFFFu), 0u);
    require(static_cast<u32>(math::arith::saturating::neg<u8>(3)), 0u);
    require(math::arith::saturating::neg<u64>(1ULL), 0ULL);
    require(math::arith::saturating::neg<u32>(0u), 0u);
    require(math::arith::saturating::neg<u32>(1u), math::arith::saturating::sub<u32>(0u, 1u));
    require(math::arith::saturating::neg<i32>(numeric_limits<i32>::min()), numeric_limits<i32>::max());
    require(math::arith::saturating::neg<i32>(5), -5);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // #16 -- r * (n - i) overflowed even where C(n,k) itself fits T
  test_case("math::comb — no intermediate overflow while the result is representable");
  {
    require(math::comb<u64>(63, 29), 759510004936100355ULL);
    require(math::comb<u64>(63, 30), 860778005594247069ULL);
    require(math::comb<u64>(67, 33), 14226520737620288370ULL);
    require(math::comb<i32>(30, 15), 155117520);
    require(math::comb<u64>(10, 5), 252ULL);
    require(math::comb<u64>(5, 6), 0ULL);
    constexpr u64 comb_ce = math::comb<u64>(63, 29);      // and it must be a constant expression
    require(comb_ce, 759510004936100355ULL);

    o_pascal();
    int bad = 0;
    int tested = 0;
    for ( int n = 0; n < __pascal_n; ++n )
      for ( int k = 0; k <= n; ++k ) {
        if ( !__pascal_ok[n][k] ) continue;
        ++tested;
        if ( math::comb<u64>(n, k) != __pascal[n][k] ) ++bad;
      }
    require_true(tested > 2000);
    require(bad, 0);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // #20 -- (lo + hi) / 2 overflowed for values in the top half of the range
  test_case("math::median — even-count average does not overflow");
  {
    {
      i32 v[2] = { 2000000000, 2000000002 };
      require(math::median<i32>(v, v + 2), 2000000001);
    }
    {
      u32 v[2] = { 4000000000u, 4000000002u };
      require(math::median<u32>(v, v + 2), 4000000001u);
    }
    {
      i32 v[2] = { numeric_limits<i32>::min(), numeric_limits<i32>::max() };
      require(math::median<i32>(v, v + 2), 0);
    }
    {
      i64 v[2] = { numeric_limits<i64>::min(), numeric_limits<i64>::min() + 2 };
      require(math::median<i64>(v, v + 2), numeric_limits<i64>::min() + 1);
    }
    {
      u64 v[2] = { 0xFFFFFFFFFFFFFFF0ULL, 0xFFFFFFFFFFFFFFF4ULL };
      require(math::median<u64>(v, v + 2), 0xFFFFFFFFFFFFFFF2ULL);
    }
    // truncation toward zero, unchanged from the pre-fix behaviour on in-range inputs
    {
      i32 v[2] = { -6, -3 };
      require(math::median<i32>(v, v + 2), -4);
    }
    {
      i32 v[2] = { -3, 6 };
      require(math::median<i32>(v, v + 2), 1);
    }
    {
      i32 v[2] = { 3, 6 };
      require(math::median<i32>(v, v + 2), 4);
    }
    {
      f64 v[2] = { 1.0, 2.0 };
      require_true(math::median<f64>(v, v + 2) == 1.5);
    }

    // small magnitudes: (lo + hi) / 2 cannot overflow there, so it is a valid independent oracle
    int bad = 0;
    for ( int i = 0; i < 20000; ++i ) {
      i32 a[4];
      i32 c[4];
      const int n = 2 + 2 * static_cast<int>(o_rng() & 1u);
      for ( int j = 0; j < n; ++j ) {
        a[j] = static_cast<i32>(o_rng() % 200000u) - 100000;
        c[j] = a[j];
      }
      for ( int x = 1; x < n; ++x ) {
        const i32 key = c[x];
        int y = x - 1;
        while ( y >= 0 && c[y] > key ) {
          c[y + 1] = c[y];
          --y;
        }
        c[y + 1] = key;
      }
      const i32 want = (n & 1) ? c[n / 2] : static_cast<i32>((c[n / 2 - 1] + c[n / 2]) / 2);
      if ( math::median<i32>(a, a + n) != want ) ++bad;
    }
    require(bad, 0);
  }
  end_test_case();

  print("=== MATH SCALAR-UTIL DEFECT GATE PASSED ===");
  return 1;
}
