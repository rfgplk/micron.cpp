// rigor_format_write.cpp — float -> text at a REQUESTED precision, the %f / %e side.
//
// The shortest-form writers already have a suite (rigor_format_ryu.cpp) and the parse direction
// has one (rigor_format_parse.cpp). This is the third corner, and it is the one that was missing:
// tests/rigor/rigor_format_float.cpp only ever asserted SHAPES ("renders 3 decimal places",
// "contains '5'"), which is precisely why d2f_buffered/d2e_buffered shipped for so long
// truncating the digit stream instead of rounding it, and padding '0' past the shortest form
// rather than continuing the true expansion.
//
// The oracle is tests/support/exact_decimal.hpp: a base-10^9 bignum that reaches the same answer
// by a different road -- v = m2 * 5^|e2| / 10^|e2|, one big integer multiply, point placed by
// arithmetic -- sharing no code with the base-10 shift machinery under test.
//
// Every hard-coded expectation below was checked against glibc printf on amd64.
//
// snowball convention: exit 1 == success.

#include "../../src/string/format.hpp"

#include "../support/exact_decimal.hpp"
#include "../support/oracles.hpp"

using mtest::prng;
namespace ed = mtest::exact_decimal;
namespace ry = micron::__impl::__ryu;
using sb::end_test_case;
using sb::require_true;
using sb::test_case;

#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_FIXED = 1500;
constexpr static const usize N_SCI = 1500;
#else
constexpr static const usize N_FIXED = 30000;
constexpr static const usize N_SCI = 30000;
#endif

struct dparts {
  u64 m2;
  i32 e2;
  bool neg;
  bool finite;
};

static dparts
decompose(u64 bits)
{
  dparts r{ 0, 0, false, true };
  r.neg = (bits >> 63) != 0;
  const u32 be = static_cast<u32>((bits >> 52) & 0x7FFu);
  const u64 mf = bits & ((1ull << 52) - 1);
  if ( be == 0x7FFu ) {
    r.finite = false;
    return r;
  }
  if ( be == 0 ) {
    r.m2 = mf;
    r.e2 = 1 - 1023 - 52;
  } else {
    r.m2 = mf | (1ull << 52);
    r.e2 = static_cast<i32>(be) - 1023 - 52;
  }
  return r;
}

static f64
f64_from_bits(u64 b)
{
  f64 x;
  __builtin_memcpy(&x, &b, 8);
  return x;
}

// -Ofast folds compile-time specials; launder every one through a volatile (see ISSUES.md and
// rigor_format_parse.cpp:54-70)
static f64
f64_opaque(u64 b)
{
  volatile u64 vb = b;
  const u64 t = vb;
  return f64_from_bits(t);
}

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

