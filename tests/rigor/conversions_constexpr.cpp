// conversions_constexpr.cpp
// Compile-time proof that micron's number<->text layer constant-evaluates: the strict integer and
// float parsers, the Ryu shortest-form writers, and the fixed-buffer integer writers.
//
// Every assertion is a static_assert, so the file failing to compile IS the failing test. main()
// re-runs a handful of the same cases through a noinline volatile round-trip, because the whole
// point is that the folded answer and the executed answer are the same answer.
//
// What this became possible on: five union type-puns (bits.hpp d2s/d2f/d2e, floating_point.hpp
// f2s/to_general) read their inactive member to decompose a float. That is a hard error in a
// constant expression. They are now math::ieee::to_bits, which is __builtin_bit_cast -- already
// constexpr, and identical codegen at run time. Everything else in the parse stack was already
// clean by construction (pure integer math over inline constexpr tables, no reinterpret_cast, no
// SIMD, no inline asm) and needed only the keyword.
//
// What is deliberately NOT here, and cannot be:
//   * format::parse_double / parse_float -- they return option<F, parse_error>, and option is a
//     raw unsigned char storage[] plus three function pointers plus placement new plus a
//     non-constexpr destructor (sum.hpp:390-450). Never a literal type. The bool + out-param
//     siblings micron::try_parse_double / try_parse_float are the compile-time path.
//   * anything returning hstring -- int_to_string, double_to_string, to_fixed, format(). Those
//     reach the heap through an allocator base chain with no constexpr member in it.
//   * format::__impl::ptr_to_buf -- reinterpret_cast<u64>(ptr), illegal in a constant expression.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/string/format.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace ry = micron::__impl::__ryu;
namespace r32 = micron::__impl::__ryu::__f32;
namespace fi = micron::format::__impl;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// strict integer parsing

consteval bool
__u64(const char *s, usize n, u64 want)
{
  u64 v = 0;
  return mc::try_parse_uint64(s, n, v) && v == want;
}
static_assert(__u64("0", 1, 0));
static_assert(__u64("12345", 5, 12345u));
static_assert(__u64("18446744073709551615", 20, ~static_cast<u64>(0)));

consteval bool
__i64(const char *s, usize n, i64 want)
{
  i64 v = 0;
  return mc::try_parse_int64(s, n, v) && v == want;
}
static_assert(__i64("-987", 4, -987));
static_assert(__i64("+7", 2, 7));
static_assert(__i64("0", 1, 0));

consteval bool
__hex(const char *s, usize n, u64 want)
{
  u64 v = 0;
  return mc::try_parse_hex64(s, n, v) && v == want;
}
static_assert(__hex("beef", 4, 0xbeefu));
static_assert(__hex("DEADBEEF", 8, 0xDEADBEEFu));

// strict means fully consumed -- trailing garbage is a rejection, not a prefix parse
consteval bool
__u64_rejects(const char *s, usize n)
{
  u64 v = 0;
  return !mc::try_parse_uint64(s, n, v);
}
static_assert(__u64_rejects("12x", 3));
static_assert(__u64_rejects("", 0));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// strict float parsing, across all three tiers

consteval bool
__d(const char *s, usize n, f64 want)
{
  f64 v = 0.0;
  return mc::try_parse_double(s, n, v) && v == want;
}

// tier 1, the exact path: mantissa <= 2^53 and |exp10| <= 22, one rounded fpu op.
// NOTE this tier is if-constexpr'd out where __FLT_EVAL_METHOD__ != 0 (i386 defaults to x87 and
// rounds twice through an 80-bit significand), and tier 2 answers instead -- same value either way
static_assert(__d("1.5", 3, 1.5));
static_assert(__d("0", 1, 0.0));
static_assert(__d("-2.25", 5, -2.25));
static_assert(__d("1e3", 3, 1000.0));
static_assert(__d("1.5e3", 5, 1500.0));

// tier 2, eisel-lemire: a 17-digit significand is past what tier 1 can hold exactly
static_assert(__d("1.2345678901234567e10", 21, 12345678901.234567));
static_assert(__d("3.141592653589793", 17, 3.141592653589793));

// hex floats
static_assert(__d("0x1p0", 5, 1.0));
static_assert(__d("0x1.8p1", 7, 3.0));

// specials. the parser returns them as ok, so the writers' own output round-trips
consteval bool
__is_inf(const char *s, usize n, bool neg)
{
  f64 v = 0.0;
  if ( !mc::try_parse_double(s, n, v) ) return false;
  return mc::math::ieee::to_bits<f64>(v) == mc::math::ieee::to_bits<f64>(neg ? -__builtin_huge_val() : __builtin_huge_val());
}
static_assert(__is_inf("inf", 3, false));
static_assert(__is_inf("-inf", 4, true));

