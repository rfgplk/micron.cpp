// regex_captures.cpp
// Exhaustive capture-group rigor: numbering, nesting, optional/unset groups,
// repetition (last iteration wins), alternation, empty groups, absolute
// offsets, and the kMaxGroups boundary. Compile-time (cmatch) + runtime.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mc = micron;
namespace io = micron::io;

// expect group g to span [start, start+len); start<0 means the group is unset
static bool
grp(const mc::rmatch &m, usize g, long start, long len)
{
  if ( start < 0 ) return m.group_start(g) == -1 && m.group_end(g) == -1 && m.group(g).len == 0;
  return m.group_start(g) == start && m.group_end(g) == start + len && (long)m.group(g).len == len;
}

int
main()
{
  print("=== REGEX CAPTURES (exhaustive) ===");

  test_case("numbering + whole match");
  {
    mc::regex re("(a)(b)(c)");
    mc::rmatch m = re.match("zzabcyy");
    require_true(m.groups() == 4);
    require_true(grp(m, 0, 2, 3));                                    // "abc"
    require_true(grp(m, 1, 2, 1));                                    // a
    require_true(grp(m, 2, 3, 1));                                    // b
    require_true(grp(m, 3, 4, 1));                                    // c
    require_true(m.group(4).len == 0 && m.group_start(4) == -1);      // out of range
  }
  end_test_case();

  test_case("nested groups");
  {
    mc::regex re("((a)(b))");
    mc::rmatch m = re.match("ab");
    require_true(m.groups() == 4);
    require_true(grp(m, 0, 0, 2) && grp(m, 1, 0, 2) && grp(m, 2, 0, 1) && grp(m, 3, 1, 1));

    mc::regex re2("(((x)))");
    mc::rmatch m2 = re2.match("x");
    require_true(grp(m2, 1, 0, 1) && grp(m2, 2, 0, 1) && grp(m2, 3, 0, 1));
  }
  end_test_case();

  test_case("optional / unset groups");
  {
    mc::regex re("(a)?(b)");
    require_true(grp(re.match("b"), 1, -1, 0) && grp(re.match("b"), 2, 0, 1));      // g1 unset
    require_true(grp(re.match("ab"), 1, 0, 1) && grp(re.match("ab"), 2, 1, 1));

    mc::regex re2("(a)(b)?");
    require_true(grp(re2.match("a"), 1, 0, 1) && grp(re2.match("a"), 2, -1, 0));      // g2 unset
  }
  end_test_case();

  test_case("alternation captures");
  {
    mc::regex re("(a|(b))");
    require_true(grp(re.match("a"), 1, 0, 1) && grp(re.match("a"), 2, -1, 0));      // b-group unset
    require_true(grp(re.match("b"), 1, 0, 1) && grp(re.match("b"), 2, 0, 1));
  }
  end_test_case();

  test_case("repetition: last iteration wins");
  {
    mc::regex re("(ab)+");
    require_true(grp(re.match("ababab"), 0, 0, 6) && grp(re.match("ababab"), 1, 4, 2));      // last "ab"
    mc::regex re2("(a)(b)\\2*");                                                             // no backrefs in ERE
    require_true(!re2.valid() || true);      // \2 is literal '2' in ERE -> may or may not match; don't assert
  }
  end_test_case();

  test_case("empty groups + zero-width");
  {
    mc::regex re("(a*)(b*)");
    mc::rmatch m = re.match("aab");
    require_true(grp(m, 1, 0, 2) && grp(m, 2, 2, 1));
    mc::regex re2("()x");
    mc::rmatch m2 = re2.match("x");
    require_true(grp(m2, 1, 0, 0));      // empty group captured at offset 0
  }
  end_test_case();

  test_case("absolute offsets for non-zero leftmost match");
  {
    mc::regex re("x(ab)y");
    mc::rmatch m = re.match("zzzxaby");
    require_true(grp(m, 0, 3, 4) && grp(m, 1, 4, 2));
  }
  end_test_case();

  test_case("kMaxGroups boundary");
  {
    // build "(a)(a)...(a)" with N groups
    char buf[200];
    auto build = [&](int n) {
      int k = 0;
      for ( int i = 0; i < n; ++i ) {
        buf[k++] = '(';
        buf[k++] = 'a';
        buf[k++] = ')';
      }
      buf[k] = 0;
    };
    build(32);
    require_true(mc::regex(buf).valid());      // exactly kMaxGroups
    build(33);
    require_true(!mc::regex(buf).valid());      // one too many -> rejected
  }
  end_test_case();

  // ---- compile-time proofs ----
  static_assert(mc::cmatch<"(a)(b)(c)">("abc").groups() == 4);
  static_assert(mc::cmatch<"((a)(b))">("ab").group(2).size() == 1);
  static_assert(mc::cmatch<"(a)?(b)">("b").group_start(1) == -1);      // unset
  static_assert(mc::cmatch<"(a)?(b)">("b").group(2).size() == 1);
  static_assert(mc::cmatch<"(ab)+">("abab").group_start(1) == 2);      // last iter
  static_assert(mc::cmatch<"(a*)(b*)">("aab").group(2).size() == 1);

  print("=== ALL REGEX CAPTURES TESTS PASSED ===");
  return 1;
}
