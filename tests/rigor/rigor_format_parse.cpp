// rigor_format_parse.cpp — text -> binary float, the input side of the Ryu pair
// (src/string/conversions/parse_float.hpp + the format::to_float / to_double porcelain).
//
// two independent oracles:
//   1. micron's own shortest-form writer. d2s/f2s emit a decimal that identifies the value
//      uniquely, so parse(write(v)) must be bit-equal to v for every finite v. this is the
//      property the old accumulate-and-divide parser could not satisfy at all -- it stopped at
//      the 'e' -- and f2s_buffered *always* emits scientific.
//   2. ryu_oracle::decodes_to, the 1408-bit big-int predicate already used by the writer suite:
//      "the correctly rounded (ties-to-even) f64 of m*10^e10 is exactly these bits".

#include "../../src/string/format.hpp"

#include "../support/format_rigor.hpp"
#include "../support/ryu_oracle.hpp"

using mtest::prng;
namespace ro = mtest::ryu_oracle;
namespace fmt = micron::format;
using sb::end_test_case;
using sb::require_true;
using sb::test_case;

#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_RT64 = 20000;
constexpr static const usize N_RT32 = 20000;
constexpr static const usize N_ORACLE = 6000;
constexpr static const usize N_ORACLE32 = 4000;
constexpr static const usize N_MONO = 4000;
#else
constexpr static const usize N_RT64 = 300000;
constexpr static const usize N_RT32 = 300000;
constexpr static const usize N_ORACLE = 80000;
constexpr static const usize N_ORACLE32 = 40000;
constexpr static const usize N_MONO = 50000;
#endif

static f64
f64_from_bits(u64 b)
{
  f64 x;
  __builtin_memcpy(&x, &b, 8);
  return x;
}

static f32
f32_from_bits(u32 b)
{
  f32 x;
  __builtin_memcpy(&x, &b, 4);
  return x;
}

// -Ofast merges compile-time +0.0 / -0.0 and folds inf/nan tests; launder every special through
// a volatile before it reaches the code under test (ISSUES.md, the d2s(+-0.0) CSE)
static f64
f64_opaque(u64 b)
{
  volatile u64 vb = b;
  u64 t = vb;
  return f64_from_bits(t);
}

static f32
f32_opaque(u32 b)
{
  volatile u32 vb = b;
  u32 t = vb;
  return f32_from_bits(t);
}

static u64
bits_of(f64 v)
{
  u64 b;
  __builtin_memcpy(&b, &v, 8);
  return b;
}

static u32
bits_of(f32 v)
{
  u32 b;
  __builtin_memcpy(&b, &v, 4);
  return b;
}

static usize
cstr_len(const char *s)
{
  usize n = 0;
  while ( s[n] ) ++n;
  return n;
}

// "<m>e<e10>", built here rather than through the writer so the oracle section does not lean on
// the code it is checking
static usize
render(u64 m, i32 e10, char *out)
{
  char tmp[24];
  usize n = 0;
  if ( m == 0 ) tmp[n++] = '0';
  while ( m ) {
    tmp[n++] = static_cast<char>('0' + (m % 10));
    m /= 10;
  }
  usize k = 0;
  for ( usize i = n; i-- > 0; ) out[k++] = tmp[i];
  out[k++] = 'e';
  if ( e10 < 0 ) {
    out[k++] = '-';
    e10 = -e10;
  }
  char et[8];
  usize en = 0;
  if ( e10 == 0 ) et[en++] = '0';
  while ( e10 ) {
    et[en++] = static_cast<char>('0' + (e10 % 10));
    e10 /= 10;
  }
  for ( usize i = en; i-- > 0; ) out[k++] = et[i];
  return k;
}

// a nonzero literal that lands on +/-0 or +/-inf saturated, and the strict tier reports that the
// way from_chars does -- false, with the correctly rounded value still written out
static bool
saturated64(u64 b)
{
  const u64 mag = b & 0x7FFFFFFFFFFFFFFFull;
  return mag == 0 || mag == 0x7FF0000000000000ull;
}

