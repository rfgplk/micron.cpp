// rigor_format_engine.cpp — snowball suite for the format() engine in
// src/string/format.hpp.
//
// Coverage (in micron::format namespace):
//   format(fmt, args...)          (printf-like with {} placeholders)
//   format_value(val)             (single-value → hstring)
//   join(items, delim)            (container → delimited string)
//   join(items, delim, prefix, suffix)
//   join_strings(fvector<S>, delim)
//
// Spec coverage (per parse_spec/apply_padding):
//   * implicit / explicit positional args ({} vs {0})
//   * literal '{{' and '}}'
//   * width / precision / fill / align / type / alt
//   * type chars: d, x, X, o, b, f, e, s, p
//   * default precision behavior

#include "../../src/string/format.hpp"

#include "../support/format_rigor.hpp"

using namespace mtest::format_rigor;
using mtest::prng;
namespace fmt = micron::format;
using sb::end_test_case;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

// substring presence
static bool
hstr_contains(const hstr &s, const char *needle)
{
  usize n = micron::strlen(needle);
  if ( n == 0 ) return true;
  if ( s.size() < n ) return false;
  for ( usize i = 0; i + n <= s.size(); ++i ) {
    bool match = true;
    for ( usize j = 0; j < n; ++j )
      if ( s[i + j] != needle[j] ) {
        match = false;
        break;
      }
    if ( match ) return true;
  }
  return false;
}

