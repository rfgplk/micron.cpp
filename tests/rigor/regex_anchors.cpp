// regex_anchors.cpp
// Exhaustive anchor rigor: ^ and $ in every position, with alternation,
// quantifiers, empty strings, redundant and mid-pattern (never-match) anchors.
// Verifies both match presence and the leftmost offset.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mc = micron;

// leftmost match offset, -1 if none
static long
mpos(const char *pat, const char *in)
{
  mc::regex re(pat);
  if ( !re.valid() ) return -2;
  mc::rmatch m = re.match(in);
  return m.has_match() ? (long)m.group_start(0) : -1;
}

static long
mlen(const char *pat, const char *in)
{
  mc::regex re(pat);
  if ( !re.valid() ) return -2;
  mc::rmatch m = re.match(in);
  return m.has_match() ? (long)m.group(0).len : -1;
}

int
main()
{
  print("=== REGEX ANCHORS (exhaustive) ===");

  test_case("^ start anchor");
  {
    require_true(mpos("^abc", "abcabc") == 0);
    require_true(mpos("^abc", "xabc") == -1);
    require_true(mpos("^a", "aaa") == 0);
  }
  end_test_case();

  test_case("$ end anchor");
  {
    require_true(mpos("abc$", "abcabc") == 3);
    require_true(mpos("abc$", "abcx") == -1);
    require_true(mpos("c$", "abc") == 2);
  }
  end_test_case();

  test_case("^...$ whole-string");
  {
    require_true(mpos("^abc$", "abc") == 0 && mlen("^abc$", "abc") == 3);
    require_true(mpos("^abc$", "abcd") == -1);
    require_true(mpos("^abc$", "xabc") == -1);
    require_true(mpos("^a*$", "aaa") == 0 && mlen("^a*$", "aaa") == 3);
    require_true(mpos("^a*$", "aab") == -1);
    require_true(mpos("^a+$", "") == -1);
    require_true(mpos("^.*$", "hello") == 0 && mlen("^.*$", "hello") == 5);
  }
  end_test_case();

  test_case("empty-string anchors");
  {
    require_true(mpos("^$", "") == 0 && mlen("^$", "") == 0);
    require_true(mpos("^$", "a") == -1);
    require_true(mpos("^", "abc") == 0 && mlen("^", "abc") == 0);
    require_true(mpos("$", "abc") == 3 && mlen("$", "abc") == 0);      // $ matches empty at end
    require_true(mpos("$", "") == 0);
  }
  end_test_case();

  test_case("anchors with alternation (precedence: | is lowest)");
  {
    // ^a|b  ==  (^a)|(b)
    require_true(mpos("^a|b", "ax") == 0);       // ^a at 0
    require_true(mpos("^a|b", "xb") == 1);       // b branch, unanchored
    require_true(mpos("^a|b", "xa") == -1);      // ^a fails (not at 0), no b
    // a$|b  ==  (a$)|(b)
    require_true(mpos("a$|b", "xa") == 1);       // a at end
    require_true(mpos("a$|b", "ax") == -1);      // a not at end, no b
    require_true(mpos("a$|b", "bx") == 0);       // b at 0
  }
  end_test_case();

  test_case("never-match + redundant anchors");
  {
    require_true(mpos("a^b", "ab") == -1);        // start-of-text after consuming a
    require_true(mpos("a$b", "ab") == -1);        // end-of-text before b
    require_true(mpos("^^abc", "abc") == 0);      // redundant ^
    require_true(mpos("abc$$", "abc") == 0);      // redundant $
    require_true(mpos("^a$", "a") == 0 && mlen("^a$", "a") == 1);
    require_true(mpos("^a$", "aa") == -1);
  }
  end_test_case();

  // ---- compile-time proofs ----
  static_assert(mc::cmatch<"^abc$">("abc").has_match());
  static_assert(!mc::cmatch<"^abc$">("xabc").has_match());
  static_assert(!mc::cmatch<"^abc$">("abcx").has_match());
  static_assert(mc::cmatch<"^$">("").has_match());
  static_assert(!mc::cmatch<"^$">("a").has_match());
  static_assert(mc::cmatch<"a$|b">("xa").group_start(0) == 1);
  static_assert(!mc::cmatch<"a^b">("ab").has_match());

  print("=== ALL REGEX ANCHORS TESTS PASSED ===");
  return 1;
}