static bool
saturated32(u32 b)
{
  const u32 mag = b & 0x7FFFFFFFu;
  return mag == 0 || mag == 0x7F800000u;
}

int
main(void)
{
  sb::print("=== format parse (text -> float) rigor ===");

  // ===========================================================
  // SECTION 1 – round-trip against micron's own shortest-form writer
  // ===========================================================
  sb::print("--- d2s / f2s round-trip ---");

  test_case("d2s round-trip – random f64 bit patterns are bit-equal");
  {
    prng r(0x5EEDF00DD2500001ull);
    char buf[64];
    usize checked = 0;
    for ( usize i = 0; i < N_RT64; ++i ) {
      const u64 b = r.next();
      if (((b >> 52) & 0x7FF) == 0x7FF ) continue;      // inf/nan get their own case
      const usize n = micron::__impl::__ryu::d2s_buffered(f64_from_bits(b), buf);
      f64 back = 0.0;
      const bool ok = micron::try_parse_double(static_cast<const char *>(buf), n, back);
      require_true(ok == !saturated64(b));
      require_true(bits_of(back) == b);
      ++checked;
    }
    require_true(checked > N_RT64 / 2);
  }
  end_test_case();

  test_case("d2s round-trip – every finite exponent field x mantissa extremes");
  {
    char buf[64];
    const u64 mants[] = { 0ull, 1ull, 0x8000000000000ull, 0xFFFFFFFFFFFFFull };
    for ( u64 e = 0; e < 0x7FF; ++e )
      for ( u64 m : mants ) {
        const u64 b = (e << 52) | m;
        const usize n = micron::__impl::__ryu::d2s_buffered(f64_from_bits(b), buf);
        f64 back = 0.0;
        micron::try_parse_double(static_cast<const char *>(buf), n, back);
        require_true(bits_of(back) == b);
      }
  }
  end_test_case();

  test_case("f2s round-trip – random f32 bit patterns are bit-equal");
  {
    // f2s_buffered always emits scientific, with an uppercase E and no '+' on the exponent, so
    // this case is exactly the one the old parser could not do at all
    prng r(0x5EEDF00DF2500002ull);
    char buf[64];
    usize checked = 0;
    for ( usize i = 0; i < N_RT32; ++i ) {
      const u32 b = static_cast<u32>(r.next() >> 32);
      if (((b >> 23) & 0xFF) == 0xFF ) continue;
      const usize n = micron::__impl::__ryu::__f32::f2s_buffered(f32_from_bits(b), buf);
      f32 back = 0.0f;
      const bool ok = micron::try_parse_float(static_cast<const char *>(buf), n, back);
      require_true(ok == !saturated32(b));
      require_true(bits_of(back) == b);
      ++checked;
    }
    require_true(checked > N_RT32 / 2);
  }
  end_test_case();

  test_case("round-trip – signed zero keeps its sign, both widths");
  {
    char buf[64];
    const u64 zeros[] = { 0x0000000000000000ull, 0x8000000000000000ull };
    for ( u64 b : zeros ) {
      const usize n = micron::__impl::__ryu::d2s_buffered(f64_opaque(b), buf);
      f64 back = 1.0;
      require_true(micron::try_parse_double(static_cast<const char *>(buf), n, back));
      require_true(bits_of(back) == b);
    }
    const u32 zeros32[] = { 0x00000000u, 0x80000000u };
    for ( u32 b : zeros32 ) {
      const usize n = micron::__impl::__ryu::__f32::f2s_buffered(f32_opaque(b), buf);      // "0E0" / "-0E0"
      f32 back = 1.0f;
      require_true(micron::try_parse_float(static_cast<const char *>(buf), n, back));
      require_true(bits_of(back) == b);
    }
  }
  end_test_case();

  test_case("round-trip – Inf / -Inf / NaN");
  {
    char buf[64];
    const u64 infs[] = { 0x7FF0000000000000ull, 0xFFF0000000000000ull };
    for ( u64 b : infs ) {
      const usize n = micron::__impl::__ryu::d2s_buffered(f64_from_bits(b), buf);      // "Inf" / "-Inf"
      f64 back = 0.0;
      require_true(micron::try_parse_double(static_cast<const char *>(buf), n, back));
      require_true(bits_of(back) == b);
    }
    // d2s_buffered drops NaN's sign, so NaN round-trips by classification, not by bit pattern
    const usize n = micron::__impl::__ryu::d2s_buffered(f64_from_bits(0xFFF8000000000000ull), buf);
    f64 back = 0.0;
    require_true(micron::try_parse_double(static_cast<const char *>(buf), n, back));
    const u64 rb = bits_of(back);
    require_true((rb & 0x7FF0000000000000ull) == 0x7FF0000000000000ull);
    require_true((rb & 0x000FFFFFFFFFFFFFull) != 0);
  }
  end_test_case();

  // ===========================================================
  // SECTION 2 – correct rounding against the big-int oracle
  // ===========================================================
  sb::print("--- correct rounding vs ryu_oracle::decodes_to ---");

  test_case("random m * 10^e10 is correctly rounded (f64)");
  {
    prng r(0xC0FFEE0DDF00D003ull);
    char buf[64];
    usize checked = 0;
    for ( usize i = 0; i < N_ORACLE; ++i ) {
      const u64 m = r.next() % 10000000000000000000ull;
      if ( m == 0 ) continue;
      const i32 e10 = static_cast<i32>(r.next_in(641)) - 330;
      const usize n = render(m, e10, buf);
      f64 v = 0.0;
      const bool ok = micron::try_parse_double(static_cast<const char *>(buf), n, v);
      const u64 b = bits_of(v);
      if ( saturated64(b) ) {
        require_true(!ok);
        continue;
      }
      require_true(ok);
      require_true(ro::decodes_to(m, e10, ro::decompose64(b)));
      ++checked;
    }
    require_true(checked > N_ORACLE / 4);
  }
  end_test_case();

  test_case("random m * 10^e10 is correctly rounded (f32)");
  {
    prng r(0xC0FFEE0DDF00D004ull);
    char buf[64];
    usize checked = 0;
    for ( usize i = 0; i < N_ORACLE32; ++i ) {
      const u64 m = r.next() % 10000000000000000000ull;
      if ( m == 0 ) continue;
      const i32 e10 = static_cast<i32>(r.next_in(85)) - 50;
      const usize n = render(m, e10, buf);
      f32 v = 0.0f;
      const bool ok = micron::try_parse_float(static_cast<const char *>(buf), n, v);
      const u32 b = bits_of(v);
      if ( saturated32(b) ) {
        require_true(!ok);
        continue;
      }
      require_true(ok);
      require_true(ro::decodes_to(m, e10, ro::decompose32(b)));
      ++checked;
    }
    require_true(checked > N_ORACLE32 / 4);
  }
  end_test_case();

  test_case("monotonicity – a <= b as decimals implies parse(a) <= parse(b)");
  {
    prng r(0xC0FFEE0DDF00D005ull);
    char ba[64], bb[64];
    for ( usize i = 0; i < N_MONO; ++i ) {
      u64 ma = r.next() % 1000000000000000000ull;
      u64 mb = r.next() % 1000000000000000000ull;
      if ( ma > mb ) {
        const u64 t = ma;
        ma = mb;
        mb = t;
      }
      const i32 e10 = static_cast<i32>(r.next_in(400)) - 200;
      f64 va = 0.0, vb = 0.0;
      micron::try_parse_double(static_cast<const char *>(ba), render(ma, e10, ba), va);
      micron::try_parse_double(static_cast<const char *>(bb), render(mb, e10, bb), vb);
      require_true(va <= vb);
    }
  }
  end_test_case();

  test_case(">19 significant digits routes through the decimal tier and still rounds correctly");
  {
    // written as <digits><PAD zeros>e<e10-PAD>, which is m*10^e10 exactly but with far more than
    // 19 significant digits -- so eisel-lemire cannot bound it from the truncated significand and
    // has to hand over to the shift-and-round bignum. decodes_to(m, e10, ..) is still the oracle,
    // because the two spellings denote the same rational.
    constexpr usize PAD = 200;
    prng r(0xC0FFEE0DDF00D006ull);
    char buf[320];
    usize checked = 0;
    for ( usize i = 0; i < N_ORACLE / 4; ++i ) {
      const u64 m = r.next() % 1000000000000000000ull;
      if ( m == 0 ) continue;
      const i32 e10 = static_cast<i32>(r.next_in(400)) - 200;

      char head[64];
      const usize hn = render(m, 0, head);      // "<digits>e0"
      const usize digits = hn - 2;              // drop the "e0"
      usize k = 0;
      for ( usize j = 0; j < digits; ++j ) buf[k++] = head[j];
      for ( usize j = 0; j < PAD; ++j ) buf[k++] = '0';
      char tail[32];
      const usize tn = render(1, e10 - static_cast<i32>(PAD), tail);      // "1e<exp>"
      for ( usize j = 1; j < tn; ++j ) buf[k++] = tail[j];                // keep the "e<exp>"

      f64 v = 0.0;
      const bool ok = micron::try_parse_double(static_cast<const char *>(buf), k, v);
      const u64 b = bits_of(v);
      if ( saturated64(b) ) {
        require_true(!ok);
        continue;
      }
      require_true(ok);
      require_true(ro::decodes_to(m, e10, ro::decompose64(b)));
      ++checked;
    }
    require_true(checked > 0);
  }
  end_test_case();

  // ===========================================================
  // SECTION 3 – hard-coded torture
  // ===========================================================
  sb::print("--- correctly-rounded torture set ---");

  test_case("subnormal boundary, ties, and the classic hard cases");
  {
    struct {
      const char *s;
      u64 bits;
      bool ok;
    } cases[] = {
      { "4.9406564584124654e-324", 0x0000000000000001ull, true  },      // min subnormal
      { "5e-324", 0x0000000000000001ull, true  },      // its shortest form
      { "2.4703282292062327e-324", 0x0000000000000000ull, false },      // below half-min: to zero
      { "2.4703282292062328e-324", 0x0000000000000001ull, true  },      // just above
      { "1e-323", 0x0000000000000002ull, true  },
      { "2.2250738585072014e-308", 0x0010000000000000ull, true  },      // DBL_MIN
      { "2.2250738585072011e-308", 0x000FFFFFFFFFFFFFull, true  },      // the php/java hang case
      { "2.2250738585072013e-308", 0x0010000000000000ull, true  },
      { "1.7976931348623157e308", 0x7FEFFFFFFFFFFFFFull, true  },      // DBL_MAX
      { "1.7976931348623158e308", 0x7FEFFFFFFFFFFFFFull, true  },
      { "1.7976931348623159e308", 0x7FF0000000000000ull, false },      // overflow
      { "9007199254740993", 0x4340000000000000ull, true  },      // 2^53+1, exact tie
      { "1.0000000000000002", 0x3FF0000000000001ull, true  },
      { "8.98846567431158e307", 0x7FE0000000000000ull, true  },
      { "5.708990770823839e33", 0x46F19799812DEA11ull, true  },
      { "0.500000000000000166533453693773481063544750213623046875", 0x3FE0000000000002ull, true  },
      { "0.5000000000000000000000000000001", 0x3FE0000000000000ull, true  },
      { "9.999999999999999999e22", 0x44B52D02C7E14AF6ull, true  },
      { "1e23", 0x44B52D02C7E14AF6ull, true  },      // the classic double-rounding pair
      { "1e22", 0x4480F0CF064DD592ull, true  },
      { "0.1", 0x3FB999999999999Aull, true  },
      { "0.3", 0x3FD3333333333333ull, true  },
      { "123456789012345678901234567890", 0x45F8EE90FF6C373Eull, true  },
      { "3.14159265358979323846264338327950288", 0x400921FB54442D18ull, true  },
      { "4.450147717014403e-308", 0x0020000000000000ull, true  },
      { "0000000000000000000000001e0", 0x3FF0000000000000ull, true  },      // leading zeros
    };
    for ( auto &c : cases ) {
      f64 v = 0.0;
      const bool ok = micron::try_parse_double(c.s, cstr_len(c.s), v);
      require_true(ok == c.ok);
      require_true(bits_of(v) == c.bits);
    }
  }
  end_test_case();

  test_case("the u64 accumulator no longer wraps, and the exponent clamps instead");
  {
    // "1" followed by 64 zeros used to accumulate mod 2^64 and come out as exactly 0.0
    char buf[512];
    buf[0] = '1';
    for ( usize i = 1; i <= 64; ++i ) buf[i] = '0';
    f64 v = 0.0;
    require_true(micron::try_parse_double(static_cast<const char *>(buf), 65u, v));
    require_true(bits_of(v) == 0x4D384F03E93FF9F5ull);      // 1e64

    for ( usize i = 1; i <= 400; ++i ) buf[i] = '0';
    require_true(!micron::try_parse_double(static_cast<const char *>(buf), 401u, v));
    require_true(bits_of(v) == 0x7FF0000000000000ull);      // overflow, saturated

    // a 20-digit exponent must clamp, never wrap into a small one
    require_true(!micron::try_parse_double("1e99999999999999999999", 22u, v));
    require_true(bits_of(v) == 0x7FF0000000000000ull);
    require_true(!micron::try_parse_double("1e-99999999999999999999", 23u, v));
    require_true(bits_of(v) == 0x0000000000000000ull);
    // ...but a zero significand short-circuits it and stays an exact zero
    require_true(micron::try_parse_double("0e99999999999999999999", 22u, v));
    require_true(bits_of(v) == 0x0000000000000000ull);
  }
  end_test_case();

  test_case("hex floats");
  {
    struct {
      const char *s;
      u64 bits;
      bool ok;
    } cases[] = {
      { "0x1.8p3", 0x4028000000000000ull, true  },
      { "0X1P-1", 0x3FE0000000000000ull, true  },
      { "-0x0p0", 0x8000000000000000ull, true  },
      { "0x1.fffffffffffffp1023", 0x7FEFFFFFFFFFFFFFull, true  },
      { "0x1p1024", 0x7FF0000000000000ull, false },
      { "0x1p-1074", 0x0000000000000001ull, true  },
      { "0x1p-1075", 0x0000000000000000ull, false },
      { "0x10", 0x4030000000000000ull, true  },
    };
    for ( auto &c : cases ) {
      f64 v = 0.0;
      const bool ok = micron::try_parse_double(c.s, cstr_len(c.s), v);
      require_true(ok == c.ok);
      require_true(bits_of(v) == c.bits);
    }
  }
  end_test_case();

  test_case("inf / nan words, case-insensitive and signed");
  {
    struct {
      const char *s;
      u64 bits;
    } infs[] = {
      { "inf", 0x7FF0000000000000ull },      { "INF", 0x7FF0000000000000ull },       { "Inf", 0x7FF0000000000000ull },
      { "-inf", 0xFFF0000000000000ull },     { "-Inf", 0xFFF0000000000000ull },      { "infinity", 0x7FF0000000000000ull },
      { "INFINITY", 0x7FF0000000000000ull }, { "+Infinity", 0x7FF0000000000000ull },
    };
    for ( auto &c : infs ) {
      f64 v = 0.0;
      require_true(micron::try_parse_double(c.s, cstr_len(c.s), v));
      require_true(bits_of(v) == c.bits);
    }
    const char *nans[] = { "nan", "NAN", "NaN", "-nan", "nan(0x1)", "NaN(1f)" };
    for ( const char *s : nans ) {
      f64 v = 0.0;
      require_true(micron::try_parse_double(s, cstr_len(s), v));
      const u64 b = bits_of(v);
      require_true((b & 0x7FF0000000000000ull) == 0x7FF0000000000000ull);
      require_true((b & 0x000FFFFFFFFFFFFFull) != 0);      // quiet bit at least
    }
  }
  end_test_case();

  test_case("f32 boundaries");
  {
    struct {
      const char *s;
      u32 bits;
      bool ok;
    } cases[] = {
      { "5E-1", 0x3F000000u, true  },
      { "0E0", 0x00000000u, true  },
      { "1e-45", 0x00000001u, true  },
      { "3.4028235e38", 0x7F7FFFFFu, true  },
      { "1e39", 0x7F800000u, false },
      { "1e-46", 0x00000000u, false },
      { "16777217", 0x4B800000u, true  },
    };
    for ( auto &c : cases ) {
      f32 v = 0.0f;
      const bool ok = micron::try_parse_float(c.s, cstr_len(c.s), v);
      require_true(ok == c.ok);
      require_true(bits_of(v) == c.bits);
    }
  }
  end_test_case();

  // ===========================================================
  // SECTION 4 – the strict tier: rejection and the option surface
  // ===========================================================
  sb::print("--- strict tier ---");

  test_case("malformed input is rejected outright");
  {
    const char *bad[] = { "", "+", "-", ".", "+.", "e5", "E5", ".e5", "1e", "1e+", "1.5abc", "abc", "--1", "1..2", "0x", "0x1p", "1e5 x", " ", "\t" };
    for ( const char *s : bad ) {
      f64 v = 1.0;
      require_true(!micron::try_parse_double(s, cstr_len(s), v));
      auto o = fmt::parse_double(s, cstr_len(s));
      require_true(o.is_second());
      require_true(o.cast<fmt::parse_error>().code == micron::error::invalid_arg);
    }
    f64 v = 1.0;
    require_true(!micron::try_parse_double(static_cast<const char *>(nullptr), 4u, v));
    require_true(!micron::try_parse_double("1.5", 0u, v));
  }
  end_test_case();

  test_case("well-formed input is accepted, leading blanks and all");
  {
    struct {
      const char *s;
      f64 v;
    } good[] = {
      { "1e5", 100000.0 },  { "5E-1", 0.5 },   { " \t1.5", 1.5 }, { "+1e5", 100000.0 },
      { "1E+5", 100000.0 }, { "0x1.8p3", 12.0 }, { "-2.5", -2.5 },  { "1.", 1.0 },
    };
    for ( auto &c : good ) {
      f64 v = 0.0;
      require_true(micron::try_parse_double(c.s, cstr_len(c.s), v));
      require_true(v == c.v);
      auto o = fmt::parse_double(c.s, cstr_len(c.s));
      require_true(o.is_first());
      require_true(o.cast<f64>() == c.v);
    }
  }
  end_test_case();

  test_case("option error alternative – has_value() is true for BOTH, success is is_first()");
  {
    auto bad = fmt::parse_double("abc", 3u);
    require_true(bad.has_value());      // the trap: this is true on the error branch too
    require_true(!bad.is_first());
    require_true(bad.is_second());
    require_true(static_cast<bool>(bad.cast<fmt::parse_error>()));
    require_true(bad.cast<fmt::parse_error>().message() != nullptr);

    auto ok = fmt::parse_double("1.5", 3u);
    require_true(ok.has_value());
    require_true(ok.is_first());
    require_true(!ok.is_second());
    require_true(ok.cast<f64>() == 1.5);

    auto over = fmt::parse_double("1e400", 5u);
    require_true(over.is_second());
    require_true(over.cast<fmt::parse_error>().code == micron::error::not_representable);

    require_true(!static_cast<bool>(fmt::parse_error{}));
  }
  end_test_case();

  test_case("out-of-range still writes the correctly rounded saturated value");
  {
    f64 v = 1.0;
    require_true(!micron::try_parse_double("1e400", 5u, v));
    require_true(bits_of(v) == 0x7FF0000000000000ull);
    require_true(!micron::try_parse_double("-1e400", 6u, v));
    require_true(bits_of(v) == 0xFFF0000000000000ull);
    require_true(!micron::try_parse_double("-1e-400", 7u, v));
    require_true(bits_of(v) == 0x8000000000000000ull);      // signed zero survives underflow
  }
  end_test_case();

  test_case("bounded length cuts the literal where it says it does");
  {
    f64 v = 0.0;
    require_true(!micron::try_parse_double("1e5", 2u, v));      // "1e" is malformed
    require_true(micron::try_parse_double("1.5abc", 3u, v));
    require_true(v == 1.5);
    require_true(micron::try_parse_double("9.9999", 3u, v));
    require_true(v == 9.9);
  }
  end_test_case();

  test_case("the option and array/container overloads agree with the pointer ones");
  {
    require_true(fmt::parse_double("1e5").is_first());
    require_true(fmt::parse_double("1e5").cast<f64>() == 100000.0);
    require_true(fmt::parse_float("5E-1").cast<f32>() == 0.5f);

    micron::hstring<schar> s("2.5e3");
    require_true(fmt::parse_double(s).is_first());
    require_true(fmt::parse_double(s).cast<f64>() == 2500.0);

    f64 d = 0.0;
    require_true(micron::try_string_to_double(s, d));
    require_true(d == 2500.0);
  }
  end_test_case();

  // ===========================================================
  // SECTION 5 – the legacy prefix contract is unchanged
  // ===========================================================
  sb::print("--- legacy to_float / to_double contract ---");

  test_case("to_* still prefix-parses and still answers 0.0 silently");
  {
    require_true(fmt::to_double(static_cast<const char *>(nullptr)) == 0.0);
    require_true(fmt::to_float(static_cast<const char *>(nullptr)) == 0.0f);
    require_true(fmt::to_double("", 0u) == 0.0);
    require_true(fmt::to_double("abc") == 0.0);
    require_true(fmt::to_double("1.5abc") == 1.5);      // prefix, not a rejection
    require_true(fmt::to_float("3.14159", 4u) == 3.14f);
    require_true(fmt::to_double("9.9999", 3u) == 9.9);
    // a total failure is +0, never -0 -- the sign is or'd into the sign bit, never applied as
    // a unary minus that -fno-signed-zeros could drop
    require_true(bits_of(fmt::to_double("-abc")) == 0x0000000000000000ull);
    require_true(bits_of(fmt::to_double("-0.0")) == 0x8000000000000000ull);
  }
  end_test_case();

  test_case("to_* now applies the exponent -- the defect this suite exists for");
  {
    require_true(fmt::to_double("1e5") == 100000.0);
    require_true(fmt::to_double("1.5e10") == 1.5e10);
    require_true(fmt::to_double("1e-5") == 1e-5);
    require_true(fmt::to_float("5E-1") == 0.5f);
    require_true(fmt::to_double("1e+10") == 1e10);
    micron::hstring<schar> s("1e5");
    require_true(fmt::to_double(s) == 100000.0);
    micron::sstring<32> ss("2.5E2");
    require_true(fmt::to_double(ss) == 250.0);
  }
  end_test_case();

  sb::print("=== FORMAT PARSE RIGOR SUITE PASSED ===");
  return 1;
}
