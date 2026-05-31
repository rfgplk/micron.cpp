// regex_basic.cpp
// First rigor suite for micron's constexpr POSIX-ERE engine (src/regex/).
// The file-scope static_asserts force the entire pipeline (parse -> compile ->
// Pike VM) through constant evaluation, proving the engine is genuinely
// constexpr. The runtime cases exercise the heap-backed micron::regex object
// and capture extraction.
//
// snowball convention: exit 1 == success; judge by the banner.
// Build & run: duck build tests/rigor/regex_basic.cpp && ./bin/regex_basic

#include "../../src/regex.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mc = micron;

// ===========================================================================
// compile-time proofs
// ===========================================================================

// literals + search semantics
static_assert(mc::cmatch<"abc">("abc").has_match());
static_assert(mc::cmatch<"abc">("xxabcyy").has_match());
static_assert(!mc::cmatch<"abc">("abx").has_match());

// '.', '*', '+', '?'
static_assert(mc::cmatch<"a.c">("axc").has_match());
static_assert(!mc::cmatch<"a.c">("ac").has_match());
static_assert(mc::cmatch<"a*b">("b").has_match());
static_assert(mc::cmatch<"a*b">("aaab").has_match());
static_assert(mc::cmatch<"a+">("aaa").has_match());
static_assert(!mc::cmatch<"a+b">("b").has_match());
static_assert(mc::cmatch<"colou?r">("color").has_match());
static_assert(mc::cmatch<"colou?r">("colour").has_match());

// alternation + grouping
static_assert(mc::cmatch<"(a|b)c">("bc").has_match());
static_assert(mc::cmatch<"(foo|bar|baz)!">("bar!").has_match());
static_assert(!mc::cmatch<"(a|b)c">("cc").has_match());

// bracket classes, ranges, negation, POSIX classes
static_assert(mc::cmatch<"[0-9]+">("xx42yy").has_match());
static_assert(mc::cmatch<"[^0-9]+">("abc").has_match());
static_assert(mc::cmatch<"[[:alpha:]]+">("Hello").has_match());
static_assert(!mc::cmatch<"[[:digit:]]+">("abc").has_match());

// anchors
static_assert(mc::cmatch<"^abc$">("abc").has_match());
static_assert(!mc::cmatch<"^abc$">("xabc").has_match());
static_assert(!mc::cmatch<"^abc$">("abcx").has_match());

// intervals {m,n}
static_assert(mc::cmatch<"a{2,3}">("aaaa").has_match());
static_assert(!mc::cmatch<"a{2,3}">("a").has_match());
static_assert(mc::cmatch<"a{3}">("aaa").has_match());
static_assert(!mc::cmatch<"xa{3}x">("xaax").has_match());

// captures at compile time
constexpr auto kcap = mc::cmatch<"(ab)(cd)">("abcd");
static_assert(kcap.has_match());
static_assert(kcap.groups() == 3);      // group 0 + 2 captures
static_assert(kcap.group(0).size() == 4);
static_assert(kcap.group(1).size() == 2);
static_assert(kcap.group(2).size() == 2);

constexpr auto krep = mc::cmatch<"(ab)+">("abab");
static_assert(krep.group(1).size() == 2);      // last iteration

// ===========================================================================
// runtime
// ===========================================================================

int
main()
{
  print("=== REGEX BASIC TESTS ===");

  test_case("runtime literal + search");
  {
    mc::regex re("abc");
    require_true(re.valid());
    require_true(re.has_match("xxabcyy"));
    require_true(!re.has_match("ab"));
  }
  end_test_case();

  test_case("runtime quantifiers / alternation");
  {
    mc::regex re("a+(b|c)*");
    require_true(re.has_match("aabc"));
    require_true(re.has_match("a"));
    require_true(!re.has_match("xyz"));
  }
  end_test_case();

  test_case("runtime capture extraction");
  {
    mc::regex re("(a+)(b+)");
    mc::rmatch m = re.match("xxaaabbyy");
    require_true(m.has_match());
    require_true(m.groups() == 3);
    auto g1 = m.group(1);
    auto g2 = m.group(2);
    require_true(g1.len == 3 && g2.len == 2);
    require_true(g1.ptr[0] == 'a' && g1.ptr[2] == 'a');
    require_true(g2.ptr[0] == 'b' && g2.ptr[1] == 'b');
  }
  end_test_case();

  test_case("runtime character classes");
  {
    mc::regex re("[[:digit:]]+");
    mc::rmatch m = re.match("abc12345xyz");
    require_true(m.has_match());
    require_true(m.group(0).len == 5);
    require_true(m.group(0).ptr[0] == '1');
  }
  end_test_case();

  test_case("runtime anchors");
  {
    mc::regex re("^[a-z]+$");
    require_true(re.has_match("hello"));
    require_true(!re.has_match("hello1"));
  }
  end_test_case();

  print("=== ALL REGEX BASIC TESTS PASSED ===");
  return 1;
}
