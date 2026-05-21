

#include "../../src/io/console.hpp"

#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

namespace
{

bool
streq(const char *a, const char *b)
{
  usize i = 0;
  for ( ; a[i] && b[i]; ++i )
    if ( a[i] != b[i] ) return false;
  return a[i] == b[i];
}
}      // namespace

int
main(int, char **)
{
  sb::print("=== STRING CROSS-CONCAT TESTS ===");

  test_case("sstring<N> += sstring<M>, dst bigger (N > M)");
  {
    micron::sstring<64> a("foo");
    micron::sstring<16> b("bar");
    a += b;
    require(a == micron::sstring<64>("foobar"), true);
    require(a.size(), 6u);
  }
  end_test_case();

  test_case("sstring<N> += sstring<M>, dst smaller (N < M), fits");
  {
    micron::sstring<16> a("foo");
    micron::sstring<64> b("bar");
    a += b;
    require(a == micron::sstring<16>("foobar"), true);
    require(a.size(), 6u);
  }
  end_test_case();

  test_case("sstring<N> += sstring<M> rvalue, differing sizes");
  {
    micron::sstring<32> a("hello ");
    a += micron::sstring<8>("world");
    require(a == micron::sstring<32>("hello world"), true);
    require(a.size(), 11u);
  }
  end_test_case();

  test_case("sstring cross-size += overflow throws");
  {
    require_throw([&] {
      micron::sstring<8> a("hello");
      micron::sstring<64> b("world!");
      a += b;
    });
  }
  end_test_case();

  test_case("sstring<N> copy-construct from sstring<M>, N < M clamps");
  {
    micron::sstring<64> big("abcdefghij");
    micron::sstring<4> small(big);
    require(small.size() <= 4u, true);
    require(small.capacity(), 4u);
  }
  end_test_case();

  test_case("sstring<N> move-construct from sstring<M>, N > M (was OOB on src)");
  {
    micron::sstring<8> src("hi");
    micron::sstring<64> dst(micron::move(src));
    require(dst == micron::sstring<64>("hi"), true);
    require(dst.size(), 2u);
  }
  end_test_case();

  test_case("sstring<N> move-construct from sstring<M>, N < M");
  {
    micron::sstring<64> src("abcdef");
    micron::sstring<8> dst(micron::move(src));
    require(dst == micron::sstring<8>("abcdef"), true);
    require(dst.size(), 6u);
  }
  end_test_case();

  test_case("sstring construct from longer heap string clamps to capacity");
  {
    micron::string s("abcdefghijklmnop");
    micron::sstring<8> dst(s);
    require(dst.size() <= 8u, true);
    require(dst.capacity(), 8u);
  }
  end_test_case();

  test_case("sstring operator= from longer heap string clamps length");
  {
    micron::string s("abcdefghijklmnop");
    micron::sstring<8> dst;
    dst = s;
    require(dst.size() <= 8u, true);
    require(dst.capacity(), 8u);
  }
  end_test_case();

  test_case("string += string");
  {
    micron::string a("foo");
    micron::string b("bar");
    a += b;
    require(streq(a.c_str(), "foobar"), true);
    require(a.size(), 6u);
  }
  end_test_case();

  test_case("string += sstring<N>");
  {
    micron::string a("foo");
    micron::sstring<16> b("bar");
    a += b;
    require(streq(a.c_str(), "foobar"), true);
    require(a.size(), 6u);
  }
  end_test_case();

  test_case("string += sstring keeps embedded NUL (length, not strlen)");
  {
    micron::sstring<8> b;
    b.push_back('a');
    b.push_back('\0');
    b.push_back('b');
    require(b.size(), 3u);
    micron::string a("x");
    a += b;
    require(a.size(), 4u);
    require(a[1], 'a');
    require(a[2], '\0');
    require(a[3], 'b');
  }
  end_test_case();

  test_case("string += istring (free cross-class operator)");
  {
    micron::string a("foo");
    micron::istring<> b("bar");
    a += b;
    require(streq(a.c_str(), "foobar"), true);
    require(a.size(), 6u);
  }
  end_test_case();

  test_case("string += rope (free cross-class operator)");
  {
    micron::string a("foo");
    micron::rope<> r("bar");
    a += r;
    require(streq(a.c_str(), "foobar"), true);
    require(a.size(), 6u);
  }
  end_test_case();

  test_case("istring += const char[] does not drop last char of lhs");
  {
    micron::istring<> a("foo");
    auto r = a += "bar";
    require(streq(r.c_str(), "foobar"), true);
    require(r.size(), 6u);
  }
  end_test_case();

  test_case("istring append(istring)");
  {
    micron::istring<> a("foo");
    micron::istring<> b("bar");
    auto r = a.append(b);
    require(streq(r.c_str(), "foobar"), true);
    require(r.size(), 6u);
  }
  end_test_case();

  test_case("rope += rope");
  {
    micron::rope<> a("foo");
    micron::rope<> b("bar");
    a += b;
    require(streq(a.c_str(), "foobar"), true);
    require(a.size(), 6u);
  }
  end_test_case();

  test_case("rope += sstring (is_string overload)");
  {
    micron::rope<> a("foo");
    micron::sstring<16> b("bar");
    a += b;
    require(streq(a.c_str(), "foobar"), true);
    require(a.size(), 6u);
  }
  end_test_case();

  test_case("operator+(sstr&, sstr&) different sizes -> heap string");
  {
    micron::sstr<8> a("foo");
    micron::sstr<16> b("bar");
    micron::string c = a + b;
    require(streq(c.c_str(), "foobar"), true);
  }
  end_test_case();

  test_case("operator+(sstr&, sstr&&) mixed value categories");
  {
    micron::sstr<8> a("foo");
    micron::string c = a + micron::sstr<16>("bar");
    require(streq(c.c_str(), "foobar"), true);
  }
  end_test_case();

  sb::print("=== ALL STRING CROSS-CONCAT TESTS PASSED ===");
  return 1;
}
