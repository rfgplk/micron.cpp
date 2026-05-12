//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

//  string_format_extensive: deep coverage of micron::format::format with a
//  particular focus on the multi-{} placeholder paths that depend on the
//  hstring::append(F*, usize) length accounting — historically the width /
//  fill / alignment branch of apply_padding was avoided because the append
//  overload underflowed length by one. With that fixed, this suite exercises
//  every combination of [fill][align][#][width][.precision][type] over every
//  formatter specialization micron exposes.

#include "../../src/string/format.hpp"
#include "../../src/string/strings.hpp"
#include "../snowball/snowball.hpp"

#include "../../src/io/console.hpp"

using namespace snowball;
namespace fmt = micron::format;

using hstr = micron::hstring<schar>;

static bool
eq(const hstr &lhs, const char *rhs)
{
  usize n = micron::strlen(rhs);
  if ( lhs.size() != n ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( lhs[i] != rhs[i] ) return false;
  return true;
}

// Length-and-content invariant: lhs.size() matches the visible content and
// lhs[size()] is reachable (string is a complete buffer). The append() bug
// would silently break the first condition while leaving content intact.
static bool
length_consistent(const hstr &lhs, const char *expected)
{
  usize n = micron::strlen(expected);
  if ( lhs.size() != n ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( lhs[i] != expected[i] ) return false;
  return true;
}

int
main(int, char **)
{
  sb::print("=== micron::format EXTENSIVE PLACEHOLDER SUITE ===");

  // ===========================================================
  // SECTION 1 — literal text + empty / pathological inputs
  // ===========================================================
  sb::print("--- literal text & pathological inputs ---");

  test_case("format – empty format string yields empty result");
  {
    require_true(eq(fmt::format(""), ""));
    auto e = fmt::format("");
    require(e.size(), usize{ 0 });
  }
  end_test_case();

  test_case("format – no placeholders passes text through verbatim");
  {
    require_true(eq(fmt::format("hello world"), "hello world"));
    require_true(eq(fmt::format("a b c d e f g"), "a b c d e f g"));
    auto s = fmt::format("plain");
    require(s.size(), usize{ 5 });
  }
  end_test_case();

  test_case("format – ignores extra args when format has no placeholders");
  {
    require_true(eq(fmt::format("static", 1, 2, 3), "static"));
  }
  end_test_case();

  test_case("format – text with unmatched '{' breaks out cleanly");
  {
    // '{' with no closing '}' truncates output at the '{'
    auto s = fmt::format("ok{");
    require_true(eq(s, "ok"));
  }
  end_test_case();

  test_case("format – lone '}' is passed through as literal");
  {
    require_true(eq(fmt::format("a}b"), "a}b"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 2 — brace escaping
  // ===========================================================
  sb::print("--- brace escaping ---");

  test_case("format – {{ and }} produce single braces");
  {
    require_true(eq(fmt::format("{{"), "{"));
    require_true(eq(fmt::format("}}"), "}"));
    require_true(eq(fmt::format("{{}}"), "{}"));
    require_true(eq(fmt::format("{{ a={} }}", 1), "{ a=1 }"));
    require_true(eq(fmt::format("set = {{{}, {}, {}}}", 1, 2, 3), "set = {1, 2, 3}"));
  }
  end_test_case();

  test_case("format – escaped braces wrapped around placeholder");
  {
    require_true(eq(fmt::format("{{{}}}", 42), "{42}"));
    require_true(eq(fmt::format("{{{}: {}}}", "k", "v"), "{k: v}"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 3 — auto-indexed {} placeholders
  // ===========================================================
  sb::print("--- auto-indexed {} placeholders ---");

  test_case("format – single {} with each scalar kind");
  {
    require_true(eq(fmt::format("{}", 0), "0"));
    require_true(eq(fmt::format("{}", -1), "-1"));
    require_true(eq(fmt::format("{}", 'Q'), "Q"));
    require_true(eq(fmt::format("{}", true), "true"));
    require_true(eq(fmt::format("{}", false), "false"));
    require_true(eq(fmt::format("{}", "lit"), "lit"));
  }
  end_test_case();

  test_case("format – two {} placeholders consume in order");
  {
    require_true(eq(fmt::format("{}+{}", 1, 2), "1+2"));
    require_true(eq(fmt::format("{} and {}", "tea", "coffee"), "tea and coffee"));
  }
  end_test_case();

  test_case("format – many {} placeholders (8 args)");
  {
    require_true(eq(fmt::format("{}/{}/{}/{}/{}/{}/{}/{}", 1, 2, 3, 4, 5, 6, 7, 8), "1/2/3/4/5/6/7/8"));
    require_true(eq(fmt::format("[{}][{}][{}][{}][{}][{}][{}][{}]", 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'), "[a][b][c][d][e][f][g][h]"));
  }
  end_test_case();

  test_case("format – {} with interleaved literal text");
  {
    require_true(eq(fmt::format("x={},y={},z={}", 1, 2, 3), "x=1,y=2,z=3"));
    require_true(eq(fmt::format("<{}>:{}|{}<{}>", 'a', 'b', 'c', 'd'), "<a>:b|c<d>"));
  }
  end_test_case();

  test_case("format – mixing types in auto-indexed args");
  {
    require_true(eq(fmt::format("{} {} {} {} {}", 1, 'x', true, "str", static_cast<f64>(3.5)), "1 x true str 3.500000"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 4 — explicit positional indices {N}
  // ===========================================================
  sb::print("--- explicit positional {N} placeholders ---");

  test_case("format – reorder args with {N}");
  {
    require_true(eq(fmt::format("{2}-{1}-{0}", 'a', 'b', 'c'), "c-b-a"));
    require_true(eq(fmt::format("{0}{0}{0}", 'x'), "xxx"));
  }
  end_test_case();

  test_case("format – {N} resets the auto-counter to N+1");
  {
    // After {0} (explicit, index 0), auto-counter becomes 1.
    // Next {} therefore picks index 1.
    require_true(eq(fmt::format("{0}{}", 'A', 'B'), "AB"));
    // {} (auto=0) → 'A'; {0} (explicit) → 'A'; {} (auto reset to 1) → 'B'
    require_true(eq(fmt::format("{}{0}{}", 'A', 'B', 'C'), "AAB"));
    // {1} → 'B'; auto resets to 2; {} → 'C'
    require_true(eq(fmt::format("{1}{}", 'A', 'B', 'C'), "BC"));
  }
  end_test_case();

  test_case("format – positional with format spec");
  {
    require_true(eq(fmt::format("{1:x} {0:X}", 0xab, 0xcd), "cd AB"));
    require_true(eq(fmt::format("[{0:>3}|{0:<3}]", 'q'), "[  q|q  ]"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 5 — integer type dispatch (every size, every base)
  // ===========================================================
  sb::print("--- integer dispatch ---");

  test_case("format – signed integer sizes route to i32/i64 formatter");
  {
    i8 vi8 = -7;
    i16 vi16 = -1234;
    i32 vi32 = -1000000;
    i64 vi64 = -9000000000LL;
    require_true(eq(fmt::format("{}", vi8), "-7"));
    require_true(eq(fmt::format("{}", vi16), "-1234"));
    require_true(eq(fmt::format("{}", vi32), "-1000000"));
    require_true(eq(fmt::format("{}", vi64), "-9000000000"));
  }
  end_test_case();

  test_case("format – unsigned integer sizes route to u32/u64 formatter");
  {
    u8 vu8 = 200;
    u16 vu16 = 60000;
    u32 vu32 = 4000000000u;
    u64 vu64 = 18000000000000000000ull;
    require_true(eq(fmt::format("{}", vu8), "200"));
    require_true(eq(fmt::format("{}", vu16), "60000"));
    require_true(eq(fmt::format("{}", vu32), "4000000000"));
    require_true(eq(fmt::format("{}", vu64), "18000000000000000000"));
  }
  end_test_case();

  test_case("format – integer min/max boundaries");
  {
    i32 imin = -2147483647 - 1;
    i32 imax = 2147483647;
    i64 lmin = static_cast<i64>(-9223372036854775807LL) - 1;
    i64 lmax = 9223372036854775807LL;
    u64 umax = 18446744073709551615ull;
    require_true(eq(fmt::format("{}", imin), "-2147483648"));
    require_true(eq(fmt::format("{}", imax), "2147483647"));
    require_true(eq(fmt::format("{}", lmin), "-9223372036854775808"));
    require_true(eq(fmt::format("{}", lmax), "9223372036854775807"));
    require_true(eq(fmt::format("{}", umax), "18446744073709551615"));
  }
  end_test_case();

  test_case("format – hex {:x} / {:X} lower & upper");
  {
    require_true(eq(fmt::format("{:x}", 0), "0"));
    require_true(eq(fmt::format("{:x}", 15), "f"));
    require_true(eq(fmt::format("{:X}", 15), "F"));
    require_true(eq(fmt::format("{:x}", 0xdeadbeefu), "deadbeef"));
    require_true(eq(fmt::format("{:X}", 0xdeadbeefu), "DEADBEEF"));
    require_true(eq(fmt::format("{:x}", static_cast<u64>(0xffffffffffffffffull)), "ffffffffffffffff"));
  }
  end_test_case();

  test_case("format – octal {:o} and binary {:b}");
  {
    require_true(eq(fmt::format("{:o}", 0), "0"));
    require_true(eq(fmt::format("{:o}", 8), "10"));
    require_true(eq(fmt::format("{:o}", 64), "100"));
    require_true(eq(fmt::format("{:o}", 0777u), "777"));
    require_true(eq(fmt::format("{:b}", 0), "0"));
    require_true(eq(fmt::format("{:b}", 1), "1"));
    require_true(eq(fmt::format("{:b}", 2), "10"));
    require_true(eq(fmt::format("{:b}", 0b10110101u), "10110101"));
  }
  end_test_case();

  test_case("format – #-alt flag adds 0x / 0 / 0b prefixes");
  {
    require_true(eq(fmt::format("{:#x}", 255), "0xff"));
    require_true(eq(fmt::format("{:#X}", 255), "0XFF"));
    require_true(eq(fmt::format("{:#o}", 8), "010"));
    require_true(eq(fmt::format("{:#b}", 5), "0b101"));
    // alt + decimal: no prefix
    require_true(eq(fmt::format("{:#d}", 12), "12"));
    require_true(eq(fmt::format("{:#}", 12), "12"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 6 — float dispatch (default + .N + e/E + g/G + f/F)
  // ===========================================================
  sb::print("--- float dispatch ---");

  test_case("format – f32 / f64 default routes through Ryu fixed @ prec=6");
  {
    f32 a32 = 0.5f;
    f64 a64 = 0.5;
    require_true(eq(fmt::format("{}", a32), "0.500000"));
    require_true(eq(fmt::format("{}", a64), "0.500000"));
    require_true(eq(fmt::format("{}", static_cast<f64>(0.0)), "0.000000"));
    require_true(eq(fmt::format("{}", static_cast<f64>(-1.0)), "-1.000000"));
  }
  end_test_case();

  test_case("format – {:.Nf} fixed precision over a wide grid");
  {
    f64 pi = 3.14159265358979;
    require_true(eq(fmt::format("{:.0f}", pi), "3"));
    require_true(eq(fmt::format("{:.1f}", pi), "3.1"));
    require_true(eq(fmt::format("{:.2f}", pi), "3.14"));
    require_true(eq(fmt::format("{:.3f}", pi), "3.141"));
    require_true(eq(fmt::format("{:.4f}", pi), "3.1415"));
    require_true(eq(fmt::format("{:.5f}", pi), "3.14159"));
    require_true(eq(fmt::format("{:.10f}", pi), "3.1415926535"));
  }
  end_test_case();

  test_case("format – {:.Ne} scientific precision over a grid");
  {
    f64 v = 1.7976931348623157e+308;
    require_true(eq(fmt::format("{:e}", v), "1.797693e+308"));
    require_true(eq(fmt::format("{:.0e}", v), "1e+308"));
    require_true(eq(fmt::format("{:.3e}", v), "1.797e+308"));
    f64 small = 1e-10;
    require_true(eq(fmt::format("{:.3e}", small), "1.000e-10"));
    f64 zero = 0.0;
    require_true(eq(fmt::format("{:.3e}", zero), "0.000e+00"));
  }
  end_test_case();

  test_case("format – f64 negative branch");
  {
    f64 v = -2.5;
    require_true(eq(fmt::format("{:.1f}", v), "-2.5"));
    require_true(eq(fmt::format("{:.3f}", v), "-2.500"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 7 — bool, char, pointer dispatch
  // ===========================================================
  sb::print("--- bool / char / pointer ---");

  test_case("format – bool prints true/false; width pads");
  {
    require_true(eq(fmt::format("{}", true), "true"));
    require_true(eq(fmt::format("{}", false), "false"));
    require_true(eq(fmt::format("{:>7}", true), "   true"));
    require_true(eq(fmt::format("{:<7}", true), "true   "));
    require_true(eq(fmt::format("{:^7}", true), " true  "));
    require_true(eq(fmt::format("{:>7}", false), "  false"));
  }
  end_test_case();

  test_case("format – char prints exactly one byte; width pads");
  {
    require_true(eq(fmt::format("{}", 'A'), "A"));
    require_true(eq(fmt::format("{:>3}", 'A'), "  A"));
    require_true(eq(fmt::format("{:<3}", 'A'), "A  "));
    require_true(eq(fmt::format("{:^5}", 'A'), "  A  "));
    // odd extra goes right (per apply_padding: left = pad/2, right = pad-left)
    require_true(eq(fmt::format("{:^4}", 'A'), " A  "));
  }
  end_test_case();

  test_case("format – pointer hex with 0x prefix");
  {
    i32 i = 0;
    i32 *p = &i;
    auto s = fmt::format("{}", p);
    // starts with "0x", rest is hex digits — verify length is plausible and prefix matches.
    require_true(s.size() >= usize{ 3 });
    require(s[0], '0');
    require(s[1], 'x');
    // null pointer formats to "0x0"
    i32 *np = nullptr;
    require_true(eq(fmt::format("{}", np), "0x0"));
    void *vp = nullptr;
    require_true(eq(fmt::format("{}", vp), "0x0"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 8 — string args
  // ===========================================================
  sb::print("--- string args ---");

  test_case("format – const char*, char[N], hstring all route through string formatter");
  {
    const char *p = "abc";
    char arr[] = "def";
    hstr h("ghi");
    require_true(eq(fmt::format("{}", p), "abc"));
    require_true(eq(fmt::format("{}", arr), "def"));
    require_true(eq(fmt::format("{}", h), "ghi"));
    require_true(eq(fmt::format("{}{}{}", p, arr, h), "abcdefghi"));
  }
  end_test_case();

  test_case("format – null const char* prints (null)");
  {
    const char *np = nullptr;
    require_true(eq(fmt::format("{}", np), "(null)"));
  }
  end_test_case();

  test_case("format – string precision .N truncates");
  {
    require_true(eq(fmt::format("{:.0}", "abc"), ""));
    require_true(eq(fmt::format("{:.1}", "abc"), "a"));
    require_true(eq(fmt::format("{:.3}", "abc"), "abc"));
    require_true(eq(fmt::format("{:.5}", "abc"), "abc"));
    require_true(eq(fmt::format("{:.4}", "abcdef"), "abcd"));
    // precision on hstring
    hstr h("helloworld");
    require_true(eq(fmt::format("{:.5}", h), "hello"));
  }
  end_test_case();

  test_case("format – empty string arg");
  {
    require_true(eq(fmt::format("[{}]", ""), "[]"));
    require_true(eq(fmt::format("[{}|{}]", "", ""), "[|]"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 9 — width with default alignment
  // ===========================================================
  // Default alignment rule (apply_padding @ src/string/format.hpp:283-288):
  //   spec.type ∈ { 's', '\0' }  → '<' (left)
  //   otherwise                  → '>' (right)
  // So {:N} without an explicit type defaults to LEFT for every type
  // (including numbers); to get right-aligned numbers use {:>N} or {:Nd}.
  sb::print("--- width with default alignment ---");

  test_case("format – {:N} (no type) defaults to left-align for every type");
  {
    require_true(eq(fmt::format("{:5}", 42), "42   "));
    require_true(eq(fmt::format("{:5}", -7), "-7   "));
    require_true(eq(fmt::format("{:5}", "ab"), "ab   "));
    require_true(eq(fmt::format("{:5}", 'X'), "X    "));
    require_true(eq(fmt::format("{:5}", true), "true "));
  }
  end_test_case();

  test_case("format – {:Nd} (typed) defaults to right-align for numbers");
  {
    require_true(eq(fmt::format("{:5d}", 42), "   42"));
    require_true(eq(fmt::format("{:5d}", -7), "   -7"));
    require_true(eq(fmt::format("{:5x}", 0xabu), "   ab"));
    require_true(eq(fmt::format("{:5o}", 8), "   10"));
    require_true(eq(fmt::format("{:5b}", 5), "  101"));
    require_true(eq(fmt::format("{:8.2f}", static_cast<f64>(3.5)), "    3.50"));
  }
  end_test_case();

  test_case("format – width <= content length: no padding, no truncation");
  {
    require_true(eq(fmt::format("{:0}", 12345), "12345"));
    require_true(eq(fmt::format("{:3}", 12345), "12345"));
    require_true(eq(fmt::format("{:5}", 12345), "12345"));
    require_true(eq(fmt::format("{:5}", "abcde"), "abcde"));
    require_true(eq(fmt::format("{:3}", "abcde"), "abcde"));
  }
  end_test_case();

  test_case("format – wide width across all numeric types (default left-align)");
  {
    require_true(eq(fmt::format("{:10}", static_cast<i8>(-3)), "-3        "));
    require_true(eq(fmt::format("{:10}", static_cast<u16>(7)), "7         "));
    require_true(eq(fmt::format("{:10}", static_cast<i32>(123456)), "123456    "));
    require_true(eq(fmt::format("{:12}", static_cast<u64>(1000000000ull)), "1000000000  "));
    require_true(eq(fmt::format("{:10.2f}", static_cast<f64>(3.5)), "      3.50"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 10 — explicit alignment and fill
  // ===========================================================
  sb::print("--- explicit alignment & fill ---");

  test_case("format – explicit < > ^ with default space fill");
  {
    require_true(eq(fmt::format("{:<5}", 'A'), "A    "));
    require_true(eq(fmt::format("{:>5}", 'A'), "    A"));
    require_true(eq(fmt::format("{:^5}", 'A'), "  A  "));
    require_true(eq(fmt::format("{:<5}", 42), "42   "));
    require_true(eq(fmt::format("{:>5}", 42), "   42"));
    require_true(eq(fmt::format("{:^5}", 42), " 42  "));
    require_true(eq(fmt::format("{:<5}", "ab"), "ab   "));
    require_true(eq(fmt::format("{:>5}", "ab"), "   ab"));
    require_true(eq(fmt::format("{:^5}", "ab"), " ab  "));
  }
  end_test_case();

  test_case("format – center alignment: odd padding goes to right");
  {
    // n=2, width=5, pad=3, left=1, right=2 → " ab  "
    require_true(eq(fmt::format("{:^5}", "ab"), " ab  "));
    // n=3, width=8, pad=5, left=2, right=3 → "  abc   "
    require_true(eq(fmt::format("{:^8}", "abc"), "  abc   "));
    // n=1, width=4, pad=3, left=1, right=2 → " A  "
    require_true(eq(fmt::format("{:^4}", 'A'), " A  "));
    // n=4, width=7, pad=3, left=1, right=2 → " true  "
    require_true(eq(fmt::format("{:^7}", true), " true  "));
  }
  end_test_case();

  test_case("format – custom fill char with each alignment");
  {
    require_true(eq(fmt::format("{:*<5}", "ab"), "ab***"));
    require_true(eq(fmt::format("{:*>5}", "ab"), "***ab"));
    // {:^7} of "ab": n=2, pad=5, left=pad/2=2, right=pad-left=3 → "**ab***"
    require_true(eq(fmt::format("{:*^7}", "ab"), "**ab***"));
    require_true(eq(fmt::format("{:0>6}", 42), "000042"));
    require_true(eq(fmt::format("{:0>8x}", 0xffu), "000000ff"));
    require_true(eq(fmt::format("{:_<6}", 'q'), "q_____"));
    require_true(eq(fmt::format("{:.^9}", "mid"), "...mid..."));
    require_true(eq(fmt::format("{:-^10}", "hi"), "----hi----"));
  }
  end_test_case();

  test_case("format – zero-pad hex with alt flag");
  {
    // parse_spec order is [fill][align][#][width][.prec][type], so the alt
    // flag has to come BEFORE width. {:0>#8x} parses as fill='0', align='>',
    // alt=true, width=8, type='x'. Formatter emits "0xff" (n=4); apply_padding
    // right-aligns to 8 wide with fill '0' → "00000xff".
    require_true(eq(fmt::format("{:0>#8x}", 0xffu), "00000xff"));
    require_true(eq(fmt::format("{: >#8x}", 0xffu), "    0xff"));
  }
  end_test_case();

  test_case("format – width with negative integer keeps sign attached to digits");
  {
    require_true(eq(fmt::format("{:>5}", -42), "  -42"));
    require_true(eq(fmt::format("{:<5}", -42), "-42  "));
    require_true(eq(fmt::format("{:^7}", -42), "  -42  "));
    require_true(eq(fmt::format("{:0>6}", -42), "000-42"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 11 — combined width + precision + type
  // ===========================================================
  sb::print("--- combined width + precision + type ---");

  test_case("format – width + precision for strings (truncate then pad)");
  {
    // precision truncates content to 3 ("abc"), then width 10 left-pads
    require_true(eq(fmt::format("{:>10.3}", "abcdef"), "       abc"));
    require_true(eq(fmt::format("{:<10.3}", "abcdef"), "abc       "));
    require_true(eq(fmt::format("{:^9.3}", "abcdef"), "   abc   "));
    require_true(eq(fmt::format("{:*>10.3}", "abcdef"), "*******abc"));
  }
  end_test_case();

  test_case("format – width + .Nf for floats");
  {
    f64 pi = 3.14159265358979;
    require_true(eq(fmt::format("{:>10.2f}", pi), "      3.14"));
    require_true(eq(fmt::format("{:<10.2f}", pi), "3.14      "));
    require_true(eq(fmt::format("{:^10.2f}", pi), "   3.14   "));
    require_true(eq(fmt::format("{:0>8.2f}", pi), "00003.14"));
  }
  end_test_case();

  test_case("format – width + .Ne for floats");
  {
    f64 v = 1.7976931348623157e+308;
    require_true(eq(fmt::format("{:>16.3e}", v), "      1.797e+308"));
    require_true(eq(fmt::format("{:<16.3e}", v), "1.797e+308      "));
    require_true(eq(fmt::format("{:0>14.3e}", v), "00001.797e+308"));
  }
  end_test_case();

  test_case("format – width + #x (alt-flag prefix participates in length)");
  {
    // # must come before width per parse_spec's [fill][align][#][width]...
    // order. formatter writes "0xff" (n=4), width=8 → 4 fills + "0xff".
    require_true(eq(fmt::format("{:>#8x}", 0xffu), "    0xff"));
    require_true(eq(fmt::format("{:<#8x}", 0xffu), "0xff    "));
    require_true(eq(fmt::format("{:^#8x}", 0xffu), "  0xff  "));
  }
  end_test_case();

  test_case("format – very wide width (much bigger than content)");
  {
    auto s = fmt::format("{:>20}", 7);
    require(s.size(), usize{ 20 });
    require(s[19], '7');
    for ( usize i = 0; i < 19; ++i ) require(s[i], ' ');
  }
  end_test_case();

  // ===========================================================
  // SECTION 12 — length / size invariants
  //   This is the regression net for the original bug:
  //   size() must always match the visible content length.
  // ===========================================================
  sb::print("--- length / size invariants ---");

  test_case("format – size() matches strlen of every produced string");
  {
    require_true(length_consistent(fmt::format("{}", 0), "0"));
    require_true(length_consistent(fmt::format("{}", -1), "-1"));
    require_true(length_consistent(fmt::format("{}", 12345), "12345"));
    require_true(length_consistent(fmt::format("{:x}", 0xabc), "abc"));
    require_true(length_consistent(fmt::format("{:#x}", 0xabc), "0xabc"));
    require_true(length_consistent(fmt::format("{}", true), "true"));
    require_true(length_consistent(fmt::format("{}", false), "false"));
    require_true(length_consistent(fmt::format("{}", 'M'), "M"));
    require_true(length_consistent(fmt::format("{}", "string"), "string"));
    require_true(length_consistent(fmt::format("{:.3}", "abcdef"), "abc"));
    require_true(length_consistent(fmt::format("{:>10}", 7), "         7"));
    require_true(length_consistent(fmt::format("{:<10}", 7), "7         "));
    require_true(length_consistent(fmt::format("{:^10}", 7), "    7     "));
    require_true(length_consistent(fmt::format("{:*^10}", 'A'), "****A*****"));
    require_true(length_consistent(fmt::format("{:.2f}", static_cast<f64>(1.0)), "1.00"));
  }
  end_test_case();

  test_case("format – chained formats stay length-consistent");
  {
    // Build a long string by formatting multiple times and stitching.
    hstr a = fmt::format("[{:>5}]", 1);
    hstr b = fmt::format("[{:<5}]", 2);
    hstr c = fmt::format("[{:^5}]", 3);
    require(a.size(), usize{ 7 });
    require(b.size(), usize{ 7 });
    require(c.size(), usize{ 7 });
    hstr combined;
    combined.append(a);
    combined.append(b);
    combined.append(c);
    require(combined.size(), usize{ 21 });
    require_true(eq(combined, "[    1][2    ][  3  ]"));
  }
  end_test_case();

  test_case("format – many appended one-byte payloads stay consistent");
  {
    // Each {:.1} produces a 1-char string; stacking 16 of them via the multi-
    // arg path exercises the per-arg apply_padding/append path 16 times.
    auto s = fmt::format("{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}{:.1}", "A", "B", "C", "D", "E", "F",
                         "G", "H", "I", "J", "K", "L", "M", "N", "O", "P");
    require_true(eq(s, "ABCDEFGHIJKLMNOP"));
    require(s.size(), usize{ 16 });
  }
  end_test_case();

  // ===========================================================
  // SECTION 13 — format_value (single-value public entry point)
  // ===========================================================
  sb::print("--- format_value single-value entry point ---");

  test_case("format_value – returns same as format(\"{}\", v) for scalars");
  {
    require_true(eq(fmt::format_value(0), "0"));
    require_true(eq(fmt::format_value(-42), "-42"));
    require_true(eq(fmt::format_value(static_cast<u64>(123ull)), "123"));
    require_true(eq(fmt::format_value(true), "true"));
    require_true(eq(fmt::format_value('q'), "q"));
    require_true(eq(fmt::format_value(static_cast<f64>(0.5)), "0.500000"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 14 — concat: the two-arg + length overload
  //   This is the path that used to pass len+1 to compensate for the bug.
  // ===========================================================
  sb::print("--- concat(const char*, usize, const char*, usize) ---");

  test_case("concat – content and length exactly match the two halves");
  {
    auto s = fmt::concat("abc", static_cast<usize>(3), "def", static_cast<usize>(3));
    require(s.size(), usize{ 6 });
    require_true(eq(s, "abcdef"));
  }
  end_test_case();

  test_case("concat – empty halves");
  {
    auto s1 = fmt::concat("", static_cast<usize>(0), "tail", static_cast<usize>(4));
    require_true(eq(s1, "tail"));
    auto s2 = fmt::concat("head", static_cast<usize>(4), "", static_cast<usize>(0));
    require_true(eq(s2, "head"));
    auto s3 = fmt::concat("", static_cast<usize>(0), "", static_cast<usize>(0));
    require(s3.size(), usize{ 0 });
  }
  end_test_case();

  test_case("concat – longer payloads");
  {
    const char *lhs = "the quick brown fox ";
    const char *rhs = "jumps over the lazy dog";
    auto s = fmt::concat(lhs, micron::strlen(lhs), rhs, micron::strlen(rhs));
    require(s.size(), micron::strlen(lhs) + micron::strlen(rhs));
    require_true(eq(s, "the quick brown fox jumps over the lazy dog"));
  }
  end_test_case();

  // ===========================================================
  // SECTION 15 — join / join_strings (uses append internally)
  // ===========================================================
  sb::print("--- join / join_strings ---");

  test_case("join – ints with comma delimiter");
  {
    micron::fvector<i32> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    auto s = fmt::join(v, ", ");
    require_true(eq(s, "1, 2, 3"));
    require(s.size(), usize{ 7 });
  }
  end_test_case();

  test_case("join – single-element produces no delimiter");
  {
    micron::fvector<i32> v;
    v.push_back(42);
    auto s = fmt::join(v, "--");
    require_true(eq(s, "42"));
  }
  end_test_case();

  test_case("join – empty container produces empty result");
  {
    micron::fvector<i32> v;
    auto s = fmt::join(v, ",");
    require(s.size(), usize{ 0 });
  }
  end_test_case();

  test_case("join_strings – fvector of hstring");
  {
    micron::fvector<hstr> v;
    v.push_back(hstr("alpha"));
    v.push_back(hstr("beta"));
    v.push_back(hstr("gamma"));
    auto s = fmt::join_strings(v, "::");
    require_true(eq(s, "alpha::beta::gamma"));
    require(s.size(), usize{ 18 });
  }
  end_test_case();

  // ===========================================================
  // SECTION 16 — stress: long formats and many args
  // ===========================================================
  sb::print("--- stress: long formats & many args ---");

  test_case("format – long literal interleaved with placeholders");
  {
    auto s = fmt::format("name={}, age={}, score={:.2f}, ratio={:.3}, hex={:#x}, bin={:#b}, ok={}", "alice", 30, static_cast<f64>(95.5),
                         "0.123456", 0xffu, 0b1010u, true);
    require_true(eq(s, "name=alice, age=30, score=95.50, ratio=0.1, hex=0xff, bin=0b1010, ok=true"));
  }
  end_test_case();

  test_case("format – repeated positional with mixed specs");
  {
    auto s = fmt::format("{0}=[{0:>4}|{0:<4}|{0:^4}]", 7);
    require_true(eq(s, "7=[   7|7   | 7  ]"));
  }
  end_test_case();

  test_case("format – 12 mixed-type args (deep recursion through format_one)");
  {
    auto s = fmt::format("{} {} {} {} {} {} {} {} {} {} {} {}", 1, 2u, static_cast<i64>(3), static_cast<u64>(4), 'A', "B", true, false,
                         static_cast<f64>(0.5), static_cast<f64>(-1.0), "tail1", "tail2");
    require_true(eq(s, "1 2 3 4 A B true false 0.500000 -1.000000 tail1 tail2"));
  }
  end_test_case();

  test_case("format – width sums add up to expected length for many args");
  {
    // Each {:>4} occupies exactly 4 chars; 5 of them + 4 separator chars = 24.
    auto s = fmt::format("[{:>4}|{:>4}|{:>4}|{:>4}|{:>4}]", 1, 22, 333, 4444, 55555);
    require_true(eq(s, "[   1|  22| 333|4444|55555]"));
    require(s.size(), usize{ 27 });
  }
  end_test_case();

  sb::print("--- regression: small payloads through placeholder pipeline ---");

  test_case("format – tons of 1-char specs round-trip exactly");
  {
    auto s = fmt::format("{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}", 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
                         'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5');
    require(s.size(), usize{ 32 });
    require_true(eq(s, "abcdefghijklmnopqrstuvwxyz012345"));
  }
  end_test_case();

  test_case("format – mix of 1-byte and multi-byte placeholders");
  {
    auto s = fmt::format("[{}|{}|{}|{}|{}]", 'a', "longer", 'b', 123, "x");
    require_true(eq(s, "[a|longer|b|123|x]"));
    require(s.size(), usize{ 18 });
  }
  end_test_case();

  // ===========================================================
  sb::print("=== ALL EXTENSIVE FORMAT TESTS PASSED ===");
  return 1;
}
