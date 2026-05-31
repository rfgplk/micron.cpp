// regex_constexpr.cpp
// Consolidated compile-time proof that the ENTIRE engine -- parse, compile,
// Pike VM, captures -- is usable in constant evaluation across every ERE
// construct. Every assertion below is a static_assert, so the file failing to
// compile IS the failing test. The runtime main() merely prints the banner.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;

// literals & search
static_assert(mc::cmatch<"abc">("abc").has_match());
static_assert(mc::cmatch<"abc">("zzabczz").has_match());
static_assert(!mc::cmatch<"abc">("ab").has_match());

// dot / any
static_assert(mc::cmatch<"a.c">("axc").has_match());
static_assert(!mc::cmatch<"a.c">("ac").has_match());
static_assert(mc::cmatch<"...">("xyz").group(0).size() == 3);

// star / plus / quest
static_assert(mc::cmatch<"a*">("aaaa").group(0).size() == 4);
static_assert(mc::cmatch<"a*">("bbb").group(0).size() == 0);
static_assert(mc::cmatch<"a+">("aaa").has_match());
static_assert(!mc::cmatch<"a+">("").has_match());
static_assert(mc::cmatch<"ab?c">("ac").has_match());

// intervals
static_assert(mc::cmatch<"a{3}">("aaa").has_match());
static_assert(mc::cmatch<"a{2,4}">("aaaaa").group(0).size() == 4);
static_assert(mc::cmatch<"a{2,}">("aaaaaa").group(0).size() == 6);
static_assert(mc::cmatch<"a{0}">("aaa").group(0).size() == 0);
static_assert(!mc::cmatch<"^a{3}$">("aa").has_match());

// alternation + leftmost-longest
static_assert(mc::cmatch<"cat|dog">("a dog here").has_match());
static_assert(mc::cmatch<"(a|ab)">("ab").group(0).size() == 2);      // POSIX longest
static_assert(mc::cmatch<"(ab|a)">("ab").group(0).size() == 2);

// groups + captures
static_assert(mc::cmatch<"(a)(b)(c)">("abc").groups() == 4);
static_assert(mc::cmatch<"((a)(b))">("ab").group(3).size() == 1);
static_assert(mc::cmatch<"(a)?(b)">("b").group_start(1) == -1);
static_assert(mc::cmatch<"(ab)+">("abab").group_start(1) == 2);
static_assert(mc::cmatch<"x(ab)y">("zxaby").group_start(1) == 2);

// classes
static_assert(mc::cmatch<"[0-9]+">("ab42cd").group(0).size() == 2);
static_assert(mc::cmatch<"[^0-9]+">("ABC1").group(0).size() == 3);
static_assert(mc::cmatch<"[[:alpha:]]+">("Hi42").group(0).size() == 2);
static_assert(mc::cmatch<"[]a]+">("]a]b").group(0).size() == 3);
static_assert(mc::cmatch<"[a-c-e]+">("abce-x").group(0).size() == 5);

// anchors
static_assert(mc::cmatch<"^abc$">("abc").has_match());
static_assert(!mc::cmatch<"^abc$">("abcd").has_match());
static_assert(mc::cmatch<"^$">("").has_match());

// escapes
static_assert(mc::cmatch<"a\\.b">("a.b").has_match());
static_assert(!mc::cmatch<"a\\.b">("axb").has_match());
static_assert(mc::cmatch<"\\(x\\)">("(x)").has_match());

// nested quantifiers terminate at compile time too
static_assert(mc::cmatch<"(a*)*">("aaa").group(0).size() == 3);
static_assert(mc::cmatch<"(a+)+">("aaaa").group(0).size() == 4);

// a fully comptime-built result reused
constexpr auto kM = mc::cmatch<"([a-z]+)@([a-z]+)">("user@host");
static_assert(kM.has_match());
static_assert(kM.group(1).size() == 4 && kM.group(2).size() == 4);

int
main()
{
  sb::print("=== REGEX CONSTEXPR PROOFS ===");
  sb::print("=== ALL REGEX CONSTEXPR TESTS PASSED (at compile time) ===");
  return 1;
}
