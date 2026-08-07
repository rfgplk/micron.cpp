// math_arbint_convert.cpp
// Base 2..36 in and out: known answers, exhaustive round-trips, and the malformed inputs that must
// be refused rather than guessed at.

#include "../../src/math/arbint.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"
#include "../support/oracles.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using micron::math::arbint;
using micron::math::arbuint;
using micron::math::from_chars;
using micron::math::to_chars;
using micron::math::to_string;
using mtest::prng;

#if defined(ARBINT_RIGOR_LITE) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_PER_BASE = 8;
constexpr static const usize MAX_LIMBS = 8;
#else
constexpr static const usize N_PER_BASE = 40;
constexpr static const usize MAX_LIMBS = 20;
#endif

using U = arbuint<>;
using Z = arbint<>;

static char g_buf[8192];

static bool
text_is(const char *p, usize n, const char *want)
{
  usize k = 0;
  while ( want[k] != '\0' ) ++k;
  if ( k != n ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( p[i] != want[i] ) return false;
  return true;
}

static U
random_value(prng &rng, usize limbs)
{
  U a;
  for ( usize i = 0; i < limbs; ++i ) {
    a <<= 64;
    a += U(rng.next());
  }
  return a;
}

int
main()
{
  print("=== ARBINT CONVERSION RIGOR ===");

  test_case("known answers");
  {
    usize n = to_chars(g_buf, sizeof g_buf, U(0u));
    require_true(text_is(g_buf, n, "0"));

    n = to_chars(g_buf, sizeof g_buf, U(1234567890u));
    require_true(text_is(g_buf, n, "1234567890"));

    n = to_chars(g_buf, sizeof g_buf, Z(-1234567890));
    require_true(text_is(g_buf, n, "-1234567890"));

    n = to_chars(g_buf, sizeof g_buf, U(255u), 16);
    require_true(text_is(g_buf, n, "ff"));
    n = to_chars(g_buf, sizeof g_buf, U(255u), 16, true);
    require_true(text_is(g_buf, n, "FF"));
    n = to_chars(g_buf, sizeof g_buf, U(5u), 2);
    require_true(text_is(g_buf, n, "101"));
    n = to_chars(g_buf, sizeof g_buf, U(35u), 36);
    require_true(text_is(g_buf, n, "z"));
    n = to_chars(g_buf, sizeof g_buf, U(36u), 36);
    require_true(text_is(g_buf, n, "10"));

    // 2^256 and 2^512, digit for digit
    n = to_chars(g_buf, sizeof g_buf, U::power_of_two(256));
    require_true(text_is(g_buf, n, "115792089237316195423570985008687907853269984665640564039457584007913129639936"));
    n = to_chars(g_buf, sizeof g_buf, U::power_of_two(512));
    require_true(text_is(g_buf, n,
                         "134078079299425970995740249982058461274793658205923933777235614437217640300735"
                         "46976801874298166903427690031858186486050853753882811946569946433649006084096"));

    // 2^127 - 1, a Mersenne prime, and 10^30
    U m = U::power_of_two(127) - U(1u);
    n = to_chars(g_buf, sizeof g_buf, m);
    require_true(text_is(g_buf, n, "170141183460469231731687303715884105727"));
    n = to_chars(g_buf, sizeof g_buf, m, 16);
    require_true(text_is(g_buf, n, "7fffffffffffffffffffffffffffffff"));

    U tp = U(1u);
    for ( int i = 0; i < 30; ++i ) tp *= U(10u);
    n = to_chars(g_buf, sizeof g_buf, tp);
    require_true(text_is(g_buf, n, "1000000000000000000000000000000"));
  }
  end_test_case();

  test_case("round trip through every base, unsigned and signed");
  {
    prng rng(0x5EEDF00D5EEDF00Dull);
    for ( u32 base = 2; base <= 36; ++base ) {
      for ( usize t = 0; t < N_PER_BASE; ++t ) {
        const usize limbs = 1u + rng.next_in(MAX_LIMBS);
        const U a = random_value(rng, limbs);

        const usize n = to_chars(g_buf, sizeof g_buf, a, base);
        require_true(n > 0);
        U back;
        require_true(from_chars(back, g_buf, n, base));
        require_true(back == a);

        // no leading zero ever survives the trip
        require_true(a.is_zero() || g_buf[0] != '0');

        for ( int neg = 0; neg < 2; ++neg ) {
          const Z s(a, neg != 0);
          const usize m = to_chars(g_buf, sizeof g_buf, s, base);
          require_true(m > 0);
          Z sback;
          require_true(from_chars(sback, g_buf, m, base));
          require_true(sback == s);
        }
      }
    }
  }
  end_test_case();

  test_case("uppercase and lowercase parse to the same value");
  {
    prng rng(0xABCDEF0123456789ull);
    for ( u32 base = 11; base <= 36; ++base ) {
      for ( usize t = 0; t < N_PER_BASE / 2u + 1u; ++t ) {
        const U a = random_value(rng, 1u + rng.next_in(6));
        const usize lo = to_chars(g_buf, sizeof g_buf, a, base, false);
        U from_lo;
        require_true(from_chars(from_lo, g_buf, lo, base));
        const usize up = to_chars(g_buf, sizeof g_buf, a, base, true);
        U from_up;
        require_true(from_chars(from_up, g_buf, up, base));
        require(lo, up);
        require_true(from_lo == a && from_up == a);
      }
    }
  }
  end_test_case();

  test_case("to_string agrees with to_chars");
  {
    prng rng(0xF4F4F4F4F4ull);
    for ( usize t = 0; t < 60; ++t ) {
      const U a = random_value(rng, 1u + rng.next_in(12));
      const micron::string s = to_string(a);
      const usize n = to_chars(g_buf, sizeof g_buf, a);
      require(s.size(), n);
      for ( usize i = 0; i < n; ++i ) require_true(s.data()[i] == g_buf[i]);

      const Z z(a, true);
      const micron::string sz = to_string(z);
      const usize m = to_chars(g_buf, sizeof g_buf, z);
      require(sz.size(), m);
      for ( usize i = 0; i < m; ++i ) require_true(sz.data()[i] == g_buf[i]);
    }
    require_true(to_string(U(0u)).size() == 1);
    require_true(to_string(Z(0)).data()[0] == '0');
  }
  end_test_case();

  test_case("malformed input is refused");
  {
    U u;
    require_true(!from_chars(u, "12x4", 4, 10));
    require_true(!from_chars(u, "", 0, 10));
    require_true(!from_chars(u, "9", 1, 8));         // 9 is not an octal digit
    require_true(!from_chars(u, "2", 1, 2));         // nor 2 a binary one
    require_true(!from_chars(u, "z", 1, 35));        // one past the top of base 35
    require_true(!from_chars(u, "1", 1, 1));         // base 1 is not a base
    require_true(!from_chars(u, "1", 1, 37));
    require_true(!from_chars(u, " 1", 2, 10));       // no leading whitespace is accepted
    require_true(!from_chars(u, "-1", 2, 10));       // the unsigned form takes no sign

    Z z;
    require_true(!from_chars(z, "-", 1, 10));
    require_true(!from_chars(z, "+", 1, 10));
    require_true(from_chars(z, "-0", 2, 10) && z.sign() == 0 && !z.negative());
    require_true(from_chars(z, "+42", 3, 10) && z == Z(42));
    require_true(from_chars(z, "-42", 3, 10) && z == Z(-42));

    // a buffer that cannot hold the answer reports zero rather than writing past its end
    char tiny[2];
    require(to_chars(tiny, 1, U(1000u)), usize(0));
    require(to_chars(tiny, 0, U(0u)), usize(0));
  }
  end_test_case();

  test_case("power-of-two bases agree with the general path");
  {
    // base 2/4/8/16/32 take the bit-slicing route; every other base divides. they must not disagree.
    prng rng(0x1234ABCD8765EF01ull);
    for ( usize t = 0; t < 120; ++t ) {
      const U a = random_value(rng, 1u + rng.next_in(10));
      // rebuild the value from its base-16 text and compare against its base-10 text
      const usize n16 = to_chars(g_buf, sizeof g_buf, a, 16);
      U from16;
      require_true(from_chars(from16, g_buf, n16, 16));
      const usize n10 = to_chars(g_buf, sizeof g_buf, a, 10);
      U from10;
      require_true(from_chars(from10, g_buf, n10, 10));
      require_true(from16 == a && from10 == a && from16 == from10);

      // and every power-of-two base against every other
      for ( u32 b : { 2u, 4u, 8u, 16u, 32u } ) {
        const usize k = to_chars(g_buf, sizeof g_buf, a, b);
        U back;
        require_true(from_chars(back, g_buf, k, b));
        require_true(back == a);
      }
    }
  }
  end_test_case();

  test_case("the divide-and-conquer formatter agrees with the basecase, deep");
  {
    // to_string takes the D&C route above threshold::get_str_dc; to_chars never does. so comparing
    // them IS the differential, and it only means anything at sizes where the recursion nests --
    // every size below is several tower levels deep.
    prng rng(0x0F1E2D3C4B5A6978ull);
    // to_chars needs bit_length characters of room, so the buffer bounds the sizes below
    static char big[70000];
    const usize limbs[] = { 16u, 40u, 100u, 250u, 600u, 1000u };
    U p10 = U(1u);
    for ( usize i = 0; i < 200; ++i ) p10 *= U(10u);
    for ( usize li = 0; li < 6; ++li ) {
      const usize n = limbs[li];
#if defined(ARBINT_RIGOR_LITE) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
      if ( n > 250u ) continue;
#endif
      U a = random_value(rng, n);
      for ( u32 base : { 3u, 10u, 36u } ) {
        const usize want = to_chars(big, sizeof big, a, base);
        require_true(want > 0);
        const micron::string got = to_string(a, base);
        require(got.size(), want);
        for ( usize i = 0; i < want; ++i ) require_true(got.data()[i] == big[i]);
        // and it still round-trips
        U back;
        require_true(from_chars(back, got.data(), got.size(), base));
        require_true(back == a);
      }
      // a value that is exactly a power of ten, and its two neighbours -- the split boundaries
      for ( const U &v : { p10, p10 - U(1u), p10 + U(1u) } ) {
        const usize want = to_chars(big, sizeof big, v, 10);
        const micron::string got = to_string(v, 10);
        require(got.size(), want);
        for ( usize i = 0; i < want; ++i ) require_true(got.data()[i] == big[i]);
      }
    }
  }
  end_test_case();

  print("=== ARBINT CONVERSION RIGOR PASSED ===");
  return 1;
}