// the sign of a zero survives -- -0.0 and 0.0 are different bit patterns
consteval bool
__neg_zero(void)
{
  f64 v = 1.0;
  return mc::try_parse_double("-0.0", 4, v) && mc::math::ieee::to_bits<f64>(v) == (static_cast<u64>(1) << 63);
}
static_assert(__neg_zero());

// strict rejects what a prefix parse would accept
consteval bool
__d_rejects(const char *s, usize n)
{
  f64 v = 0.0;
  return !mc::try_parse_double(s, n, v);
}
static_assert(__d_rejects("1.5x", 4));
static_assert(__d_rejects("abc", 3));
static_assert(__d_rejects("", 0));

// f32
consteval bool
__f(const char *s, usize n, f32 want)
{
  f32 v = 0.0f;
  return mc::try_parse_float(s, n, v) && v == want;
}
static_assert(__f("2.5", 3, 2.5f));
static_assert(__f("-0.125", 6, -0.125f));
static_assert(__f("1e10", 4, 1e10f));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the Ryu writers -- this is what the five union removals bought

consteval bool
__d2s(f64 v, const char *want, usize wn)
{
  char b[32] = {};
  const usize n = ry::d2s_buffered(v, b);
  if ( n != wn ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( b[i] != want[i] ) return false;
  return true;
}
// d2s emits shortest-form with a mandatory fractional digit: 0.0 is "0.0", not "0"
static_assert(__d2s(1.5, "1.5", 3));
static_assert(__d2s(0.0, "0.0", 3));
static_assert(__d2s(1.0, "1.0", 3));
static_assert(__d2s(-2.25, "-2.25", 5));

consteval bool
__f2s_starts(f32 v, char c)
{
  char b[32] = {};
  const usize n = r32::f2s_buffered(v, b);
  return n > 0 && b[0] == c;
}
static_assert(__f2s_starts(2.5f, '2'));
static_assert(__f2s_starts(-1.0f, '-'));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the precision writers -- %f and %e, both tiers
//
// these fold for the first time as of the conversions/fixed.hpp rewrite. the tier-2 cases below
// are load-bearing: __dec_shift_left declares `u8 scratch[Cap + 64]` and only reads back what it
// wrote, which a constant evaluation is not allowed to do, so it carries an `if consteval`
// zero-init. nothing constant-evaluated the decimal bignum before -- parse_float's own folded
// cases all land in its tier 1 or 2 -- so without a tier-2 assertion here that guard is untested.

consteval bool
__d2f(f64 v, u32 prec, const char *want, usize wn)
{
  char b[1400] = {};
  const usize n = ry::d2f_buffered(v, b, 1400, prec);
  if ( n != wn ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( b[i] != want[i] ) return false;
  return true;
}

consteval bool
__d2e(f64 v, u32 prec, const char *want, usize wn)
{
  char b[1400] = {};
  const usize n = ry::d2e_buffered(v, b, 1400, prec);
  if ( n != wn ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( b[i] != want[i] ) return false;
  return true;
}

// tier 1: the u64 kernel. rounds, never truncates
static_assert(__d2f(0.6, 0, "1", 1));
static_assert(__d2f(3.14159265358979, 6, "3.141593", 8));
static_assert(__d2f(0.35, 1, "0.3", 3));
static_assert(__d2f(1.5, 0, "2", 1));
static_assert(__d2f(2.5, 0, "2", 1));
static_assert(__d2f(0.5, 0, "0", 1));
static_assert(__d2f(-273.15, 2, "-273.15", 7));
static_assert(__d2f(9.999, 2, "10.00", 5));

// tier 2: the decimal bignum. precision past 19 leaves the u64 kernel, so these force the
// __dec_shift path -- and with it the `if consteval` scratch guard
static_assert(__d2f(1.0 / 3.0, 20, "0.33333333333333331483", 22));
static_assert(__d2f(0.1, 20, "0.10000000000000000555", 22));

static_assert(__d2e(9.99, 1, "1.0e+01", 7));
static_assert(__d2e(0.0, 6, "0.000000e+00", 12));
static_assert(__d2e(1.7976931348623157e308, 3, "1.798e+308", 10));

// the sign comes from the sign bit, so a negative zero keeps it at compile time too
static_assert(__d2f(-0.0, 2, "-0.00", 5));

// the buffer contract: too small answers 0 rather than truncating
consteval bool
__d2f_rejects(f64 v, u32 prec, usize cap)
{
  char b[64] = {};
  return ry::d2f_buffered(v, b, cap, prec) == 0;
}
static_assert(__d2f_rejects(1.5, 2, 3));
static_assert(!__d2f_rejects(1.5, 2, 4));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// to_chars / from_chars
//
// the integer and bool halves fold; the float to_chars does not (d2s_buffered is constexpr but
// the porcelain overload set is not marked, and the hex/general paths reach __exact_round which
// is fine -- what cannot fold is nothing here, so both directions are asserted).

consteval bool
__tc(auto v, u32 base, const char *want, usize wn)
{
  char b[96] = {};
  const usize n = mc::to_chars(b, sizeof(b), v, base);
  if ( n != wn ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( b[i] != want[i] ) return false;
  return true;
}
static_assert(__tc(0, 10u, "0", 1));
static_assert(__tc(-1, 10u, "-1", 2));
static_assert(__tc(255u, 16u, "ff", 2));
static_assert(__tc(static_cast<i8>(-128), 10u, "-128", 4));
static_assert(__tc(static_cast<i64>(-0x7FFFFFFFFFFFFFFFll - 1), 10u, "-9223372036854775808", 20));
static_assert(__tc(-5, 2u, "-101", 4));      // the sign is emitted for every base

template<typename I>
consteval bool
__fc(const char *s, usize n, u32 base, I want)
{
  I v{};
  return mc::from_chars(v, s, n, base) && v == want;
}
static_assert(__fc<i32>("-42", 3, 10u, -42));
static_assert(__fc<u64>("18446744073709551615", 20, 10u, ~0ull));
static_assert(__fc<u8>("255", 3, 10u, 255));
static_assert(__fc<i8>("-128", 4, 10u, -128));
static_assert(__fc<u32>("ff", 2, 16u, 255u));

template<typename I>
consteval bool
__fc_rejects(const char *s, usize n, u32 base = 10u)
{
  I v{};
  return !mc::from_chars(v, s, n, base);
}
static_assert(__fc_rejects<u8>("256", 3));           // out of range for the width
static_assert(__fc_rejects<i8>("128", 3));
static_assert(__fc_rejects<u32>("-1", 2));           // unsigned rejects a sign
static_assert(__fc_rejects<i32>(" 1", 2));           // strict: no leading whitespace
static_assert(__fc_rejects<i32>("1 ", 2));           // strict: no trailing whitespace
static_assert(__fc_rejects<i32>("1x", 2));
static_assert(__fc_rejects<i32>("2", 1, 2u));        // digit outside the base

consteval bool
__tc_bool(bool v, const char *want, usize wn)
{
  char b[8] = {};
  const usize n = mc::to_chars(b, sizeof(b), v);
  if ( n != wn ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( b[i] != want[i] ) return false;
  return true;
}
static_assert(__tc_bool(true, "true", 4));
static_assert(__tc_bool(false, "false", 5));

consteval bool
__fc_bool(const char *s, usize n, bool want)
{
  bool v = false;
  return mc::from_chars(v, s, n) && v == want;
}
static_assert(__fc_bool("true", 4, true));
static_assert(__fc_bool("false", 5, false));
static_assert(__fc_bool("1", 1, true));
static_assert(__fc_bool("0", 1, false));

consteval bool
__b2h(bool upper, const char *want)
{
  const u8 src[2] = { 0xAB, 0x0F };
  char b[8] = {};
  const usize n = mc::bytes_to_hex(b, sizeof(b), src, 2, upper);
  if ( n != 4 ) return false;
  for ( usize i = 0; i < 4; ++i )
    if ( b[i] != want[i] ) return false;
  return true;
}
static_assert(__b2h(false, "ab0f"));
static_assert(__b2h(true, "AB0F"));

// the float from_chars folds too -- it is try_parse_double behind a strictness screen
consteval bool
__fc_f64(const char *s, usize n, f64 want)
{
  f64 v = 0.0;
  return mc::from_chars(v, s, n) && v == want;
}
static_assert(__fc_f64("1.5", 3, 1.5));
static_assert(__fc_f64("1e5", 3, 100000.0));
static_assert(__fc_f64("0x1p+0", 6, 1.0));

consteval bool
__fc_f64_rejects(const char *s, usize n)
{
  f64 v = 0.0;
  return !mc::from_chars(v, s, n);
}
static_assert(__fc_f64_rejects(" 1.5", 4));
static_assert(__fc_f64_rejects("1.5x", 4));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// FULL ROUND TRIP, folded end to end.
//
// d2s emits the shortest decimal that identifies the value uniquely, so parse(write(v)) must be
// bit-equal to v for every finite v. this is the same property tests/rigor/rigor_format_parse.cpp
// checks over 300k random values at run time -- here it is proven at compile time, which means
// the writer and the reader agree in BOTH evaluation modes

consteval bool
__roundtrip(f64 v)
{
  char b[32] = {};
  const usize n = ry::d2s_buffered(v, b);
  f64 back = 0.0;
  if ( !mc::try_parse_double(b, n, back) ) return false;
  return mc::math::ieee::to_bits<f64>(back) == mc::math::ieee::to_bits<f64>(v);
}
static_assert(__roundtrip(1.5));
static_assert(__roundtrip(0.0));
static_assert(__roundtrip(-0.0));      // the sign of a zero has to survive both directions
static_assert(__roundtrip(1.0));
static_assert(__roundtrip(-1.0));
static_assert(__roundtrip(3.141592653589793));
static_assert(__roundtrip(2.718281828459045));
static_assert(__roundtrip(1e300));
static_assert(__roundtrip(1e-300));
static_assert(__roundtrip(1.7976931348623157e308));         // DBL_MAX
static_assert(__roundtrip(2.2250738585072014e-308));        // DBL_MIN, smallest normal
static_assert(__roundtrip(-2.2250738585072014e-308));
static_assert(__roundtrip(123456789.123456789));
static_assert(__roundtrip(0.1));
static_assert(__roundtrip(1.0 / 3.0));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the fixed-buffer integer writers

consteval bool
__u2b(u64 v, u32 base, const char *want, usize wn)
{
  char b[72] = {};
  const usize n = fi::fmt_uint_to_buf(b, 72, v, base, false);
  if ( n != wn ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( b[i] != want[i] ) return false;
  return true;
}
static_assert(__u2b(12345, 10, "12345", 5));
static_assert(__u2b(0, 10, "0", 1));
static_assert(__u2b(0xbeef, 16, "beef", 4));
static_assert(__u2b(255, 2, "11111111", 8));

consteval bool
__i2b(i64 v, const char *want, usize wn)
{
  char b[72] = {};
  const usize n = fi::fmt_int_to_buf(b, 72, v, 10, false);
  if ( n != wn ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( b[i] != want[i] ) return false;
  return true;
}
static_assert(__i2b(-42, "-42", 3));
static_assert(__i2b(0, "0", 1));
static_assert(__i2b(-9223372036854775807LL - 1, "-9223372036854775808", 20));      // INT64_MIN

consteval bool
__b2b(void)
{
  char t[8] = {}, f[8] = {};
  return fi::bool_to_buf(t, 8, true) == 4 && t[0] == 't' && fi::bool_to_buf(f, 8, false) == 5 && f[0] == 'f';
}
static_assert(__b2b());

// the format-spec parser
consteval bool
__spec(void)
{
  const char s[] = ">8.3f";
  const fi::fmt_spec p = fi::parse_spec(s, s + 5);
  return p.align == '>' && p.width == 8 && p.prec == 3 && p.has_prec && p.type == 'f';
}
static_assert(__spec());

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the executed half. same entry points, same inputs, real code -- if the `if !consteval` split in
// micron::memcpy or the folded/emitted arms of the parser ever disagreed, only this would see it.
//
// -Ofast merges compile-time +-0.0 and folds inf/nan tests, so every special goes through a
// volatile u64 first (ISSUES.md, the d2s(+-0.0) CSE on linaro aarch64)

[[gnu::noinline]] static f64
f64_opaque(u64 b)
{
  volatile u64 vb = b;
  const u64 t = vb;
  return mc::math::ieee::from_bits<f64>(t);
}

[[gnu::noinline]] static bool
rt_roundtrip(f64 v)
{
  char b[32] = {};
  const usize n = ry::d2s_buffered(v, b);
  f64 back = 0.0;
  if ( !mc::try_parse_double(b, n, back) ) return false;
  return mc::math::ieee::to_bits<f64>(back) == mc::math::ieee::to_bits<f64>(v);
}

int
main()
{
  sb::print("=== CONVERSIONS CONSTEXPR PROOFS ===");

  // the same values the static_asserts above proved, now executed
  sb::require(rt_roundtrip(1.5));
  sb::require(rt_roundtrip(3.141592653589793));
  sb::require(rt_roundtrip(1e300));
  sb::require(rt_roundtrip(0.1));
  sb::require(rt_roundtrip(f64_opaque(0x0000000000000000ull)));        // +0.0
  sb::require(rt_roundtrip(f64_opaque(0x8000000000000000ull)));        // -0.0
  sb::require(rt_roundtrip(f64_opaque(0x7FEFFFFFFFFFFFFFull)));        // DBL_MAX
  sb::require(rt_roundtrip(f64_opaque(0x0010000000000000ull)));        // smallest normal
  sb::require(rt_roundtrip(f64_opaque(0x0000000000000001ull)));        // smallest subnormal

  {
    u64 v = 0;
    sb::require(mc::try_parse_uint64("12345", 5, v) && v == 12345u);
    f64 d = 0.0;
    sb::require(mc::try_parse_double("1.5", 3, d) && d == 1.5);
    sb::require(!mc::try_parse_double("1.5x", 4, d));
    char b[72] = {};
    sb::require(fi::fmt_int_to_buf(b, 72, -42, 10, false) == 3 && b[0] == '-');
  }

  sb::print("=== ALL CONVERSIONS CONSTEXPR TESTS PASSED ===");
  return 1;
}
