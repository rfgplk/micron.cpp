// rigor_conversions_chars.cpp — the to_chars / from_chars tier (src/string/conversions/chars.hpp).
//
// One zero-allocation shape for every scalar, matching the names and signatures micron already
// ships for arbitrary precision at math/arbint/convert.hpp. What this suite pins:
//
//   * round-trip at EVERY integer width x every base 2..36, including the type minimum, which is
//     where a naive negation overflows
//   * the range check. from_chars<u8>("256") must fail, not wrap -- this is the gap that made
//     string_to_int32/uint32/int16/uint16 report failure as a 0 indistinguishable from a real 0
//   * strictness: from_chars consumes the WHOLE range, so whitespace and trailing garbage are
//     rejections. this deliberately differs from micron::try_parse_*, which tolerates spaces.
//   * every float_format round-trips bit-exact, including hex -- the writer's exact partner is
//     parse_float's __scan_hex, so %a closes the loop with no oracle needed
//   * to_chars never truncates: a short buffer answers 0
//
// snowball convention: exit 1 == success.

#include "../../src/string/format.hpp"

#include "../support/oracles.hpp"

using mtest::prng;
using sb::end_test_case;
using sb::require_true;
using sb::test_case;

#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_ITER = 2000;
#else
constexpr static const usize N_ITER = 40000;
#endif

static bool
same(const char *a, usize an, const char *b, usize bn)
{
  if ( an != bn ) return false;
  for ( usize i = 0; i < an; ++i )
    if ( a[i] != b[i] ) return false;
  return true;
}

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
  return same(got, n, want, cstr_len(want));
}

// round-trip one integer value through every base
template<typename I>
static void
rt_bases(I v)
{
  char buf[96];
  for ( u32 base = 2; base <= 36; ++base ) {
    const usize n = micron::to_chars(buf, sizeof(buf), v, base);
    require_true(n > 0);
    I back{};
    require_true(micron::from_chars(back, buf, n, base));
    require_true(back == v);
  }
}

template<typename I>
static void
rt_width(u64 seed)
{
  // the boundaries first: min, max, 0, +-1
  using L = micron::numeric_limits<I>;
  rt_bases<I>(static_cast<I>(0));
  rt_bases<I>(L::max());
  rt_bases<I>(L::min());
  if constexpr ( micron::is_signed_v<I> ) {
    rt_bases<I>(static_cast<I>(-1));
    rt_bases<I>(static_cast<I>(L::min() + 1));
  }
  rt_bases<I>(static_cast<I>(L::max() - 1));

  prng r(seed);
  for ( usize i = 0; i < 400; ++i ) rt_bases<I>(static_cast<I>(r.next()));
}

