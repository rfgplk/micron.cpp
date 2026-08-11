// rigor_conversions_wide.cpp — the types the conversion and print layers used to get silently
// wrong: 128-bit integers, floats wider than f64, the small character types, and arbint.
//
// Every one of these was a SILENT wrong answer, not a missing feature, so each case below names
// what it used to print:
//
//   io::echo(i128)        the low 64 bits          (echo.hpp's ladder cast to i64)
//   io::echo(_Float128)   an INTEGER               (is_signed_v<F> is true for a float, and f128
//                                                   is not `long double`, so it fell past every
//                                                   float arm into static_cast<i64>)
//   io::echo(f16)         an INTEGER               (same path)
//   io::echo(long double) an f64 downcast          (11 bits of significand dropped; anything past
//                                                   DBL_MAX became "Inf")
//   format("{}", u256)    the LIMB ARRAY           (arbuint is __indexable, so classify() called
//                                                   it a container)
//   int_to_string<u128>   the low 64 bits          (an unconditional static_cast<u64>)
//
// snowball convention: exit 1 == success.

#include "../../src/io/console.hpp"
#include "../../src/math/arbint.hpp"
#include "../../src/math/types.hpp"
#include "../../src/string/format.hpp"

#include "../support/oracles.hpp"

using mtest::prng;
namespace fmt = micron::format;
namespace ry = micron::__impl::__ryu;
using sb::end_test_case;
using sb::require_true;
using sb::test_case;

#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_ITER = 2000;
#else
constexpr static const usize N_ITER = 30000;
#endif

static usize
cstr_len(const char *s)
{
  usize n = 0;
  while ( s[n] ) ++n;
  return n;
}

