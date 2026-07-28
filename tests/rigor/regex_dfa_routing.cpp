// Copyright (c) 2024- David Lucius Severus
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include "../../src/regex.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mc = micron;
namespace io = micron::io;

static const char *
path_name(int p)
{
  switch ( p ) {
  case 4:
    return "accel-dfa";
  case 3:
    return "sheng-dfa";
  case 2:
    return "table-dfa";
  case 1:
    return "prefilter+pike";
  default:
    return "pike";
  }
}

static int
path_of(const char *pat)
{
  mc::regex re(pat);
  return re.valid() ? re.has_match_path() : -1;
}

static void
test_routing()
{
  print("=== has_match() engine selection ===");

  test_case("literals take the accelerated DFA");
  {
    const char *pats[] = { "needle", "z", "abcdefghijklmnop", "foobar", "a" };
    for ( const char *p : pats ) {
      int got = path_of(p);
      if ( got != 4 ) io::print("  pat=`", p, "` path=", got, " (", path_name(got), ") expected accel-dfa\n");
      require_true(got == 4);
    }
  }
  end_test_case();

  test_case("narrow classes and alternations take an accelerated DFA");
  {
    const char *pats[] = { "[a-z]+", "[0-9]{3}", "(foo|bar|baz)", "colou?r", "[0-9a-f]{8}-[0-9a-f]{4}" };
    for ( const char *p : pats ) {
      int got = path_of(p);
      if ( got != 4 ) io::print("  pat=`", p, "` path=", got, " (", path_name(got), ") expected accel-dfa\n");
      require_true(got == 4);
    }
  }
  end_test_case();

  test_case("patterns that already used the DFA still do");
  {
    const char *pats[] = { ".*foo", "a.c", "^abc", "[^0-9]x", "[^a]b" };
    for ( const char *p : pats ) {
      int got = path_of(p);
      if ( got < 2 ) io::print("  pat=`", p, "` path=", got, " (", path_name(got), ") regressed off the DFA\n");
      require_true(got >= 2);
    }
  }
  end_test_case();

  test_case("patterns with no DFA still fall back to Pike cleanly");
  {

    mc::regex re("a|^b");
    require_true(re.valid());
    require_true(!re.uses_dfa());
    require_true(re.has_match_path() < 2);
    require_true(re.has_match("ba"));
    require_true(re.has_match("xa"));
    require_true(!re.has_match("xb"));
  }
  end_test_case();
}

struct pat_case {
  const char *pat;
};

static const pat_case kPats[] = {
  { "needle" },
  { "z" },
  { "abcdefghijklmnop" },
  { "a" },
  { "aaa" },
  { "[a-z]+" },
  { "[0-9]{3}" },
  { "[a-z]*x" },
  { "[A-Za-z]+" },
  { "[[:digit:]]+" },
  { "(foo|bar|baz)" },
  { "colou?r" },
  { "a{3}" },
  { "ab*c" },
  { "a+b+" },
  { ".*foo" },
  { "a.c" },
  { "^abc" },
  { "abc$" },
  { "^a.*z$" },
  { "[^0-9]x" },
  { "[^a]b" },
  { "(a|b)*c" },
  { "x?y?z" },
  { "[0-9a-f]{4}" },
  { "(ab)+c" },
  { "a|bb|ccc" },
  { "[-+]?[0-9]+" },
  { "^$" },
  { "q" },
};

static const char *kInputs[] = {
  "",
  "a",
  "z",
  "needle",
  "haystack with a needle inside",
  "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnneedle",
  "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn",
  "abcdefghijklmnop",
  "zzzzabcdefghijklmnopzzzz",
  "0123456789",
  "abc",
  "ABC",
  "foo",
  "bar",
  "baz",
  "color",
  "colour",
  "aaabbbccc",
  "-42",
  "+7",
  "deadbeef",
  "the quick brown fox jumps over the lazy dog",
  "THE QUICK BROWN FOX",
  "x",
  "xyz",
  "a.c",
  "!!!???///",
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaz",
  "                                                                ",
  "mixed 123 CASE and-symbols_here!",
};

static void
test_dfa_vs_pike()
{
  print("=== DFA has_match() vs Pike search() ===");

  int checked = 0, shown = 0, fails = 0;
  int by_path[5] = { 0, 0, 0, 0, 0 };

  test_case("every pattern x input: has_match() agrees with search().matched");
  {
    for ( const pat_case &pc : kPats ) {
      mc::regex re(pc.pat);
      require_true(re.valid());
      int path = re.has_match_path();
      if ( path >= 0 && path <= 4 ) ++by_path[path];

      for ( const char *in : kInputs ) {
        usize n = mc::strlen(in);
        bool dfa_says = re.has_match_n(in, n);
        bool pike_says = re.search_n(in, n).matched;
        ++checked;
        if ( dfa_says != pike_says ) {
          ++fails;
          if ( shown++ < 20 )
            io::print("  MISMATCH pat=`", pc.pat, "` in=`", in, "` path=", path_name(path), " has_match=", (int)dfa_says,
                      " search=", (int)pike_says, "\n");
        }
      }
    }
    io::print("  pairs=", checked, " accel=", by_path[4], " sheng=", by_path[3], " table=", by_path[2], " prefilter+pike=", by_path[1],
              " pike=", by_path[0], " mismatches=", fails, "\n");
    require_true(fails == 0);
  }
  end_test_case();
}

static void
test_dense_first_byte()
{
  print("=== first-byte-dense input (the 32-attempt prefilter cliff) ===");

  test_case("literal miss in first-byte-dense input is correct on both engines");
  {

    mc::string hay;
    for ( int i = 0; i < 20000; ++i ) hay.push_back('n');

    mc::regex re("needle");
    require_true(re.has_match_path() == 4);
    require_true(!re.has_match_n(hay.c_str(), hay.size()));
    require_true(!re.search_n(hay.c_str(), hay.size()).matched);

    hay += "needle";
    require_true(re.has_match_n(hay.c_str(), hay.size()));
    require_true(re.search_n(hay.c_str(), hay.size()).matched);
  }
  end_test_case();

  test_case("match at the very start and the very end");
  {
    mc::regex re("needle");
    mc::string a("needle");
    for ( int i = 0; i < 5000; ++i ) a.push_back('n');
    require_true(re.has_match_n(a.c_str(), a.size()));

    mc::string b;
    for ( int i = 0; i < 5000; ++i ) b.push_back('n');
    b += "needle";
    require_true(re.has_match_n(b.c_str(), b.size()));
  }
  end_test_case();

  test_case("captures still come from Pike and are unaffected by the routing change");
  {
    mc::regex re("([a-z]+)-([0-9]+)");
    mc::rmatch m = re.match("id: widget-42 done");
    require_true(m.matched);
    require_true(m.groups() == 3);
    require_true(m.group(1).len == 6 && mc::strncmp(m.group(1).ptr, "widget", 6) == 0);
    require_true(m.group(2).len == 2 && mc::strncmp(m.group(2).ptr, "42", 2) == 0);
  }
  end_test_case();
}

int
main()
{
  print("=== REGEX DFA ROUTING ===");
  test_routing();
  test_dfa_vs_pike();
  test_dense_first_byte();
  print("=== REGEX DFA ROUTING PASSED ===");
  return 1;
}