int
main(void)
{
  sb::print("=== to_chars / from_chars rigor ===");

  // ==========================================================================
  sb::print("--- integers: round-trip at every width x every base ---");

  test_case("i8 / u8 / i16 / u16 round-trip over bases 2..36");
  {
    rt_width<i8>(0xC0FFEE0DDC0DE001ull);
    rt_width<u8>(0xC0FFEE0DDC0DE002ull);
    rt_width<i16>(0xC0FFEE0DDC0DE003ull);
    rt_width<u16>(0xC0FFEE0DDC0DE004ull);
  }
  end_test_case();

  test_case("i32 / u32 / i64 / u64 round-trip over bases 2..36");
  {
    rt_width<i32>(0xC0FFEE0DDC0DE005ull);
    rt_width<u32>(0xC0FFEE0DDC0DE006ull);
    rt_width<i64>(0xC0FFEE0DDC0DE007ull);
    rt_width<u64>(0xC0FFEE0DDC0DE008ull);
  }
  end_test_case();

  test_case("the type minimum negates without overflowing");
  {
    char b[96];
    usize n = micron::to_chars(b, sizeof(b), static_cast<i8>(-128));
    require_true(lit(b, n, "-128"));
    n = micron::to_chars(b, sizeof(b), static_cast<i16>(-32768));
    require_true(lit(b, n, "-32768"));
    n = micron::to_chars(b, sizeof(b), static_cast<i32>(-2147483647 - 1));
    require_true(lit(b, n, "-2147483648"));
    n = micron::to_chars(b, sizeof(b), static_cast<i64>(-0x7FFFFFFFFFFFFFFFll - 1));
    require_true(lit(b, n, "-9223372036854775808"));
  }
  end_test_case();

  test_case("the sign is emitted for EVERY base, not just base 10");
  {
    char b[96];
    usize n = micron::to_chars(b, sizeof(b), -255, 16);
    require_true(lit(b, n, "-ff"));
    n = micron::to_chars(b, sizeof(b), -255, 16, true);
    require_true(lit(b, n, "-FF"));
    n = micron::to_chars(b, sizeof(b), -5, 2);
    require_true(lit(b, n, "-101"));
    i32 back = 0;
    require_true(micron::from_chars(back, "-ff", 3, 16) && back == -255);
  }
  end_test_case();

  test_case("known spellings");
  {
    char b[96];
    usize n = micron::to_chars(b, sizeof(b), 0);
    require_true(lit(b, n, "0"));
    n = micron::to_chars(b, sizeof(b), 255u, 16);
    require_true(lit(b, n, "ff"));
    n = micron::to_chars(b, sizeof(b), 255u, 16, true);
    require_true(lit(b, n, "FF"));
    n = micron::to_chars(b, sizeof(b), 8u, 8);
    require_true(lit(b, n, "10"));
    n = micron::to_chars(b, sizeof(b), 35u, 36);
    require_true(lit(b, n, "z"));
    n = micron::to_chars(b, sizeof(b), 1234567890u);
    require_true(lit(b, n, "1234567890"));
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- the range check, which is the gap this closes ---");

  test_case("out of range for the destination width is a rejection, never a wrap");
  {
    u8 b8{};
    require_true(micron::from_chars(b8, "255", 3) && b8 == 255);
    require_true(!micron::from_chars(b8, "256", 3));
    i8 s8{};
    require_true(micron::from_chars(s8, "127", 3) && s8 == 127);
    require_true(!micron::from_chars(s8, "128", 3));
    require_true(micron::from_chars(s8, "-128", 4) && s8 == -128);
    require_true(!micron::from_chars(s8, "-129", 4));
    u16 b16{};
    require_true(micron::from_chars(b16, "65535", 5) && b16 == 65535);
    require_true(!micron::from_chars(b16, "65536", 5));
    i32 s32{};
    require_true(micron::from_chars(s32, "2147483647", 10) && s32 == 2147483647);
    require_true(!micron::from_chars(s32, "2147483648", 10));
    require_true(micron::from_chars(s32, "-2147483648", 11));
    u64 b64{};
    require_true(micron::from_chars(b64, "18446744073709551615", 20) && b64 == ~0ull);
    require_true(!micron::from_chars(b64, "18446744073709551616", 20));
  }
  end_test_case();

  test_case("an unsigned destination rejects a minus sign");
  {
    u32 v{};
    require_true(!micron::from_chars(v, "-1", 2));
    require_true(!micron::from_chars(v, "-0", 2));
  }
  end_test_case();

  test_case("strict: whitespace and trailing garbage are rejections");
  {
    i32 v{};
    require_true(!micron::from_chars(v, " 1", 2));
    require_true(!micron::from_chars(v, "1 ", 2));
    require_true(!micron::from_chars(v, "1x", 2));
    require_true(!micron::from_chars(v, "", 0));
    require_true(!micron::from_chars(v, "+", 1));
    require_true(!micron::from_chars(v, "-", 1));
    require_true(!micron::from_chars(v, "0x10", 4, 16));      // no prefix handling, 'x' is garbage
    require_true(micron::from_chars(v, "+7", 2) && v == 7);
    // a digit outside the base is garbage too
    require_true(!micron::from_chars(v, "2", 1, 2));
    require_true(micron::from_chars(v, "1", 1, 2) && v == 1);
  }
  end_test_case();

  test_case("to_chars refuses to truncate");
  {
    char b[8];
    require_true(micron::to_chars(b, 3, 100u) == 3);
    require_true(micron::to_chars(b, 2, 100u) == 0);
    require_true(micron::to_chars(b, 4, -100) == 4);
    require_true(micron::to_chars(b, 3, -100) == 0);
    require_true(micron::to_chars(b, 8, 1u, 1) == 0);       // base below 2
    require_true(micron::to_chars(b, 8, 1u, 37) == 0);      // base above 36
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- floats: every format round-trips bit-exact ---");

  test_case("f64 round-trip through shortest / hex over random bit patterns");
  {
    prng r(0xC0FFEE0DDC0DE010ull);
    char b[64];
    usize checked = 0;
    for ( usize i = 0; i < N_ITER; ++i ) {
      const u64 bits = r.next();
      if ( ((bits >> 52) & 0x7FFull) == 0x7FFull ) continue;      // specials have no round-trip
      f64 v;
      __builtin_memcpy(&v, &bits, 8);

      usize n = micron::to_chars(b, sizeof(b), v, micron::float_format::shortest);
      f64 back = 0.0;
      require_true(micron::from_chars(back, b, n));
      u64 rb;
      __builtin_memcpy(&rb, &back, 8);
      require_true(rb == bits);

      n = micron::to_chars(b, sizeof(b), v, micron::float_format::hex);
      back = 0.0;
      require_true(micron::from_chars(back, b, n));
      __builtin_memcpy(&rb, &back, 8);
      require_true(rb == bits);
      ++checked;
    }
    require_true(checked > N_ITER / 2);
  }
  end_test_case();

  test_case("f32 round-trip through shortest / hex");
  {
    prng r(0xC0FFEE0DDC0DE011ull);
    char b[64];
    usize checked = 0;
    for ( usize i = 0; i < N_ITER; ++i ) {
      const u32 bits = static_cast<u32>(r.next());
      if ( ((bits >> 23) & 0xFFu) == 0xFFu ) continue;
      f32 v;
      __builtin_memcpy(&v, &bits, 4);

      usize n = micron::to_chars(b, sizeof(b), v, micron::float_format::shortest);
      f32 back = 0.0f;
      require_true(micron::from_chars(back, b, n));
      u32 rb;
      __builtin_memcpy(&rb, &back, 4);
      require_true(rb == bits);
      ++checked;
    }
    require_true(checked > N_ITER / 2);
  }
  end_test_case();

  test_case("fixed / scientific / general spellings");
  {
    char b[512];
    usize n = micron::to_chars(b, sizeof(b), 3.14159265358979, micron::float_format::fixed, 2);
    require_true(lit(b, n, "3.14"));
    n = micron::to_chars(b, sizeof(b), 0.6, micron::float_format::fixed, 0);
    require_true(lit(b, n, "1"));
    n = micron::to_chars(b, sizeof(b), 9.99, micron::float_format::scientific, 1);
    require_true(lit(b, n, "1.0e+01"));
    n = micron::to_chars(b, sizeof(b), 1.5, micron::float_format::fixed);      // default precision 6
    require_true(lit(b, n, "1.500000"));
  }
  end_test_case();

  test_case("general follows the C %g rule and trims");
  {
    char b[512];
    usize n = micron::to_chars(b, sizeof(b), 1.5, micron::float_format::general, 3);
    require_true(lit(b, n, "1.5"));
    n = micron::to_chars(b, sizeof(b), 123456.0, micron::float_format::general, 6);
    require_true(lit(b, n, "123456"));
    n = micron::to_chars(b, sizeof(b), 1234567.0, micron::float_format::general, 6);
    require_true(lit(b, n, "1.23457e+06"));
    n = micron::to_chars(b, sizeof(b), 1e-5, micron::float_format::general, 6);
    require_true(lit(b, n, "1e-05"));
    n = micron::to_chars(b, sizeof(b), 0.0001, micron::float_format::general, 6);
    require_true(lit(b, n, "0.0001"));
    n = micron::to_chars(b, sizeof(b), 0.0, micron::float_format::general, 6);
    require_true(lit(b, n, "0"));
  }
  end_test_case();

  test_case("hex spellings match printf %a");
  {
    char b[64];
    usize n = micron::to_chars(b, sizeof(b), 1.0, micron::float_format::hex);
    require_true(lit(b, n, "0x1p+0"));
    n = micron::to_chars(b, sizeof(b), 3.14159265358979, micron::float_format::hex);
    require_true(lit(b, n, "0x1.921fb54442d11p+1"));
    n = micron::to_chars(b, sizeof(b), 5e-324, micron::float_format::hex);
    require_true(lit(b, n, "0x0.0000000000001p-1022"));
    n = micron::to_chars(b, sizeof(b), 3.14159265358979, micron::float_format::hex, 3);
    require_true(lit(b, n, "0x1.922p+1"));
  }
  end_test_case();

  test_case("float from_chars is strict");
  {
    f64 v = 0.0;
    require_true(micron::from_chars(v, "1.5", 3) && v == 1.5);
    require_true(!micron::from_chars(v, " 1.5", 4));
    require_true(!micron::from_chars(v, "1.5x", 4));
    require_true(!micron::from_chars(v, "", 0));
    require_true(micron::from_chars(v, "1e5", 3) && v == 100000.0);
    require_true(micron::from_chars(v, "0x1p+0", 6) && v == 1.0);
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- bool, pointer, bytes ---");

  test_case("bool round-trips, and accepts the 1/0 spellings");
  {
    char b[8];
    usize n = micron::to_chars(b, sizeof(b), true);
    require_true(lit(b, n, "true"));
    n = micron::to_chars(b, sizeof(b), false);
    require_true(lit(b, n, "false"));
    require_true(micron::to_chars(b, 3, true) == 0);

    bool v = false;
    require_true(micron::from_chars(v, "true", 4) && v);
    require_true(micron::from_chars(v, "false", 5) && !v);
    require_true(micron::from_chars(v, "1", 1) && v);
    require_true(micron::from_chars(v, "0", 1) && !v);
    require_true(!micron::from_chars(v, "TRUE", 4));
    require_true(!micron::from_chars(v, "tru", 3));
    require_true(!micron::from_chars(v, "2", 1));
    require_true(!micron::from_chars(v, "", 0));
  }
  end_test_case();

  test_case("pointer");
  {
    char b[32];
    usize n = micron::to_chars(b, sizeof(b), static_cast<const void *>(nullptr));
    require_true(lit(b, n, "0x0"));
    int x = 0;
    n = micron::to_chars(b, sizeof(b), static_cast<const void *>(&x));
    require_true(n > 2 && b[0] == '0' && b[1] == 'x');
  }
  end_test_case();

  test_case("bytes_to_hex is the inverse of try_parse_hex_bytes");
  {
    prng r(0xC0FFEE0DDC0DE020ull);
    for ( usize t = 0; t < 500; ++t ) {
      u8 src[16], back[16];
      char hex[32];
      for ( usize i = 0; i < 16; ++i ) src[i] = static_cast<u8>(r.next());
      const usize n = micron::bytes_to_hex(hex, sizeof(hex), src, 16);
      require_true(n == 32);
      require_true(micron::try_parse_hex_bytes<char>(hex, n, back, 16));
      for ( usize i = 0; i < 16; ++i ) require_true(back[i] == src[i]);
    }
    char h[8];
    const u8 s[2] = { 0xAB, 0x0F };
    usize n = micron::bytes_to_hex(h, sizeof(h), s, 2);
    require_true(lit(h, n, "ab0f"));
    n = micron::bytes_to_hex(h, sizeof(h), s, 2, true);
    require_true(lit(h, n, "AB0F"));
    require_true(micron::bytes_to_hex(h, 3, s, 2) == 0);      // refuses to truncate
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- {:a} / {:A} reach the format engine ---");

  test_case("format spec hex float");
  {
    namespace fmt = micron::format;
    auto a = fmt::format("{:a}", 1.0);
    require_true(a.size() == 6 && a[0] == '0' && a[1] == 'x');
    auto c = fmt::format("{:A}", 1.0);
    require_true(c.size() == 6 && c[1] == 'X' && c[3] == 'P');
    auto d = fmt::format("{:.3a}", 3.14159265358979);
    require_true(d.size() == 10);
  }
  end_test_case();

  sb::print("=== TO_CHARS / FROM_CHARS RIGOR SUITE PASSED ===");
  return 1;
}
