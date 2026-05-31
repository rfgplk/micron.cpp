// regex_api.cpp
// Exhaustive API-surface rigor: match/search/has_match and the _n length-aware
// variants, rmatch accessors, empty patterns/inputs, invalid patterns, move
// semantics, repeated reuse, and const char* vs micron string inputs.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mc = micron;

int
main()
{
  print("=== REGEX API (exhaustive) ===");

  test_case("match / search / has_match equivalence (unanchored leftmost)");
  {
    mc::regex re("b+");
    mc::rmatch a = re.match("aabbbcc");
    mc::rmatch b = re.search("aabbbcc");
    require_true(a.has_match() && b.has_match());
    require_true(a.group_start(0) == 2 && a.group_end(0) == 5);
    require_true(b.group_start(0) == 2 && b.group_end(0) == 5);
    require_true(re.has_match("aabbbcc"));
    require_true(!re.has_match("aacc"));
  }
  end_test_case();

  test_case("rmatch accessors");
  {
    mc::regex re("(a)(b)");
    mc::rmatch m = re.match("ab");
    require_true(m.has_match());
    require_true((bool)m == true);      // operator bool
    require_true(m.groups() == 3);
    require_true(m.group(1).len == 1 && m.group(1).ptr != nullptr);
    require_true(m.group_start(2) == 1 && m.group_end(2) == 2);

    mc::rmatch nm = re.match("xy");      // no match
    require_true(!nm.has_match());
    require_true((bool)nm == false);
    require_true(nm.group(0).len == 0 && nm.group(0).ptr == nullptr);
    require_true(nm.group_start(0) == -1 && nm.group_end(0) == -1);
  }
  end_test_case();

  test_case("_n explicit-length variants + embedded NUL");
  {
    mc::regex re("c+");
    char data[7] = { 'a', 'c', 'c', '\0', 'c', 'c', 'c' };
    mc::rmatch m = re.search_n(data, 7);
    require_true(m.group_start(0) == 1 && m.group_end(0) == 3);      // leftmost "cc" before NUL
    require_true(re.has_match_n(data, 7));
    mc::rmatch m2 = re.match_n(data + 4, 3);      // past the NUL
    require_true(m2.group_start(0) == 0 && m2.group_end(0) == 3);
    // strlen-view stops at the NUL
    mc::regex re2("c{3}");
    require_true(!re2.has_match(data));          // only "cc" before NUL
    require_true(re2.has_match_n(data, 7));      // "ccc" after NUL
  }
  end_test_case();

  test_case("empty pattern + empty input");
  {
    mc::regex empty("");
    mc::rmatch m = empty.match("abc");
    require_true(m.has_match() && m.group(0).len == 0 && m.group_start(0) == 0);
    require_true(empty.has_match(""));

    mc::regex star("a*");
    require_true(star.has_match(""));      // nullable matches empty
    require_true(star.match("").group(0).len == 0);

    mc::regex lit("a");
    require_true(!lit.has_match(""));      // non-nullable on empty input
  }
  end_test_case();

  test_case("invalid patterns");
  {
    const char *bad[] = { "[", "a(b", "a)b", "[z-a]", "*abc" };
    // note: "*abc" -- a leading quantifier with no atom: micron treats '*' as a
    // literal (lenient), so it is actually VALID. Check the genuinely-bad ones:
    require_true(!mc::regex("[").valid());
    require_true(!mc::regex("a(b").valid());
    require_true(!mc::regex("a)b").valid());
    require_true(!mc::regex("[z-a]").valid());
    (void)bad;
    mc::regex iv("[");
    require_true(!iv.has_match("anything"));      // invalid -> never matches
    require_true(!iv.match("anything").has_match());
  }
  end_test_case();

  test_case("move semantics");
  {
    mc::regex a("[0-9]+");
    require_true(a.has_match("x42"));
    mc::regex b(static_cast<mc::regex &&>(a));      // move-construct
    require_true(b.has_match("x42") && b.match("x42").group(0).len == 2);

    mc::regex c("zzz");
    c = static_cast<mc::regex &&>(b);      // move-assign
    require_true(c.has_match("x42") && c.match("x42").group(0).len == 2);
  }
  end_test_case();

  test_case("repeated reuse of one regex object");
  {
    mc::regex re("a(b+)c");
    for ( int i = 0; i < 100; ++i ) {
      mc::rmatch m = re.match("xxabbbcyy");
      require_true(m.has_match() && m.group(1).len == 3);
    }
    require_true(!re.has_match("ac"));
  }
  end_test_case();

  test_case("string-object inputs (has_cstr)");
  {
    mc::regex re("[a-z]+");
    mc::string s("123abc456");
    require_true(re.has_match(s));
    require_true(re.match(s).group(0).len == 3);
    mc::sstr<16> ss("XYabZ");
    require_true(re.match(ss).group(0).len == 2);      // "ab"
  }
  end_test_case();

  print("=== ALL REGEX API TESTS PASSED ===");
  return 1;
}
