// regex_fuzz.cpp
// Randomized differential test: micron::regex vs glibc POSIX regexec over the
// generated corpus (regex_fuzz.hpp from regex_fuzz_gen.c). Group-0 (whole
// match) must agree with glibc on every case (hard failure otherwise);
// sub-group capture diffs are reported as INFO.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex.hpp"

#include "../snowball/snowball.hpp"

#include "regex_corpus.hpp"      // for the regex_case typedef
#include "regex_fuzz.hpp"

using sb::print;
using sb::require_true;

namespace mc = micron;
namespace io = micron::io;

#define NG 10

int
main()
{
  print("=== REGEX FUZZ DIFFERENTIAL (vs glibc POSIX regexec) ===");

  int g0_fail = 0, sub_diff = 0, shown = 0, glibc_slow = 0;

  for ( int i = 0; i < REGEX_FUZZ_N; ++i ) {
    const long *E = REGEX_FUZZ_EXPECTED[i];
    const char *pat = REGEX_FUZZ[i].pat;
    const char *in = REGEX_FUZZ[i].in;

    // micron runs on EVERY case (incl. those where glibc went exponential) --
    // a hang here would be a micron bug. The Pike VM is linear, so it won't.
    mc::regex re(pat);
    mc::rmatch m = re.match(in);

    if ( E[0] == -2 ) {      // glibc had no ground truth (timed out) -- can't compare
      ++glibc_slow;
      continue;
    }

    int gm = m.has_match() ? 1 : 0;
    long g0s = gm ? (long)m.group_start(0) : -1;
    long g0e = gm ? (long)m.group_end(0) : -1;

    if ( gm != (int)E[0] || g0s != E[1] || g0e != E[2] ) {
      ++g0_fail;
      if ( shown++ < 40 )
        io::print("  G0 FAIL [", i, "] pat=`", pat, "` in=`", in, "`  exp(m=", E[0], " [", E[1], ",", E[2], "])  got(m=", gm, " [", g0s,
                  ",", g0e, "])\n");
      continue;
    }
    if ( !gm ) continue;

    for ( int g = 1; g < NG; ++g ) {
      long s = (long)m.group_start((usize)g), e = (long)m.group_end((usize)g);
      if ( s != E[1 + 2 * g] || e != E[2 + 2 * g] ) {
        ++sub_diff;
        if ( sub_diff <= 25 )
          io::print("  ~ subgrp ", g, " [", i, "] pat=`", pat, "` in=`", in, "` exp[", E[1 + 2 * g], ",", E[2 + 2 * g], "] got[", s, ",", e,
                    "]\n");
        break;
      }
    }
  }

  io::print("fuzz cases=", REGEX_FUZZ_N, " glibc_timed_out(micron still ran)=", glibc_slow, " group0_failures=", g0_fail,
            " subgroup_diffs=", sub_diff, "\n");
  require_true(g0_fail == 0);

  print("=== REGEX FUZZ PASSED (group-0 matches glibc on all cases) ===");
  return 1;
}
