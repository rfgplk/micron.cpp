// regex_string_input.cpp
// Verifies the engine accepts const char* AND any has_cstr micron string object
// (micron::string, micron::sstr<N>) for both the pattern and the subject.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mc = micron;

int
main()
{
  print("=== REGEX STRING-INPUT TESTS ===");

  test_case("subject as micron::string");
  {
    mc::regex re("[0-9]+");
    mc::string subject("abc12345xyz");
    mc::rmatch m = re.match(subject);
    require_true(m.has_match());
    require_true(m.group(0).len == 5);
    require_true(m.group(0).ptr[0] == '1');
  }
  end_test_case();

  test_case("subject as sstr<N>");
  {
    mc::regex re("(a+)(b+)");
    mc::sstr<32> subject("xaaabbbz");
    mc::rmatch m = re.match(subject);
    require_true(m.has_match());
    require_true(m.group(1).len == 3 && m.group(2).len == 3);
  }
  end_test_case();

  test_case("pattern as micron::string");
  {
    mc::string pat("colou?r");
    mc::regex re(pat);
    require_true(re.valid());
    require_true(re.has_match("color"));
    require_true(re.has_match("colour"));
    require_true(!re.has_match("colr"));
  }
  end_test_case();

  test_case("search_n explicit length + embedded NUL");
  {
    mc::regex re("b+");
    // buffer of exactly 7 bytes with an embedded NUL: a b b \0 b b a
    char data[7] = { 'a', 'b', 'b', '\0', 'b', 'b', 'a' };
    mc::rmatch m = re.search_n(data, 7);
    require_true(m.has_match());
    require_true(m.group(0).ptr == data + 1 && m.group(0).len == 2);      // leftmost "bb" before the NUL

    // matching past the NUL too (length-aware, not strlen-truncated)
    mc::regex re2("a$");
    require_true(re2.has_match_n(data, 7));      // trailing 'a' at offset 6
    require_true(!re2.has_match(data));          // strlen view stops at the NUL (offset 3)
  }
  end_test_case();

  print("=== ALL REGEX STRING-INPUT TESTS PASSED ===");
  return 1;
}
