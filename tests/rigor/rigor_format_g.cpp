// rigor_format_g.cpp -- {:g} / {:G} / {:E}, and the format-vs-to_chars agreement they broke.
//
// conversions/fixed.hpp grew a d2g_buffered implementing the real C %g rule, but format.hpp's
// 'g' arm was never repointed at it: it kept recovering the exponent by re-parsing
// d2s_buffered's SHORTEST text, and when d2s chose fixed form (no 'e') it fell straight through
// to d2f with the RAW precision. So {:.6g} of 123456.0 answered "123456.000000" and {:.3g} of 1.5
// answered "1.500" -- neither is %g -- while to_chars(..., float_format::general) answered
// correctly. Two spellings of the same thing, two different answers.
//
// {:G} had a second defect on the same line: it routed to d2e_buffered, which only ever writes a
// lowercase 'e', so {:G} of 1e-10 came out "1.00000e-10". {:E} had it too.
//
// ORACLE. Not d2g_buffered -- that would be circular. The C rule is:
//
//   P = precision (0 means 1); round to P SIGNIFICANT digits; take the exponent X of the RESULT;
//   -4 <= X < P  ->  fixed with P-1-X fraction digits,  else scientific with P-1;
//   then strip trailing fractional zeros (and a bare '.') unless '#'.
//
// so the oracle is built out of to_scientific/to_fixed -- the %e and %f writers, which are exact
// and pinned independently by rigor_conversions_bufsz.cpp and format_values.cpp. It reads X back
// out of the scientific form rather than computing it, which is exactly the step the broken code
// got wrong by reading it out of the SHORTEST form instead.

#include "../../src/string/format.hpp"
#include "../../src/string/strings.hpp"

#include "../support/oracles.hpp"

using mtest::prng;
using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_FUZZ = 4000;
#else
constexpr static const usize N_FUZZ = 60000;
#endif