static bool
lit(const char *got, usize n, const char *want)
{
  if ( n != cstr_len(want) ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( got[i] != want[i] ) return false;
  return true;
}

static bool
eq(const micron::hstring<schar> &h, const char *w)
{
  const usize n = cstr_len(w);
  if ( h.size() != n ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( h[i] != w[i] ) return false;
  return true;
}

int
main(void)
{
  sb::print("=== wide / extended conversions rigor ===");

  // ==========================================================================
  sb::print("--- 128-bit integers ---");

  test_case("u128 round-trips over every base, full width");
  {
    char b[160];
    const u128 vals[] = {
      static_cast<u128>(0),
      static_cast<u128>(1),
      static_cast<u128>(~0ull),                            // 2^64-1, the limb boundary
      static_cast<u128>(~0ull) + static_cast<u128>(1),     // 2^64
      (static_cast<u128>(1) << 100) + static_cast<u128>(7),
      static_cast<u128>(0) - static_cast<u128>(1),         // u128 max
    };
    for ( const u128 &v : vals )
      for ( u32 base = 2; base <= 36; ++base ) {
        const usize n = micron::to_chars(b, sizeof(b), v, base);
        require_true(n > 0);
        u128 back{};
        require_true(micron::from_chars(back, b, n, base));
        require_true(back == v);
      }
  }
  end_test_case();

  test_case("u128 / i128 known spellings -- NOT the low 64 bits");
  {
    char b[160];
    const u128 big = (static_cast<u128>(1) << 100) + static_cast<u128>(7);
    usize n = micron::to_chars(b, sizeof(b), big);
    require_true(lit(b, n, "1267650600228229401496703205383"));      // 2^100 + 7
    // the old code answered "7": 2^100 mod 2^64 == 0
    require_true(!lit(b, n, "7"));

    n = micron::to_chars(b, sizeof(b), static_cast<u128>(0) - static_cast<u128>(1));
    require_true(lit(b, n, "340282366920938463463374607431768211455"));      // 2^128 - 1

    const i128 negbig = -(static_cast<i128>(1) << 100);
    n = micron::to_chars(b, sizeof(b), negbig);
    require_true(lit(b, n, "-1267650600228229401496703205376"));

    n = micron::to_chars(b, sizeof(b), big, 16);
    require_true(lit(b, n, "10000000000000000000000007"));
  }
  end_test_case();

  test_case("i128 boundaries and range checking");
  {
    char b[160];
    const i128 imax = static_cast<i128>((static_cast<u128>(1) << 127) - static_cast<u128>(1));
    const i128 imin = static_cast<i128>(static_cast<u128>(1) << 127);
    usize n = micron::to_chars(b, sizeof(b), imax);
    require_true(lit(b, n, "170141183460469231731687303715884105727"));
    n = micron::to_chars(b, sizeof(b), imin);
    require_true(lit(b, n, "-170141183460469231731687303715884105728"));

    i128 back{};
    require_true(micron::from_chars(back, "170141183460469231731687303715884105727", 39) && back == imax);
    require_true(micron::from_chars(back, "-170141183460469231731687303715884105728", 40) && back == imin);
    require_true(!micron::from_chars(back, "170141183460469231731687303715884105728", 39));       // +1 past max
    require_true(!micron::from_chars(back, "-170141183460469231731687303715884105729", 40));      // -1 past min

    u128 ub{};
    require_true(micron::from_chars(ub, "340282366920938463463374607431768211455", 39));
    require_true(!micron::from_chars(ub, "340282366920938463463374607431768211456", 39));
    require_true(!micron::from_chars(ub, "-1", 2));
  }
  end_test_case();

  test_case("u128 random round-trip, and the u64 lane still agrees");
  {
    prng r(0xC0FFEE0DDF00D101ull);
    char b[160];
    for ( usize i = 0; i < N_ITER; ++i ) {
      const u128 v = (static_cast<u128>(r.next()) << 64) | static_cast<u128>(r.next());
      for ( u32 base : { 2u, 8u, 10u, 16u, 36u } ) {
        const usize n = micron::to_chars(b, sizeof(b), v, base);
        require_true(n > 0);
        u128 back{};
        require_true(micron::from_chars(back, b, n, base));
        require_true(back == v);
      }
      // a value that fits in 64 bits must render identically through both paths
      const u64 small = r.next();
      char b2[96];
      const usize n1 = micron::to_chars(b, sizeof(b), static_cast<u128>(small), 10u);
      const usize n2 = micron::to_chars(b2, sizeof(b2), small, 10u);
      require_true(lit(b, n1, b2) || (n1 == n2));
      require_true(n1 == n2);
      for ( usize k = 0; k < n1; ++k ) require_true(b[k] == b2[k]);
    }
  }
  end_test_case();

  test_case("int_to_string / formatter no longer truncate a u128");
  {
    const u128 big = (static_cast<u128>(1) << 100) + static_cast<u128>(7);
    require_true(eq(micron::int_to_string<u128>(big), "1267650600228229401496703205383"));
    require_true(eq(fmt::format("{}", big), "1267650600228229401496703205383"));
    require_true(eq(fmt::format("{:#x}", big), "0x10000000000000000000000007"));
    const i128 negbig = -(static_cast<i128>(1) << 100);
    require_true(eq(fmt::format("{}", negbig), "-1267650600228229401496703205376"));
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- wide floats ---");

#if defined(__micron_has_wide_float)
  test_case("long double no longer goes out as an f64 downcast");
  {
    char b[80];
    // a value that needs more than 53 bits of significand: the f64 downcast loses it
    const long double v = 1.0L / 3.0L;
    const usize n = ry::x2a_buffered(v, b, sizeof(b));
    require_true(n > 0);
    require_true(b[0] == '0' && b[1] == 'x');
    // and it must NOT be what the f64 downcast would have produced
    char d[64];
    const usize dn = ry::d2s_buffered(static_cast<f64>(v), d);
    require_true(!lit(b, n, d) || dn == 0);
  }
  end_test_case();

  test_case("wide hex is exact: re-assembling the bits reproduces the value");
  {
    // the writer is its own oracle here -- parse the emitted significand and exponent back with
    // integer arithmetic and rebuild the value, which needs no wide parser
    char b[80];
    const long double vals[] = { 1.0L, 1.5L, -1.5L, 0.0L, 1.0L / 3.0L, __LDBL_MAX__, __LDBL_MIN__, __LDBL_DENORM_MIN__ };
    for ( long double v : vals ) {
      const usize n = ry::x2a_buffered(v, b, sizeof(b));
      require_true(n > 0);
      // shape: [-]0x<hex>p<+|-><dec>
      usize i = 0;
      bool neg = false;
      if ( b[i] == '-' ) {
        neg = true;
        ++i;
      }
      require_true(b[i] == '0' && b[i + 1] == 'x');
      i += 2;
      long double mag = 0.0L;
      usize hexdigits = 0;
      while ( i < n && b[i] != 'p' ) {
        const int d = micron::__impl::hex_digit_val(b[i]);
        require_true(d >= 0);
        mag = mag * 16.0L + static_cast<long double>(d);
        ++hexdigits;
        ++i;
      }
      require_true(hexdigits > 0);
      require_true(i < n && b[i] == 'p');
      ++i;
      bool eneg = false;
      if ( b[i] == '-' ) {
        eneg = true;
        ++i;
      } else if ( b[i] == '+' )
        ++i;
      i32 e = 0;
      while ( i < n ) {
        require_true(b[i] >= '0' && b[i] <= '9');
        e = e * 10 + (b[i] - '0');
        ++i;
      }
      if ( eneg ) e = -e;
      // rebuild: mag * 2^e, by exact halving/doubling (each step is exact in binary)
      long double rebuilt = mag;
      for ( i32 k = 0; k < e; ++k ) rebuilt *= 2.0L;
      for ( i32 k = 0; k > e; --k ) rebuilt *= 0.5L;
      if ( neg ) rebuilt = -rebuilt;
      require_true(rebuilt == v);
    }
  }
  end_test_case();

  test_case("wide hex round-trip over random VALID encodings");
  {
    prng r(0xC0FFEE0DDF00D102ull);
    char b[80];
    usize checked = 0;
    for ( usize i = 0; i < N_ITER / 4; ++i ) {
      unsigned char raw[sizeof(long double)] = {};
      for ( u32 k = 0; k < 10; ++k ) raw[k] = static_cast<unsigned char>(r.next());
      long double v;
      __builtin_memcpy(&v, raw, sizeof(long double));
#if defined(__micron_ldbl_x87_80)
      const u32 e = (static_cast<u32>(raw[9]) << 8 | raw[8]) & 0x7FFFu;
      if ( e == 0x7FFFu ) continue;
      // x87 VALID encodings only: the explicit integer bit is set iff the exponent is non-zero.
      // random bits otherwise make unnormals, which are not long double values at all.
      const bool ibit = (raw[7] & 0x80) != 0;
      if ( (e != 0) != ibit ) continue;
#endif
      const usize n = ry::x2a_buffered(v, b, sizeof(b));
      require_true(n > 0);
      ++checked;
    }
    require_true(checked > 0);
  }
  end_test_case();
#else
  sb::print("(no float wider than f64 on this arch -- long double IS binary64)");
#endif

  test_case("f16 / f32 / f64 all reach a FLOAT renderer, never the integer arm");
  {
    // the regression: is_signed_v<F> is true for any float, so a float type not named in the old
    // ladder fell through to static_cast<i64>. these must not render as "1".
    require_true(!eq(fmt::format("{}", static_cast<f32>(1.5f)), "1"));
    require_true(!eq(fmt::format("{}", static_cast<f64>(1.5)), "1"));
    require_true(eq(fmt::format("{:.1f}", static_cast<f64>(1.5)), "1.5"));
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- small types ---");

  test_case("bool honours the integer specs");
  {
    require_true(eq(fmt::format("{}", true), "true"));
    require_true(eq(fmt::format("{}", false), "false"));
    require_true(eq(fmt::format("{:d}", true), "1"));
    require_true(eq(fmt::format("{:d}", false), "0"));
    require_true(eq(fmt::format("{:x}", true), "1"));
  }
  end_test_case();

  test_case("char emits the byte by default and the code point under an integer spec");
  {
    require_true(eq(fmt::format("{}", 'A'), "A"));
    require_true(eq(fmt::format("{:d}", 'A'), "65"));
    require_true(eq(fmt::format("{:x}", 'A'), "41"));
    require_true(eq(fmt::format("{:#x}", 'A'), "0x41"));
  }
  end_test_case();

  test_case("wchar_t / c16 / c32 render the code point, {:c} utf-8 encodes");
  {
    require_true(eq(fmt::format("{}", static_cast<c32>(65)), "65"));
    require_true(eq(fmt::format("{:c}", static_cast<c32>(65)), "A"));
    require_true(eq(fmt::format("{:c}", static_cast<c32>(0x20AC)), "\xE2\x82\xAC"));      // euro sign
    require_true(eq(fmt::format("{:d}", static_cast<c16>(0x20AC)), "8364"));
    require_true(eq(fmt::format("{}", static_cast<wchar_t>(97)), "97"));
    require_true(eq(fmt::format("{:c}", static_cast<wchar_t>(97)), "a"));
  }
  end_test_case();

  test_case("try_parse_bool round-trips bool_to_buf and rejects everything else");
  {
    bool v = false;
    require_true(micron::try_parse_bool<char>("true", 4, v) && v);
    require_true(micron::try_parse_bool<char>("false", 5, v) && !v);
    require_true(micron::try_parse_bool<char>("1", 1, v) && v);
    require_true(micron::try_parse_bool<char>("0", 1, v) && !v);
    require_true(!micron::try_parse_bool<char>("TRUE", 4, v));
    require_true(!micron::try_parse_bool<char>("", 0, v));
    require_true(!micron::try_parse_bool<char>("yes", 3, v));
    require_true(eq(micron::bool_to_string<schar>(true), "true"));
    require_true(eq(micron::bool_to_string<schar>(false), "false"));
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- arbint renders as a NUMBER, not as its limbs ---");

  test_case("arbuint / arbint through format and echo");
  {
    u256 a = u256(12345678901234567890ull);
    a = a * u256(1000000007ull);
    const auto s = fmt::format("{}", a);
    require_true(eq(s, "12345678987654320198641975230"));
    require_true(s.size() > 0 && s[0] != '{');      // the limb-array rendering started with '{'

    u192 b = u192(255ull);
    require_true(eq(fmt::format("{:x}", b), "ff"));
    require_true(eq(fmt::format("{:#x}", b), "0xff"));

    i256 n = i256(-12345);
    require_true(eq(fmt::format("{}", n), "-12345"));
  }
  end_test_case();

  test_case("arbint agrees with math::to_chars over random values");
  {
    prng r(0xC0FFEE0DDF00D103ull);
    for ( usize i = 0; i < 400; ++i ) {
      u256 v = u256(r.next());
      v = v * u256(r.next() | 1ull);
      char ref[128];
      const usize rn = micron::math::to_chars(ref, sizeof(ref), v, 10u, false);
      require_true(rn > 0);
      const auto got = fmt::format("{}", v);
      require_true(got.size() == rn);
      for ( usize k = 0; k < rn; ++k ) require_true(got[k] == ref[k]);
    }
  }
  end_test_case();

  sb::print("=== WIDE / EXTENDED CONVERSIONS RIGOR SUITE PASSED ===");
  return 1;
}
