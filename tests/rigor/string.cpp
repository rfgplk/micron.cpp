#include "../snowball/snowball.hpp"

#include "../../src/string/strings.hpp"

using namespace snowball;
int
main(int, char **)
{
  // Test construction
  test_case("Default construction");
  {
    micron::string s;
    require(s.empty(), true);
    require(s.size(), 0u);
    require(s.max_size() >= 0u);
  }
  end_test_case();

  test_case("Construction from const char*");
  {
    micron::string s("hello");
    require(s.size(), 5u);
    require(s[0], 'h');
    require(s[4], 'o');
  }
  end_test_case();

  test_case("Construction from micron::string");
  {
    micron::string tmp = "world";
    micron::string s(tmp);
    require(s.size(), tmp.size());
    require(s[0], 'w');
  }
  end_test_case();

  test_case("Copy construction");
  {
    micron::string s1("copyme");
    micron::string s2(s1);
    require(s1 == s2, true);
    require(s1.size(), s2.size());
  }
  end_test_case();

  test_case("Move construction");
  {
    micron::string s1("moveme");
    micron::string s2(micron::move(s1));
    require(s2.size(), 6u);
  }
  end_test_case();

  test_case("Copy assignment");
  {
    micron::string s1("assign");
    micron::string s2;
    s2 = s1;
    require(s2 == s1, true);
  }
  end_test_case();

  test_case("Move assignment");
  {
    micron::string s1("moveassign");
    micron::string s2;
    s2 = micron::move(s1);
    require(s2.size(), 10u);
  }
  end_test_case();

  test_case("Element access & bounds");
  {
    micron::string s("abcdef");
    require(s.at(0), 'a');
    require(s[5], 'f');
    require_throw([&] { s.at(6); });
  }
  end_test_case();

  test_case("Front & back");
  {
    micron::string s("xyz");
    require(s.front(), 'x');
    require(s.back(), 'z');
  }
  end_test_case();

  test_case("Data pointer access");
  {
    micron::string s("data");
    const char *ptr = s.data();
    require(ptr[0], 'd');
  }
  end_test_case();

  test_case("micron::stringize, empty, max_size");
  {
    micron::string s;
    require(s.empty(), true);
    s = "hello";
    require(s.size(), 5u);
    require(s.empty(), false);
    require_greater(s.max_size(), 0u);
  }
  end_test_case();

  test_case("Clear");
  {
    micron::string s("clearme");
    s.clear();
    require(s.empty(), true);
    require(s.size(), 0u);
  }
  end_test_case();

  test_case("Push back & pop back");
  {
    micron::string s;
    s.push_back('a');
    s.push_back('b');
    require(s.size(), 2u);
    require(s.back(), 'b');
  }
  end_test_case();

  test_case("Resize");
  {
    micron::string s("resize");
    s.resize(10, 'x');
    require(s.size(), 10u);
    require(s[7], 'x');
  }
  end_test_case();

  /*
  test_case("micron::stringwap");
  {
    micron::string s1("abc");
    micron::string s2("xyz");
    s1.swap(s2);
    require(s1 == micron::string("xyz"), true);
    require(s2 == micron::string("abc"), true);
  }
  end_test_case();
*/
  test_case("Append & operator+=");
  {
    micron::string s("foo");
    s += "bar";
    require(s == micron::string("foobar"), true);
    s.append("baz");
    require(s == micron::string("foobarbaz"), true);
  }
  end_test_case();

  test_case("Insert & erase");
  {
    micron::string s("abcd");
    s.insert(2, "XX");
    require(s == micron::string("abXXcd"), true);
    s.erase(2, 2);
    require(s == micron::string("abcd"), true);
  }
  end_test_case();

  /*test_case("Replace");
  {
    micron::string s("hello");
    s.replace(1, 3, "i");
    require(s == micron::string("ho"), true);
  }
  end_test_case();

  test_case("Find & rfind");
  {
    micron::string s("abracadabra");
    require(s.find("ra"), 2u);
    require(s.find("zz"), micron::npos);
  }
  end_test_case();
*/
  test_case("Comparison operators");
  {
    micron::string s1("aaa");
    micron::string s2("aaa");
    micron::string s3("bbb");
    require(s1 == s2, true);
    require(s1 != s3, true);
    require(s1 < s3, true);
    require(s3 > s2, true);
    //require(s1 <= s2, true);
    //require(s3 >= s2, true);
  }
  end_test_case();

  test_case("micron::stringubstr");
  {
    micron::string s("substring");
    require(s.substr(3, 3), micron::string("str"));
    require_throw([&] { s.substr(10); });
  }
  end_test_case();

  test_case("Exception safety for at()");
  {
    micron::string s("abc");
    require_throw([&] { s.at(100); });
  }
  end_test_case();
  return 1;
}
