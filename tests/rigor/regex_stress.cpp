// regex_stress.cpp
// Adversarial / large-input rigor. Patterns that cause CATASTROPHIC backtracking
// in a backtracking engine (nested quantifiers) are run over large inputs: the
// linear Pike VM / DFA completes instantly and returns the correct result. Also
// exercises deep nesting, long alternations, and large literal/class throughput.
// If this test ever hangs, the engine has lost its linear-time guarantee.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mc = micron;

// fill an allocated buffer with `n` copies of `ch` then a suffix, NUL-terminated
static char *
make_buf(usize n, char ch, const char *suffix, usize &out_len)
{
  usize sl = 0;
  while ( suffix[sl] ) ++sl;
  char *b = mc::alloc<char>(n + sl + 1);
  for ( usize i = 0; i < n; ++i ) b[i] = ch;
  for ( usize i = 0; i < sl; ++i ) b[n + i] = suffix[i];
  b[n + sl] = 0;
  out_len = n + sl;
  return b;
}

int
main()
{
  print("=== REGEX STRESS (adversarial + large input, must stay linear) ===");

  test_case("catastrophic-backtracking patterns stay linear + correct");
  {
    const usize N = 50000;
    usize len;

    char *aN = make_buf(N, 'a', "", len);           // N 'a'
    char *aNbang = make_buf(N, 'a', "!", len);      // N 'a' then '!'

    require_true(mc::regex("(a+)+$").has_match(aN));           // matches the a-run
    require_true(!mc::regex("(a+)+$").has_match(aNbang));      // '!' defeats the $; no exponential blowup
    require_true(!mc::regex("(a*)*b").has_match(aN));          // no 'b'
    require_true(!mc::regex("(a|a)*c").has_match(aN));         // no 'c'
    require_true(!mc::regex("(.*)*x").has_match(aN));          // no 'x'
    require_true(!mc::regex(".*.*.*.*y").has_match(aN));       // no 'y'

    char *aNb;
    usize l2;
    aNb = make_buf(N, 'a', "b", l2);
    require_true(mc::regex("(a*)*b").has_match(aNb));      // now there IS a b

    mc::free(aN);
    mc::free(aNbang);
    mc::free(aNb);
  }
  end_test_case();

  test_case("large literal / class throughput");
  {
    const usize N = 1u * 1000u * 1000u;      // 1 MB of 'a'
    usize len;

    char *lit = make_buf(N, 'a', "ZZZ", len);
    mc::regex re("ZZ+");
    mc::rmatch m = re.search_n(lit, len);
    require_true(m.has_match() && m.group_start(0) == (long)N && m.group(0).len == 3);
    mc::free(lit);

    char *cls = make_buf(N, 'a', "789", len);
    mc::regex re2("[0-9]{3}");
    mc::rmatch m2 = re2.search_n(cls, len);
    require_true(m2.has_match() && m2.group_start(0) == (long)N);
    mc::free(cls);

    // class-led pattern over a big buffer (truffle prefilter)
    char *cls2 = make_buf(N, 'a', "qer", len);
    require_true(mc::regex("[q-s]e").has_match_n(cls2, len));
    require_true(!mc::regex("[w-z]e").has_match_n(cls2, len));
    mc::free(cls2);
  }
  end_test_case();

  test_case("deep nesting + long alternation");
  {
    require_true(mc::regex("(((((((((a)))))))))").match("a").group(9).len == 1);
    require_true(mc::regex("a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p").has_match("zzznzzz"));
    require_true(!mc::regex("a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p").has_match("ZZZ"));
    // long concatenation
    require_true(mc::regex("abcdefghijklmnop").has_match("xxabcdefghijklmnopxx"));
  }
  end_test_case();

  print("=== ALL REGEX STRESS TESTS PASSED ===");
  return 1;
}
