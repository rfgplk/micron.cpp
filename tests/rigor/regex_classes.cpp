// regex_classes.cpp
// Exhaustive character-class rigor. For every class form, all 256 byte values
// are checked against a hand reference AND cross-checked between the two
// independent engine paths: has_match (Sheng SIMD DFA) and match (truffle SIMD
// prefilter + Pike VM). Covers POSIX [:classes:], ranges, negation, bracket
// edge cases, escapes, and high bytes.
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

// --- ASCII/POSIX reference predicates (match src/regex/ast.hpp posix_class) --
static bool
r_digit(int b)
{
  return b >= '0' && b <= '9';
}

static bool
r_upper(int b)
{
  return b >= 'A' && b <= 'Z';
}

static bool
r_lower(int b)
{
  return b >= 'a' && b <= 'z';
}

static bool
r_alpha(int b)
{
  return r_upper(b) || r_lower(b);
}

static bool
r_alnum(int b)
{
  return r_alpha(b) || r_digit(b);
}

static bool
r_space(int b)
{
  return b == ' ' || b == '\t' || b == '\n' || b == '\r' || b == '\v' || b == '\f';
}

static bool
r_blank(int b)
{
  return b == ' ' || b == '\t';
}

static bool
r_xdigit(int b)
{
  return r_digit(b) || (b >= 'A' && b <= 'F') || (b >= 'a' && b <= 'f');
}

static bool
r_cntrl(int b)
{
  return (b >= 0 && b <= 31) || b == 127;
}

static bool
r_print(int b)
{
  return b >= 32 && b <= 126;
}

static bool
r_graph(int b)
{
  return b >= 33 && b <= 126;
}

static bool
r_punct(int b)
{
  return (b >= 33 && b <= 47) || (b >= 58 && b <= 64) || (b >= 91 && b <= 96) || (b >= 123 && b <= 126);
}

// custom predicates for the literal class tests
static bool
r_az(int b)
{
  return b >= 'a' && b <= 'z';
}

static bool
r_az09_(int b)
{
  return r_az(b) || r_digit(b) || b == '_';
}

static bool
r_hex(int b)
{
  return r_xdigit(b);
}

static bool
r_not_digit(int b)
{
  return !r_digit(b);
}

static bool
r_not_az(int b)
{
  return !r_az(b);
}

static bool
r_bracket_rsqb_a(int b)
{
  return b == ']' || b == 'a';
}      // []a]

static bool
r_dash_a(int b)
{
  return b == '-' || b == 'a';
}      // [-a]  and  [a-]

static bool
r_a_c_dash_e(int b)
{
  return (b >= 'a' && b <= 'c') || b == '-' || b == 'e';
}      // [a-c-e]

static bool
r_x09y(int b)
{
  return b == 'x' || b == 'y' || r_digit(b);
}      // [x[:digit:]y]

static bool
r_high(int b)
{
  return b >= 0x80;
}      // [\x80-\xff] approximated below with a literal-range test

// verify a class regex byte-for-byte through BOTH engine paths
static int
verify(const char *pat, bool (*pred)(int))
{
  mc::regex re(pat);
  if ( !re.valid() ) {
    io::print("  INVALID pattern `", pat, "`\n");
    return 1;
  }
  int fails = 0;
  for ( int b = 0; b < 256; ++b ) {
    char c = (char)(unsigned char)b;
    bool want = pred(b);
    bool sheng = re.has_match_n(&c, 1);
    bool pike = re.match_n(&c, 1).has_match();
    if ( sheng != want || pike != want ) {
      if ( fails < 6 ) io::print("  [`", pat, "`] byte ", b, " want=", (int)want, " has_match=", (int)sheng, " match=", (int)pike, "\n");
      ++fails;
    }
  }
  return fails;
}

int
main()
{
  print("=== REGEX CLASSES (exhaustive per-byte, both engine paths) ===");
  int F = 0;

  test_case("POSIX [:classes:]");
  {
    F += verify("[[:digit:]]", r_digit);
    F += verify("[[:alpha:]]", r_alpha);
    F += verify("[[:alnum:]]", r_alnum);
    F += verify("[[:upper:]]", r_upper);
    F += verify("[[:lower:]]", r_lower);
    F += verify("[[:space:]]", r_space);
    F += verify("[[:blank:]]", r_blank);
    F += verify("[[:xdigit:]]", r_xdigit);
    F += verify("[[:cntrl:]]", r_cntrl);
    F += verify("[[:print:]]", r_print);
    F += verify("[[:graph:]]", r_graph);
    F += verify("[[:punct:]]", r_punct);
    require_true(F == 0);
  }
  end_test_case();

  test_case("ranges + literal sets");
  {
    F += verify("[a-z]", r_az);
    F += verify("[a-z0-9_]", r_az09_);
    F += verify("[a-fA-F0-9]", r_hex);
    require_true(F == 0);
  }
  end_test_case();

  test_case("negation (complement over all 256 incl. NUL / high bytes)");
  {
    F += verify("[^0-9]", r_not_digit);
    F += verify("[^a-z]", r_not_az);
    require_true(F == 0);
  }
  end_test_case();

  test_case("bracket edge cases");
  {
    F += verify("[]a]", r_bracket_rsqb_a);      // ']' literal when first
    F += verify("[-a]", r_dash_a);              // '-' literal when first
    F += verify("[a-]", r_dash_a);              // '-' literal when last
    F += verify("[a-c-e]", r_a_c_dash_e);       // range then literal '-' then 'e'
    F += verify("[x[:digit:]y]", r_x09y);       // POSIX class mixed with literals
    require_true(F == 0);
  }
  end_test_case();

  test_case("high bytes (truffle bit-7 path) via explicit range");
  {
    // [\x80-\xff] in source: bytes 0x80..0xff
    F += verify("[\x80-\xff]", r_high);
    require_true(F == 0);
  }
  end_test_case();

  test_case("invalid patterns rejected (valid()==false)");
  {
    require_true(!mc::regex("[z-a]").valid());      // reversed range
    require_true(!mc::regex("[").valid());          // unterminated class
    require_true(!mc::regex("a(b").valid());        // unbalanced group
    require_true(!mc::regex("a)b").valid());        // stray close
  }
  end_test_case();

  // constexpr proofs of class behaviour
  static_assert(mc::cmatch<"[[:digit:]]+">("ab123cd").group(0).size() == 3);
  static_assert(mc::cmatch<"[^a-z]+">("ABC123").group(0).size() == 6);
  static_assert(!mc::cmatch<"[[:upper:]]">("abc").has_match());
  static_assert(mc::cmatch<"[a-c-e]+">("abce-").group(0).size() == 5);

  print("=== ALL REGEX CLASSES TESTS PASSED ===");
  return 1;
}