// hstring content == literal
static bool
eq(const micron::hstring<schar> &h, const char *lit)
{
  const usize n = cstr_len(lit);
  if ( h.size() != n ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( h[i] != lit[i] ) return false;
  return true;
}

int
main(void)
{
  sb::print("=== format write (float -> text at precision) rigor ===");

  // ==========================================================================
  sb::print("--- the defects this suite exists for ---");

  test_case("%f rounds the last kept digit instead of truncating it");
  {
    char b[64];
    usize n = ry::d2f_buffered(0.6, b, 64, 0);
    require_true(same(b, n, "1", 1));      // was "0"

    n = ry::d2f_buffered(3.14159265358979, b, 64, 6);
    require_true(same(b, n, "3.141593", 8));      // was "3.141592"
  }
  end_test_case();

  test_case("%f continues the TRUE expansion past the shortest form");
  {
    char b[128];
    usize n = ry::d2f_buffered(1.0 / 3.0, b, 128, 20);
    require_true(same(b, n, "0.33333333333333331483", 22));      // was "0.33333333333333330000"

    n = ry::d2f_buffered(0.1, b, 128, 20);
    require_true(same(b, n, "0.10000000000000000555", 22));
  }
  end_test_case();

  test_case("%e re-derives the exponent AFTER rounding");
  {
    char b[64];
    usize n = ry::d2e_buffered(9.99, b, 64, 1);
    require_true(same(b, n, "1.0e+01", 7));      // was "9.9e+00"

    n = ry::d2e_buffered(1.7976931348623157e308, b, 64, 3);
    require_true(same(b, n, "1.798e+308", 10));      // was "1.797e+308"
  }
  end_test_case();

  test_case("the shortest form is a ROUNDING and must not be re-rounded (0.35 at %.1f)");
  {
    // Ryu shortest of 0.35 is "35e-2"; cutting that at one fraction digit looks like an exact tie
    // and ties-to-even on the odd 3 would answer "0.4". The exact value is 0.34999999999999997779..
    // so the answer is "0.3". This is the case that makes a shortest-form fast path unsound.
    char b[64];
    const usize n = ry::d2f_buffered(0.35, b, 64, 1);
    require_true(same(b, n, "0.3", 3));
  }
  end_test_case();

  test_case("sign comes from the sign BIT, so -0.0 keeps it");
  {
    char b[64];
    const f64 nz = f64_opaque(0x8000000000000000ull);
    usize n = ry::d2f_buffered(nz, b, 64, 2);
    require_true(same(b, n, "-0.00", 5));      // `val < 0.0` is false for -0.0, so this was "0.00"

    n = ry::d2e_buffered(nz, b, 64, 2);
    require_true(same(b, n, "-0.00e+00", 9));
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- ties-to-even, the rule glibc uses in FE_TONEAREST ---");

  test_case("exact halves round to even");
  {
    char b[64];
    struct {
      f64 v;
      const char *want;
    } cases[] = {
      { 0.5, "0"  },
      { 1.5, "2"  },
      { 2.5, "2"  },
      { 3.5, "4"  },
      { 4.5, "4"  },
      { -0.5, "-0" },
      { -1.5, "-2" },
    };
    for ( auto &c : cases ) {
      const usize n = ry::d2f_buffered(c.v, b, 64, 0);
      require_true(same(b, n, c.want, cstr_len(c.want)));
    }
  }
  end_test_case();

  test_case("a carry that escapes the leading digit bumps the exponent");
  {
    char b[64];
    usize n = ry::d2f_buffered(9.999, b, 64, 2);
    require_true(same(b, n, "10.00", 5));

    n = ry::d2e_buffered(9.999, b, 64, 2);
    require_true(same(b, n, "1.00e+01", 8));

    n = ry::d2e_buffered(0.0999, b, 64, 1);
    require_true(same(b, n, "1.0e-01", 7));
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- against the independent base-10^9 oracle ---");

  test_case("%f matches the exact expansion over random bit patterns x precision");
  {
    prng r(0x5EEDF00DF12ED001ull);
    char got[1400], want[1400];
    usize checked = 0;
    for ( usize i = 0; i < N_FIXED; ++i ) {
      const u64 bits = r.next();
      const dparts p = decompose(bits);
      if ( !p.finite ) continue;
      const u32 prec = static_cast<u32>(r.next_in(25));
      const f64 v = f64_from_bits(bits);

      const usize gn = ry::d2f_buffered(v, got, 1400, prec);
      const usize wn = ed::fixed(p.m2, p.e2, p.neg, prec, want);
      require_true(same(got, gn, want, wn));
      ++checked;
    }
    require_true(checked > N_FIXED / 2);
  }
  end_test_case();

  test_case("%e matches the exact expansion over random bit patterns x precision");
  {
    prng r(0x5EEDF00DF12ED002ull);
    char got[1400], want[1400];
    usize checked = 0;
    for ( usize i = 0; i < N_SCI; ++i ) {
      const u64 bits = r.next();
      const dparts p = decompose(bits);
      if ( !p.finite ) continue;
      const u32 prec = static_cast<u32>(r.next_in(25));
      const f64 v = f64_from_bits(bits);

      const usize gn = ry::d2e_buffered(v, got, 1400, prec);
      const usize wn = ed::sci(p.m2, p.e2, p.neg, prec, want);
      require_true(same(got, gn, want, wn));
      ++checked;
    }
    require_true(checked > N_SCI / 2);
  }
  end_test_case();

  test_case("tier 1 (u64 kernel) agrees byte-for-byte with tier 2 (bignum)");
  {
    // the strongest cheap check in the suite: wherever the fast kernel claims `ok`, its string
    // must equal what the exact expansion produces. no external oracle needed, and it covers the
    // tie-parity trap the kernel is most likely to get wrong (%.0f of 1.5 must consult the parity
    // of the INTEGER digit, not of the fractional quotient).
    prng r(0x5EEDF00DF12ED003ull);
    char fast[1400], slow[1400];
    usize covered = 0;
    for ( usize i = 0; i < N_FIXED; ++i ) {
      const u64 bits = r.next();
      const dparts p = decompose(bits);
      if ( !p.finite ) continue;
      const u32 prec = static_cast<u32>(r.next_in(20));

      const ry::__fx::fixed64 k = ry::__fx::__fixed_u64(p.m2, p.e2, prec);
      if ( !k.ok ) continue;
      const usize fn = ry::__fx::__emit_fixed64(fast, 1400, p.neg, k.ip, k.fp, prec);

      char dig[1400];
      u32 dn = 0;
      i32 pt = 0;
      ry::__fx::__exact_round(p.m2, p.e2, ry::__fx::cut::fraction, static_cast<i32>(prec), dig, dn, pt);
      const usize sn = ry::__fx::__emit_fixed(slow, 1400, p.neg, dig, dn, pt, prec);

      require_true(same(fast, fn, slow, sn));
      ++covered;
    }
    require_true(covered > N_FIXED / 4);      // the kernel must actually be taking the work
  }
  end_test_case();

  test_case("tier 1 agrees with tier 2 on the exact-tie grid");
  {
    // halves, quarters and eighths are where the kernel's rounding decision is an exact tie
    char fast[1400], slow[1400];
    for ( i32 num = -64; num <= 64; ++num )
      for ( i32 den : { 1, 2, 4, 8, 16 } )
        for ( u32 prec = 0; prec <= 6; ++prec ) {
          const f64 v = static_cast<f64>(num) / static_cast<f64>(den);
          const u64 bits = micron::math::ieee::to_bits<f64>(v);
          const dparts p = decompose(bits);
          const ry::__fx::fixed64 k = ry::__fx::__fixed_u64(p.m2, p.e2, prec);
          if ( !k.ok ) continue;
          const usize fn = ry::__fx::__emit_fixed64(fast, 1400, p.neg, k.ip, k.fp, prec);
          char dig[1400];
          u32 dn = 0;
          i32 pt = 0;
          ry::__fx::__exact_round(p.m2, p.e2, ry::__fx::cut::fraction, static_cast<i32>(prec), dig, dn, pt);
          const usize sn = ry::__fx::__emit_fixed(slow, 1400, p.neg, dig, dn, pt, prec);
          require_true(same(fast, fn, slow, sn));
        }
  }
  end_test_case();

  test_case("%f over the whole exponent range with edge mantissas");
  {
    const u64 mants[] = { 0ull, 1ull, 0x8000000000000ull, 0xFFFFFFFFFFFFFull };
    const u32 precs[] = { 0u, 1u, 6u, 17u, 25u };
    char got[1400], want[1400];
    for ( u32 be = 0; be < 0x7FFu; ++be )
      for ( u64 m : mants )
        for ( u32 prec : precs ) {
          const u64 bits = (static_cast<u64>(be) << 52) | m;
          const dparts p = decompose(bits);
          const f64 v = f64_from_bits(bits);
          const usize gn = ry::d2f_buffered(v, got, 1400, prec);
          const usize wn = ed::fixed(p.m2, p.e2, p.neg, prec, want);
          require_true(same(got, gn, want, wn));
        }
  }
  end_test_case();

  test_case("subnormal extremes at high precision");
  {
    char got[1400], want[1400];
    const u64 bits[] = { 1ull, 2ull, 0xFFFFFFFFFFFFFull, 0x10000000000000ull };
    const u32 precs[] = { 0u, 6u, 320u, 330u, 400u };
    for ( u64 b : bits )
      for ( u32 prec : precs ) {
        const dparts p = decompose(b);
        const f64 v = f64_from_bits(b);
        const usize gn = ry::d2f_buffered(v, got, 1400, prec);
        const usize wn = ed::fixed(p.m2, p.e2, p.neg, prec, want);
        require_true(same(got, gn, want, wn));
      }
  }
  end_test_case();

  test_case("DBL_MAX renders all 309 integer digits");
  {
    char got[1400], want[1400];
    const u64 bits = 0x7FEFFFFFFFFFFFFFull;
    const dparts p = decompose(bits);
    const f64 v = f64_from_bits(bits);
    for ( u32 prec : { 0u, 1u, 6u, 20u } ) {
      const usize gn = ry::d2f_buffered(v, got, 1400, prec);
      const usize wn = ed::fixed(p.m2, p.e2, p.neg, prec, want);
      require_true(same(got, gn, want, wn));
      require_true(gn >= 309);
    }
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- specials and the buffer contract ---");

  test_case("NaN / Inf spellings are unchanged");
  {
    char b[64];
    usize n = ry::d2f_buffered(f64_opaque(0x7FF8000000000000ull), b, 64, 6);
    require_true(same(b, n, "NaN", 3));
    n = ry::d2f_buffered(f64_opaque(0x7FF0000000000000ull), b, 64, 6);
    require_true(same(b, n, "Inf", 3));
    n = ry::d2f_buffered(f64_opaque(0xFFF0000000000000ull), b, 64, 6);
    require_true(same(b, n, "-Inf", 4));
    n = ry::d2e_buffered(f64_opaque(0x7FF0000000000000ull), b, 64, 6);
    require_true(same(b, n, "Inf", 3));
  }
  end_test_case();

  test_case("a buffer that cannot hold the result answers 0, never a truncated string");
  {
    char b[8];
    // 1e300 at %.6f needs 309 + 1 + 6 bytes
    require_true(ry::d2f_buffered(1e300, b, 8, 6) == 0);
    require_true(ry::d2e_buffered(1e300, b, 8, 6) == 0);
    // exactly-fitting and one-short
    char c[16];
    require_true(ry::d2f_buffered(1.5, c, 4, 2) == 4);      // "1.50"
    require_true(ry::d2f_buffered(1.5, c, 3, 2) == 0);
  }
  end_test_case();

  test_case("d2f_size / d2e_size bound the writers");
  {
    char b[1400];
    const f64 vals[] = { 0.0, 1.5, -273.15, 1e300, 5e-324, 1.7976931348623157e308 };
    for ( f64 v : vals )
      for ( u32 prec : { 0u, 6u, 30u } ) {
        const usize need = ry::d2f_size(v, prec);
        const usize n = ry::d2f_buffered(v, b, 1400, prec);
        require_true(n <= need);
        require_true(ry::d2f_buffered(v, b, need, prec) == n);
      }
  }
  end_test_case();

  // ==========================================================================
  sb::print("--- the hstring porcelain agrees with the buffered writers ---");

  test_case("to_fixed / to_scientific / to_fixed_trim");
  {
    require_true(eq(micron::to_fixed(0.6, 0u), "1"));
    require_true(eq(micron::to_fixed(1.0 / 3.0, 20u), "0.33333333333333331483"));
    require_true(eq(micron::to_scientific(9.99, 1u), "1.0e+01"));
    require_true(eq(micron::to_fixed_trim(1.5, 6u), "1.5"));
    require_true(eq(micron::to_fixed_trim(2.0, 6u), "2"));
    require_true(eq(micron::to_fixed(-273.15, 2u), "-273.15"));
  }
  end_test_case();

  test_case("format {:.Nf} / {:.Ne} route through the same writers");
  {
    namespace fmt = micron::format;
    require_true(eq(fmt::format("{:.0f}", 0.6), "1"));
    require_true(eq(fmt::format("{:.6f}", 3.14159265358979), "3.141593"));
    require_true(eq(fmt::format("{:.1e}", 9.99), "1.0e+01"));
    require_true(eq(fmt::format("{:.1f}", 0.35), "0.3"));
  }
  end_test_case();

  sb::print("=== FORMAT WRITE RIGOR SUITE PASSED ===");
  return 1;
}