static f64
f64_from_bits(u64 b) noexcept
{
  f64 x;
  __builtin_memcpy(&x, &b, 8);
  return x;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the oracle

static micron::hstring<schar>
g_oracle(f64 v, u32 precision, bool alt, bool upper)
{
  const u32 P = precision == 0 ? 1u : precision;

  // round to P significant digits and read the exponent back out of the result. taking X AFTER
  // the rounding is the whole subtlety: 9.99 at P=2 rounds to 1.0e+01, so X is 1 and not 0.
  micron::hstring<char> sci = micron::to_scientific(v, P - 1u);

  // specials pass straight through -- to_scientific already prints NaN/Inf/-Inf
  bool has_e = false;
  usize epos = 0;
  for ( usize i = 0; i < sci.size(); ++i )
    if ( sci[i] == 'e' ) {
      has_e = true;
      epos = i;
      break;
    }
  if ( !has_e ) return micron::hstring<schar>(sci.c_str());

  i32 X = 0;
  bool eneg = sci[epos + 1] == '-';
  for ( usize j = epos + 2; j < sci.size(); ++j ) X = X * 10 + (sci[j] - '0');
  if ( eneg ) X = -X;

  micron::hstring<schar> out;
  if ( X >= -4 && X < static_cast<i32>(P) ) {
    micron::hstring<char> fx = micron::to_fixed(v, static_cast<u32>(static_cast<i32>(P) - 1 - X));
    out = micron::hstring<schar>(fx.c_str());
  } else {
    out = micron::hstring<schar>(sci.c_str());
  }

  if ( !alt ) {
    // strip trailing fractional zeros, and a bare '.' with them. the exponent tail, if any,
    // survives and is pulled back down.
    usize dot = out.size(), ep = out.size();
    for ( usize i = 0; i < out.size(); ++i ) {
      if ( out[i] == '.' )
        dot = i;
      else if ( out[i] == 'e' ) {
        ep = i;
        break;
      }
    }
    if ( dot != out.size() ) {
      usize end = ep;
      while ( end > dot + 1 && out[end - 1] == '0' ) --end;
      if ( end == dot + 1 ) end = dot;
      micron::hstring<schar> t;
      for ( usize i = 0; i < end; ++i ) t += out[i];
      for ( usize i = ep; i < out.size(); ++i ) t += out[i];
      out = t;
    }
  }
  if ( upper )
    for ( usize i = 0; i < out.size(); ++i )
      if ( out[i] == 'e' ) out[i] = 'E';
  return out;
}

static bool
same(const micron::hstring<schar> &a, const micron::hstring<schar> &b)
{
  if ( a.size() != b.size() ) return false;
  for ( usize i = 0; i < a.size(); ++i )
    if ( a[i] != b[i] ) return false;
  return true;
}

int
main()
{
  prng rng(0x9e3779b97f4a7c15ULL);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("the named %g regressions");
  {
    require_true(same(micron::format::format("{:.6g}", 123456.0), micron::hstring<schar>("123456")));
    require_true(same(micron::format::format("{:.3g}", 1.5), micron::hstring<schar>("1.5")));
    require_true(same(micron::format::format("{:g}", 0.0001), micron::hstring<schar>("0.0001")));
    require_true(same(micron::format::format("{:.0g}", 1234.0), micron::hstring<schar>("1e+03")));
    require_true(same(micron::format::format("{:G}", 1e-10), micron::hstring<schar>("1E-10")));
    require_true(same(micron::format::format("{:.1g}", 9.99), micron::hstring<schar>("1e+01")));
    require_true(same(micron::format::format("{:.2g}", 9.99), micron::hstring<schar>("10")));
    // the -4 boundary, both sides
    require_true(same(micron::format::format("{:.6g}", 0.0001), micron::hstring<schar>("0.0001")));
    require_true(same(micron::format::format("{:.6g}", 0.00001), micron::hstring<schar>("1e-05")));
    // '#' keeps the zeros
    require_true(same(micron::format::format("{:#.6g}", 1.5), micron::hstring<schar>("1.50000")));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("{:E} and {:A} carry their case, as {:G} now does");
  {
    require_true(same(micron::format::format("{:E}", 1.5), micron::hstring<schar>("1.500000E+00")));
    require_true(same(micron::format::format("{:e}", 1.5), micron::hstring<schar>("1.500000e+00")));
    micron::hstring<schar> A = micron::format::format("{:A}", 1.5);
    micron::hstring<schar> a = micron::format::format("{:a}", 1.5);
    bool upperA = false, lowera = false;
    for ( usize i = 0; i < A.size(); ++i )
      if ( A[i] == 'P' || A[i] == 'X' ) upperA = true;
    for ( usize i = 0; i < a.size(); ++i )
      if ( a[i] == 'p' || a[i] == 'x' ) lowera = true;
    require_true(upperA && lowera);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("format({:g}) and to_chars(general) agree -- they used to not");
  {
    char buf[512];
    for ( usize i = 0; i < N_FUZZ; ++i ) {
      f64 v;
      switch ( i % 5 ) {
      case 0:
        v = static_cast<f64>(static_cast<i64>(rng.next() % 2000001ull) - 1000000);
        break;
      case 1:
        v = f64_from_bits(rng.next() & 0x7FEFFFFFFFFFFFFFull);      // finite, any magnitude
        break;
      case 2:
        v = static_cast<f64>(rng.next()) * 1e-9;
        break;
      case 3:
        v = 1.0 / static_cast<f64>((rng.next() % 999999ull) + 1ull);
        break;
      default:
        v = static_cast<f64>(rng.next() % 1000000ull) / 10000.0;
        break;
      }
      const u32 prec = static_cast<u32>(rng.next() % 18ull);

      // there is no runtime-precision spelling ({:.*}), so the spec is built as text
      micron::hstring<schar> spec("{:.");
      micron::hstring<char> ps = micron::to_string<u32>(prec);
      spec.append(ps.c_str(), ps.size());
      spec += 'g';
      spec += '}';

      micron::hstring<schar> got = micron::format::format(spec.c_str(), v);
      micron::hstring<schar> want = g_oracle(v, prec, false, false);
      require_true(same(got, want));

      // and to_chars must give the identical bytes for the same request
      const usize n = micron::to_chars(buf, sizeof(buf), v, micron::float_format::general, prec);
      require_true(n == want.size());
      for ( usize k = 0; k < n; ++k ) require_true(buf[k] == want[k]);
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: {:#g} keeps the zeros, {:G} upcases");
  {
    for ( usize i = 0; i < N_FUZZ / 4; ++i ) {
      const f64 v = f64_from_bits(rng.next() & 0x7FEFFFFFFFFFFFFFull);
      const u32 prec = static_cast<u32>(rng.next() % 15ull);

      micron::hstring<schar> spec("{:#.");
      micron::hstring<char> ps = micron::to_string<u32>(prec);
      spec.append(ps.c_str(), ps.size());
      spec += 'g';
      spec += '}';
      require_true(same(micron::format::format(spec.c_str(), v), g_oracle(v, prec, true, false)));

      micron::hstring<schar> uspec("{:.");
      uspec.append(ps.c_str(), ps.size());
      uspec += 'G';
      uspec += '}';
      micron::hstring<schar> up = micron::format::format(uspec.c_str(), v);
      require_true(same(up, g_oracle(v, prec, false, true)));
      for ( usize k = 0; k < up.size(); ++k ) require_true(up[k] != 'e');
    }
  }
  end_test_case();

  print("=== FORMAT %g RIGOR SUITE PASSED ===");
  return 1;
}
