// regex_sheng.cpp
// Verifies the Sheng SIMD DFA has_match() accelerator against glibc POSIX
// ground truth over the generated fuzz corpus. has_match() transparently uses
// Sheng for anchor-free <=16-state patterns and the Pike VM otherwise; both
// must agree with glibc on "does the pattern match anywhere". Also reports how
// many cases actually exercised the Sheng path.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex.hpp"

#include "../snowball/snowball.hpp"

#include "regex_corpus.hpp"
#include "regex_fuzz.hpp"

using sb::print;
using sb::require_true;

namespace mc = micron;
namespace io = micron::io;

int
main()
{
  print("=== REGEX SHENG (SIMD has_match) vs glibc ===");

  int fails = 0, shown = 0, sheng_cases = 0, table_cases = 0, pike_cases = 0;

  for ( int i = 0; i < REGEX_FUZZ_N; ++i ) {
    const long *E = REGEX_FUZZ_EXPECTED[i];
    if ( E[0] == -2 ) continue;      // glibc had no ground truth
    const char *pat = REGEX_FUZZ[i].pat;
    const char *in = REGEX_FUZZ[i].in;

    mc::regex re(pat);
    bool want = (E[0] == 1);
    bool got = re.has_match(in);
    if ( re.uses_sheng() )
      ++sheng_cases;
    else if ( re.uses_dfa() )
      ++table_cases;
    else
      ++pike_cases;
    if ( got != want ) {
      ++fails;
      if ( shown++ < 40 )
        io::print("  has_match FAIL [", i, "] pat=`", pat, "` in=`", in, "` sheng=", (int)re.uses_sheng(), " want=", (int)want,
                  " got=", (int)got, "\n");
    }
  }

  io::print("cases=", REGEX_FUZZ_N, " sheng(SIMD)=", sheng_cases, " table_dfa=", table_cases, " pike=", pike_cases,
            " has_match_failures=", fails, "\n");
  require_true(fails == 0);

  print("=== REGEX DFA PASSED (Sheng + table has_match agree with glibc) ===");
  return 1;
}
