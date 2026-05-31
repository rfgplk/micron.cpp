// regex_quantifiers.cpp
// Exhaustive quantifier rigor: * + ? and intervals {m} {m,} {m,n} on atoms,
// classes, '.', and groups; exact greedy match lengths and boundaries; nested
// quantifiers (termination + correctness). Compile-time + runtime.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mc = micron;

// whole-match length of the leftmost match, -1 if none, -2 if pattern invalid
static long
wlen(const char *pat, const char *in)
{
  mc::regex re(pat);
  if ( !re.valid() ) return -2;
  mc::rmatch m = re.match(in);
  return m.has_match() ? (long)m.group(0).len : -1;
}

int
main()
{
  print("=== REGEX QUANTIFIERS (exhaustive) ===");

  test_case("* + ? greedy lengths");
  {
    require_true(wlen("a*", "aaaa") == 4);
    require_true(wlen("a*", "bbb") == 0);      // empty match at 0
    require_true(wlen("a*", "") == 0);
    require_true(wlen("a+", "aaaa") == 4);
    require_true(wlen("a+", "b") == -1);
    require_true(wlen("a?", "aaa") == 1);
    require_true(wlen("a?", "b") == 0);
    require_true(wlen("ab?c", "ac") == 2);
    require_true(wlen("ab?c", "abc") == 3);
  }
  end_test_case();

  test_case("interval {m} {m,} {m,n} exact counts");
  {
    require_true(wlen("a{3}", "aaaaa") == 3);
    require_true(wlen("^a{3}$", "aa") == -1);
    require_true(wlen("a{2,4}", "aaaaa") == 4);      // greedy cap
    require_true(wlen("a{2,4}", "aaa") == 3);
    require_true(wlen("a{2,4}", "aa") == 2);
    require_true(wlen("a{2,4}", "a") == -1);
    require_true(wlen("a{2,}", "aaaaaa") == 6);
    require_true(wlen("a{0,2}", "aaa") == 2);
    require_true(wlen("a{0}", "aaa") == 0);
    require_true(wlen("a{0,0}", "aaa") == 0);
    require_true(wlen("a{1,1}", "aaa") == 1);
  }
  end_test_case();

  test_case("quantifiers on classes / dot / groups");
  {
    require_true(wlen("[0-9]+", "12345") == 5);
    require_true(wlen("[a-z]{2,3}", "abcd") == 3);
    require_true(wlen(".*", "hello") == 5);
    require_true(wlen(".+", "") == -1);
    require_true(wlen("(ab)+", "ababab") == 6);
    require_true(wlen("(ab){2}", "ababab") == 4);
    require_true(wlen("(ab){2,3}", "ababababab") == 6);
    require_true(wlen("(abc)?", "abc") == 3);
    require_true(wlen("(abc)?", "xyz") == 0);
  }
  end_test_case();

  test_case("combinations + greedy interplay");
  {
    require_true(wlen("a{2}b{3}", "aabbb") == 5);
    require_true(wlen("a*b*", "aaabb") == 5);
    require_true(wlen("a.*b", "axbxxb") == 6);       // greedy .* extends to last b
    require_true(wlen("a.*?b", "axbxxb") == 6);      // no lazy in ERE: '?' applies to nothing meaningful here
    require_true(wlen("x[0-9]*y", "x123y") == 5);
  }
  end_test_case();

  test_case("nested quantifiers terminate + correct");
  {
    require_true(wlen("(a*)*", "aaa") == 3);
    require_true(wlen("(a+)+", "aaaa") == 4);
    require_true(wlen("(a?)*", "aaa") == 3);
    require_true(wlen("(a*)+b", "aaab") == 4);
    require_true(wlen("(a*)+b", "b") == 1);
    require_true(wlen("(.*)*x", "aaaax") == 5);
  }
  end_test_case();

  // ---- compile-time proofs ----
  static_assert(mc::cmatch<"a{3}">("aaaaa").group(0).size() == 3);
  static_assert(mc::cmatch<"a{2,4}">("aaaaa").group(0).size() == 4);
  static_assert(!mc::cmatch<"^a{2,4}$">("a").has_match());
  static_assert(mc::cmatch<"(ab){2,3}">("ababab").group(0).size() == 6);
  static_assert(mc::cmatch<"a*b*">("aaabb").group(0).size() == 5);
  static_assert(mc::cmatch<"(a*)*">("aaa").group(0).size() == 3);

  print("=== ALL REGEX QUANTIFIERS TESTS PASSED ===");
  return 1;
}