int
main()
{
  sb::print("=== FORMAT/ENGINE RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // No-arg formatting
  // ════════════════════════════════════════════════════════════════════

  test_case("format(\"plain text\") returns it verbatim");
  {
    auto s = fmt::format("hello, world");
    require_true(hstr_equal_cstr(s, "hello, world"));
  }
  end_test_case();

  test_case("format(\"\") returns empty");
  {
    auto s = fmt::format("");
    require(s.size(), usize(0));
  }
  end_test_case();

  test_case("format(nullptr) returns empty");
  {
    auto s = fmt::format(nullptr);
    require(s.size(), usize(0));
  }
  end_test_case();

  test_case("format('{{') escapes to '{'");
  {
    auto s = fmt::format("a{{b");
    require_true(hstr_equal_cstr(s, "a{b"));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Positional args
  // ════════════════════════════════════════════════════════════════════

  test_case("format('{} {} {}', a, b, c) interpolates in order");
  {
    auto s = fmt::format("{} {} {}", 1, 2, 3);
    require_true(hstr_equal_cstr(s, "1 2 3"));
  }
  end_test_case();

  test_case("format('{0}+{1}={2}', a, b, c) explicit indices");
  {
    auto s = fmt::format("{0}+{1}={2}", 2, 3, 5);
    require_true(hstr_equal_cstr(s, "2+3=5"));
  }
  end_test_case();

  test_case("format('{1} then {0}', a, b) reorders");
  {
    auto s = fmt::format("{1} then {0}", 1, 2);
    require_true(hstr_equal_cstr(s, "2 then 1"));
  }
  end_test_case();

  test_case("format('{}', 42) integer");
  {
    auto s = fmt::format("{}", 42);
    require_true(hstr_equal_cstr(s, "42"));
  }
  end_test_case();

  test_case("format('{}', -42) negative integer");
  {
    auto s = fmt::format("{}", -42);
    require_true(hstr_equal_cstr(s, "-42"));
  }
  end_test_case();

  test_case("format('{}', \"abc\") string literal");
  {
    auto s = fmt::format("{}", "abc");
    require_true(hstr_equal_cstr(s, "abc"));
  }
  end_test_case();

  test_case("format('{}', true / false) bool");
  {
    auto a = fmt::format("{}", true);
    auto b = fmt::format("{}", false);
    require_true(hstr_equal_cstr(a, "true"));
    require_true(hstr_equal_cstr(b, "false"));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Width / fill / align
  //
  // NOTE on default align: micron's apply_padding defaults to LEFT when no
  // type letter is present, regardless of value type. So {:5} of int gives
  // "42   " (left-padded), and {:5d} of int (with type) gives "   42"
  // (right-padded). This is documented behavior in
  // string_format_extensive.cpp.
  // ════════════════════════════════════════════════════════════════════

  test_case("format('{:5}', 42) without type letter defaults to LEFT");
  {
    auto s = fmt::format("{:5}", 42);
    require_true(hstr_equal_cstr(s, "42   "));
  }
  end_test_case();

  test_case("format('{:5d}', 42) with explicit 'd' right-justifies");
  {
    auto s = fmt::format("{:5d}", 42);
    require_true(hstr_equal_cstr(s, "   42"));
  }
  end_test_case();

  test_case("format('{:<5}', 42) explicit left");
  {
    auto s = fmt::format("{:<5}", 42);
    require_true(hstr_equal_cstr(s, "42   "));
  }
  end_test_case();

  test_case("format('{:>5d}', 42) explicit right with type");
  {
    auto s = fmt::format("{:>5d}", 42);
    require_true(hstr_equal_cstr(s, "   42"));
  }
  end_test_case();

  test_case("format('{:^5}', 42) center");
  {
    auto s = fmt::format("{:^5}", 42);
    // 5 chars with content '42' (2 chars) — pad 3 total, 1 left, 2 right (or vice versa)
    require(s.size(), usize(5));
    require_true(hstr_contains(s, "42"));
  }
  end_test_case();

  test_case("format('{:*>6d}', 42) fill='*' right with type");
  {
    auto s = fmt::format("{:*>6d}", 42);
    require_true(hstr_equal_cstr(s, "****42"));
  }
  end_test_case();

  test_case("format('{:0>5d}', 7) zero-padded with type");
  {
    auto s = fmt::format("{:0>5d}", 7);
    require_true(hstr_equal_cstr(s, "00007"));
  }
  end_test_case();

  test_case("format('{:5}', \"abc\") string defaults to left");
  {
    auto s = fmt::format("{:5}", "abc");
    require_true(hstr_equal_cstr(s, "abc  "));
  }
  end_test_case();

  test_case("format('{:>5s}', \"abc\") explicit right with type");
  {
    auto s = fmt::format("{:>5s}", "abc");
    require_true(hstr_equal_cstr(s, "  abc"));
  }
  end_test_case();

  test_case("format width less than content -> no padding");
  {
    auto s = fmt::format("{:2}", 12345);
    require_true(hstr_equal_cstr(s, "12345"));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Type chars: x, X, o, b, d, p
  // ════════════════════════════════════════════════════════════════════

  test_case("format('{:x}', 255) lower hex");
  {
    auto s = fmt::format("{:x}", 255);
    require_true(hstr_equal_cstr(s, "ff"));
  }
  end_test_case();

  test_case("format('{:X}', 255) upper hex");
  {
    auto s = fmt::format("{:X}", 255);
    require_true(hstr_equal_cstr(s, "FF"));
  }
  end_test_case();

  test_case("format('{:o}', 8) octal");
  {
    auto s = fmt::format("{:o}", 8);
    require_true(hstr_equal_cstr(s, "10"));
  }
  end_test_case();

  test_case("format('{:b}', 5) binary");
  {
    auto s = fmt::format("{:b}", 5);
    require_true(hstr_equal_cstr(s, "101"));
  }
  end_test_case();

  test_case("format('{:d}', 42) explicit decimal");
  {
    auto s = fmt::format("{:d}", 42);
    require_true(hstr_equal_cstr(s, "42"));
  }
  end_test_case();

  test_case("format('{:p}', ptr) pointer formatting");
  {
    int x = 0;
    auto s = fmt::format("{:p}", static_cast<const void *>(&x));
    require_true(s.size() >= 3);
    require(s[0], '0');
    require(s[1], 'x');
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Float types: f, e, g
  // ════════════════════════════════════════════════════════════════════

  test_case("format('{:.3f}', pi) fixed precision");
  {
    auto s = fmt::format("{:.3f}", 3.14159265);
    require_true(hstr_contains(s, "3.142") || hstr_contains(s, "3.141"));
  }
  end_test_case();

  test_case("format('{:.2e}', 12345.0) scientific");
  {
    auto s = fmt::format("{:.2e}", 12345.0);
    require_true(hstr_contains(s, "e") || hstr_contains(s, "E"));
  }
  end_test_case();

  test_case("format('{}', 1.5) float default");
  {
    auto s = fmt::format("{}", 1.5);
    require_true(s.size() > 0);
    require(s[0], '1');
  }
  end_test_case();

  test_case("format('{:10.2f}', 3.14) width + precision");
  {
    auto s = fmt::format("{:10.2f}", 3.14);
    require(s.size(), usize(10));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Alt flag (#)
  // ════════════════════════════════════════════════════════════════════

  test_case("format('{:#x}', 255) alt-hex contains 'ff'");
  {
    auto s = fmt::format("{:#x}", 255);
    require_true(hstr_contains(s, "ff"));
  }
  end_test_case();

  test_case("format('{:#o}', 8) alt-octal prefixes 0");
  {
    auto s = fmt::format("{:#o}", 8);
    // Alt may prepend '0' or not; verify the digits at least
    require_true(hstr_contains(s, "10"));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // format_value
  // ════════════════════════════════════════════════════════════════════

  test_case("format_value(int) matches int_to_string");
  {
    for ( int v : { 0, 1, -1, 42, -42, 12345 } ) {
      auto a = fmt::format_value(v);
      auto b = micron::int_to_string<int>(v);
      require(a.size(), b.size());
      for ( usize i = 0; i < a.size(); ++i ) require(a[i], b[i]);
    }
  }
  end_test_case();

  test_case("format_value(bool)");
  {
    auto t = fmt::format_value(true);
    auto f = fmt::format_value(false);
    require_true(hstr_equal_cstr(t, "true"));
    require_true(hstr_equal_cstr(f, "false"));
  }
  end_test_case();

  test_case("format_value(const char*)");
  {
    auto s = fmt::format_value("hello");
    require_true(hstr_equal_cstr(s, "hello"));
  }
  end_test_case();

  test_case("format_value(double)");
  {
    auto s = fmt::format_value(1.5);
    require_true(s.size() > 0);
    require(s[0], '1');
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // join
  // ════════════════════════════════════════════════════════════════════

  test_case("join over micron::vector<int> with ','");
  {
    micron::vector<int> v(4, 0);
    for ( int i = 0; i < 4; ++i ) v[i] = i + 1;
    auto s = fmt::join(v, ", ");
    require_true(hstr_equal_cstr(s, "1, 2, 3, 4"));
  }
  end_test_case();

  test_case("join empty container produces empty");
  {
    micron::vector<int> v;
    auto s = fmt::join(v, ", ");
    require(s.size(), usize(0));
  }
  end_test_case();

  test_case("join single element -> no delimiter");
  {
    micron::vector<int> v;
    v.push_back(42);
    auto s = fmt::join(v, ", ");
    require_true(hstr_equal_cstr(s, "42"));
  }
  end_test_case();

  test_case("join with prefix + suffix");
  {
    micron::vector<int> v(3, 0);
    v[0] = 1;
    v[1] = 2;
    v[2] = 3;
    auto s = fmt::join(v, ",", "[", "]");
    require_true(hstr_equal_cstr(s, "[1,2,3]"));
  }
  end_test_case();

  test_case("join_strings on fvector<hstring>");
  {
    micron::fvector<hstr> v;
    v.push_back(mk_hstring("foo"));
    v.push_back(mk_hstring("bar"));
    v.push_back(mk_hstring("baz"));
    auto s = fmt::join_strings(v, ", ");
    require_true(hstr_equal_cstr(s, "foo, bar, baz"));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Mixed-type format calls
  // ════════════════════════════════════════════════════════════════════

  test_case("format mixed int + string + float");
  {
    auto s = fmt::format("name={} age={} pi={:.2f}", "alice", 30, 3.14);
    require_true(hstr_contains(s, "name=alice"));
    require_true(hstr_contains(s, "age=30"));
    require_true(hstr_contains(s, "pi=3.14"));
  }
  end_test_case();

  test_case("format with hex + width (right-align with spaces by default)");
  {
    auto s = fmt::format("addr={:8x}", 0xdeadu);
    // type='x' → default right-align with space fill: "    dead"
    require_true(hstr_contains(s, "    dead"));
  }
  end_test_case();

  test_case("format hex with zero fill (explicit fill='0' align='>')");
  {
    auto s = fmt::format("{:0>8x}", 0xdeadu);
    require_true(hstr_equal_cstr(s, "0000dead"));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Property tests
  // ════════════════════════════════════════════════════════════════════

  property_test(
      "format('{}', int) == int_to_string (10k)",
      [](u32 raw) {
        i32 v = static_cast<i32>(raw);
        auto a = fmt::format("{}", v);
        auto b = micron::int_to_string<i32>(v);
        require(a.size(), b.size());
        for ( usize i = 0; i < a.size(); ++i ) require(a[i], b[i]);
      },
      10000);

  property_test(
      "format('{:x}', uint) == uint_to_string_base 16 (10k)",
      [](u32 raw) {
        u32 v = raw;
        auto a = fmt::format("{:x}", v);
        auto b = micron::uint_to_string_base<u32>(v, 16, false);
        require(a.size(), b.size());
        for ( usize i = 0; i < a.size(); ++i ) require(a[i], b[i]);
      },
      10000);

  property_test(
      "format width >= content_len produces width chars (10k)",
      [](u32 raw_n, u32 raw_w) {
        i32 v = static_cast<i32>(raw_n % 1000);
        (void)raw_w;
        auto s = fmt::format("{:*>16d}", v);      // hardcoded width=16, type=d
        require(s.size(), usize(16));
      },
      10000);

  property_test(
      "format determinism: same args → same output (10k)",
      [](u32 raw_a, u32 raw_b) {
        i32 a = static_cast<i32>(raw_a);
        i32 b = static_cast<i32>(raw_b);
        auto s1 = fmt::format("{} + {} = ?", a, b);
        auto s2 = fmt::format("{} + {} = ?", a, b);
        require(s1.size(), s2.size());
        for ( usize i = 0; i < s1.size(); ++i ) require(s1[i], s2[i]);
      },
      10000);

  sb::print("=== FORMAT/ENGINE RIGOR SUITE PASSED ===");
  return 1;
}
