#include "../../src/io/console.hpp"

#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"

using namespace snowball;

template<typename F, typename G>
concept can_append_istring = requires(micron::hstring<F> a, micron::istring<G> b) { a += b; };
template<typename F, typename G>
concept can_append_rope = requires(micron::hstring<F> a, micron::rope<G> b) { a += b; };

int
main(int, char **)
{
  sb::print("=== STRINGS_OPS ADV ===");

  test_case("equal-width hstring += istring concatenates, c_str NUL-terminated");
  {
    micron::string a("foo");
    micron::istring<char> b("barbaz");
    a += b;
    require(a == "foobarbaz", true);
    require(micron::strlen(a.c_str()) == 9u, true);

    micron::istring<char> e("");
    a += e;
    require(a == "foobarbaz", true);
    require(micron::strlen(a.c_str()) == 9u, true);
  }
  end_test_case();

  test_case("equal-width hstring += rope concatenates, c_str NUL-terminated");
  {
    micron::string a("foo");
    micron::rope<char> r("quux");
    a += r;
    require(a == "fooquux", true);
    require(micron::strlen(a.c_str()) == 7u, true);
  }
  end_test_case();

  test_case("equal-width wide hstring += istring (unicode32)");
  {
    micron::hstring<unicode32> a(U"ab");
    micron::istring<unicode32> b(U"cd");
    a += b;
    require(a.size() == 4u, true);
    require(a[3] == static_cast<unicode32>(U'd'), true);
  }
  end_test_case();

  static_assert(can_append_istring<char, char>, "equal-width istring += must remain well-formed");
  static_assert(can_append_istring<unicode32, unicode32>, "equal-width wide istring += must remain well-formed");
  static_assert(!can_append_istring<char, unicode32>, "cross-width istring += must be ill-formed (over-read path)");
  static_assert(!can_append_istring<unicode32, char>, "cross-width istring += must be ill-formed (truncation path)");
  static_assert(can_append_rope<char, char>, "equal-width rope += must remain well-formed");
  static_assert(!can_append_rope<char, unicode32>, "cross-width rope += must be ill-formed (over-read path)");
  static_assert(!can_append_rope<unicode32, char>, "cross-width rope += must be ill-formed (truncation path)");

  sb::print("ALL STRINGS_OPS ADV PASSED");
  return 1;
}
