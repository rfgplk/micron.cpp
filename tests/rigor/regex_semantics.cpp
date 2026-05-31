// regex_semantics.cpp
// Semantic rigor for the micron ERE engine: POSIX leftmost-longest matching,
// greedy quantifiers, nested captures, character-class edge cases, escapes and
// epsilon-loop safety. static_asserts prove the comptime path; runtime cases
// cover the loop-hazard patterns (so a hang shows up at run time, not compile).
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mc = micron;

// --- POSIX leftmost-longest: alternation must prefer the LONGER branch ------
static_assert(mc::cmatch<"(a|ab)">("ab").group(0).size() == 2);
static_assert(mc::cmatch<"(ab|a)">("ab").group(0).size() == 2);
static_assert(mc::cmatch<"(a|ab|abc)">("abc").group(0).size() == 3);

// --- greedy quantifiers -----------------------------------------------------
static_assert(mc::cmatch<"a.*b">("axbxb").group(0).size() == 5);
static_assert(mc::cmatch<"a.*">("abc").group(0).size() == 3);
static_assert(mc::cmatch<"b*">("bbb").group(0).size() == 3);

// --- leftmost position ------------------------------------------------------
static_assert(mc::cmatch<"a+">("bbaaa").group_start(0) == 2);
static_assert(mc::cmatch<"a+">("bbaaa").group(0).size() == 3);

// --- empty / zero-width matches --------------------------------------------
static_assert(mc::cmatch<"a*">("").has_match());
static_assert(mc::cmatch<"a*">("").group(0).size() == 0);
static_assert(mc::cmatch<"a*">("bbb").group(0).size() == 0);      // empty match at offset 0

// --- nested capture groups --------------------------------------------------
static_assert(mc::cmatch<"((a)(b))">("ab").groups() == 4);
static_assert(mc::cmatch<"((a)(b))">("ab").group(1).size() == 2);
static_assert(mc::cmatch<"((a)(b))">("ab").group(2).size() == 1);
static_assert(mc::cmatch<"((a)(b))">("ab").group(3).size() == 1);

// --- escapes ----------------------------------------------------------------
static_assert(mc::cmatch<"a\\.b">("a.b").has_match());
static_assert(!mc::cmatch<"a\\.b">("axb").has_match());
static_assert(mc::cmatch<"\\(x\\)">("(x)").has_match());

// --- character-class edge cases --------------------------------------------
static_assert(mc::cmatch<"[]a]">("]").has_match());      // ']' first is literal
static_assert(mc::cmatch<"[-a]">("-").has_match());      // '-' first is literal
static_assert(mc::cmatch<"[a-]">("-").has_match());      // '-' last is literal
static_assert(mc::cmatch<"[a-c]+">("abcabc").group(0).size() == 6);
static_assert(!mc::cmatch<"[^a-c]">("b").has_match());

// --- intervals --------------------------------------------------------------
static_assert(mc::cmatch<"a{2,}">("aaaa").group(0).size() == 4);
static_assert(mc::cmatch<"(ab){2,3}">("ababab").group(0).size() == 6);
static_assert(!mc::cmatch<"^a{2,}$">("a").has_match());

// --- alternation with an empty branch --------------------------------------
static_assert(mc::cmatch<"(a|)b">("b").has_match());
static_assert(mc::cmatch<"x(a|)b">("xab").has_match());

int
main()
{
  print("=== REGEX SEMANTICS TESTS ===");

  test_case("epsilon-loop safety (must terminate)");
  {
    mc::regex r1("(a*)*");
    require_true(r1.has_match("aaa"));
    mc::regex r2("(a?)*");
    require_true(r2.has_match("b"));      // empty match
    mc::regex r3("(a*)+b");
    require_true(r3.has_match("aaab"));
    require_true(r3.has_match("b"));
  }
  end_test_case();

  test_case("leftmost-longest runtime spans");
  {
    mc::regex re("(a|ab)");
    mc::rmatch m = re.match("ab");
    require_true(m.group(0).len == 2);      // POSIX longest

    mc::regex re2("a.*b");
    mc::rmatch m2 = re2.match("zaxbxby");
    require_true(m2.group(0).len == 5);      // "axbxb"
    require_true(m2.group(0).ptr[0] == 'a');
  }
  end_test_case();

  test_case("nested group spans runtime");
  {
    mc::regex re("((a+)(b+))");
    mc::rmatch m = re.match("aaabb");
    require_true(m.groups() == 4);
    require_true(m.group(0).len == 5);
    require_true(m.group(2).len == 3);      // a+
    require_true(m.group(3).len == 2);      // b+
    require_true(m.group(2).ptr[0] == 'a' && m.group(3).ptr[0] == 'b');
  }
  end_test_case();

  print("=== ALL REGEX SEMANTICS TESTS PASSED ===");
  return 1;
}
