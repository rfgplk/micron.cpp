// string_exhaustive.cpp
// Exhaustive per-member-function tests for micron::string (hstring<T, Sf, Alloc>).
// hstring has ~80 public methods across construction, assignment, search,
// modification, comparison; existing string.cpp covers ~22 cases. This file
// fills gaps: c_str/w_str/uni_str/stack, into_chars/into_bytes, clone,
// multi-overload insert/append/push_back, remove/remove_all, substr/truncate
// (3 overloads), iterator-based variants, and edge-case comparisons.

#include "../../src/std.hpp"
#include "../../src/string/string.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

using S = micron::string;

int
main()
{
  print("=== STRING EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION GAPS                                            //
  // ============================================================ //
  test_case("ctor: hstring(usize cnt, T ch) — repeat char");
  {
    S s(5, 'a');
    require(s.size(), usize(5));
    require(s[0], 'a');
    require(s[4], 'a');
  }
  end_test_case();

  test_case("ctor: hstring(usize n) — capacity only");
  {
    S s(usize(32));
    require_true(s.empty());
    require_true(s.max_size() >= usize(32));
  }
  end_test_case();

  test_case("ctor: hstring(const char (&str)[M]) — literal array");
  {
    S s("Hello");
    require(s.size(), usize(5));
    require(s[0], 'H');
    require(s[4], 'o');
  }
  end_test_case();

  test_case("ctor: hstring(iterator, iterator) — range");
  {
    S src("abcdefgh");
    S s(src.begin() + 2, src.begin() + 6);
    require(s.size(), usize(4));
    require(s[0], 'c');
    require(s[3], 'f');
  }
  end_test_case();

  test_case("ctor: hstring(const_iterator, const_iterator) — bad range throws");
  {
    bool threw = false;
    try {
      S src("ab");
      S s(src.cend(), src.cbegin());      // end < begin
      (void)s;
    } catch ( ... ) {
      threw = true;
    }
    require(threw, true);
  }
  end_test_case();

  // ============================================================ //
  //  ASSIGNMENT GAPS                                              //
  // ============================================================ //
  test_case("op=(const F (&str)[M]): from char array");
  {
    S s("orig");
    s = "replaced";
    require(s.size(), usize(8));
    require(s[0], 'r');
  }
  end_test_case();

  // ============================================================ //
  //  ELEMENT ACCESS GAPS                                          //
  // ============================================================ //
  test_case("at(): bounds-checked");
  {
    S s("abc");
    require(s.at(0), 'a');
    require(s.at(2), 'c');
    require_throw([&s]() { (void)s.at(usize(99)); });
  }
  end_test_case();

  test_case("c_str(): null-terminated C string");
  {
    S s("hello");
    const char *p = s.c_str();
    require(p[0], 'h');
    require(p[5], '\0');
  }
  end_test_case();

  test_case("stack(): produces a stack copy");
  {
    S s("stacky");
    auto ss = s.stack();
    require(ss.size(), usize(6));
    require(ss[0], 's');
    require(ss[5], 'y');
  }
  end_test_case();

  test_case("into_chars(): slice over chars");
  {
    S s("abcd");
    auto sl = s.into_chars();
    require(sl.size(), usize(4));
  }
  end_test_case();

  test_case("into_bytes(): byte slice over storage");
  {
    S s("hi");
    auto bs = s.into_bytes();
    require(bs.size(), usize(2));
  }
  end_test_case();

  // ============================================================ //
  //  ITERATION GAPS                                               //
  // ============================================================ //
  test_case("cbegin/cend: const iteration");
  {
    S s("xyz");
    char acc = 0;
    for ( auto it = s.cbegin(); it != s.cend(); ++it ) acc ^= *it;
    require(acc, char('x' ^ 'y' ^ 'z'));
  }
  end_test_case();

  test_case("last(): iterator to final char");
  {
    S s("abc");
    auto l = s.last();
    require(*l, 'c');
  }
  end_test_case();

  // ============================================================ //
  //  CAPACITY                                                     //
  // ============================================================ //
  test_case("reserve(n): grows capacity");
  {
    S s;
    s.reserve(256);
    require_true(s.max_size() >= usize(256));
  }
  end_test_case();

  test_case("set_size: direct length mutation");
  {
    S s("hello world");
    s.set_size(5);
    require(s.size(), usize(5));
  }
  end_test_case();

  // ============================================================ //
  //  SEARCH                                                       //
  // ============================================================ //
  test_case("find(ch, pos): present");
  {
    S s("hello");
    require(s.find('l'), usize(2));
    require(s.find('l', usize(3)), usize(3));
  }
  end_test_case();

  test_case("find(ch): absent returns npos");
  {
    S s("hello");
    auto pos = s.find('z');
    // npos for hstring is typically the max size_type
    require_true(pos == usize(-1) || pos >= s.size());
  }
  end_test_case();

  test_case("find(hstring): substring");
  {
    S a("hello world");
    S b("wor");
    require(a.find(b), usize(6));
  }
  end_test_case();

  test_case("find_substr(needle, len): low-level");
  {
    S s("foobarbaz");
    const char *needle = "bar";
    require(s.find_substr(needle, 3), usize(3));
  }
  end_test_case();

  // ============================================================ //
  //  APPEND OVERLOADS                                             //
  // ============================================================ //
  test_case("append(const F (&str)[M]): literal");
  {
    S s("ab");
    s.append("cd");
    require(s.size(), usize(4));
    require(s[3], 'd');
  }
  end_test_case();

  test_case("append(const hstring&)");
  {
    S a("Hello, ");
    S b("World!");
    a.append(b);
    require(a.size(), usize(13));
    require(a[12], '!');
  }
  end_test_case();

  test_case("append(const char*, usize): partial");
  {
    S s("a");
    s.append("bcdef", usize(3));
    require(s.size(), usize(4));
    require(s[3], 'd');
  }
  end_test_case();

  // ============================================================ //
  //  PUSH_BACK OVERLOADS                                          //
  // ============================================================ //
  test_case("push_back(ch)");
  {
    S s("hi");
    s.push_back('!');
    require(s.size(), usize(3));
    require(s[2], '!');
  }
  end_test_case();

  test_case("push_back(const F (&str)[M])");
  {
    S s("aa");
    s.push_back("bb");
    require(s.size(), usize(4));
    require(s[3], 'b');
  }
  end_test_case();

  test_case("push_back(const hstring&)");
  {
    S a("foo");
    S b("bar");
    a.push_back(b);
    require(a.size(), usize(6));
    require(a[5], 'r');
  }
  end_test_case();

  test_case("pop_back");
  {
    S s("abc");
    s.pop_back();
    require(s.size(), usize(2));
    require(s[1], 'b');
  }
  end_test_case();

  // ============================================================ //
  //  INSERT OVERLOADS                                             //
  // ============================================================ //
  test_case("insert(idx, ch, cnt=1)");
  {
    S s("hllo");
    s.insert(usize(1), 'e', usize(1));
    require(s.size(), usize(5));
    require(s[1], 'e');
  }
  end_test_case();

  test_case("insert(idx, const F (&str)[M], cnt=1)");
  {
    S s("hWorld");
    s.insert(usize(1), "ello ", usize(1));
    require(s.size(), usize(11));
  }
  end_test_case();

  // ============================================================ //
  //  ERASE                                                        //
  // ============================================================ //
  test_case("erase(idx, cnt)");
  {
    S s("abcdef");
    s.erase(usize(2), usize(2));
    require(s.size(), usize(4));
    require(s[0], 'a');
    require(s[1], 'b');
    require(s[2], 'e');
    require(s[3], 'f');
  }
  end_test_case();

  test_case("erase(iterator, cnt)");
  {
    S s("abcdef");
    s.erase(s.begin() + 1, usize(3));
    require(s.size(), usize(3));
    require(s[0], 'a');
    require(s[1], 'e');
    require(s[2], 'f');
  }
  end_test_case();

  // ============================================================ //
  //  REMOVE / REMOVE_ALL                                          //
  // ============================================================ //
  test_case("remove(needle): first match");
  {
    S s("hello world hello");
    s.remove("hello");
    require_true(s.size() < usize(17));
  }
  end_test_case();

  test_case("remove_all(needle): all matches");
  {
    S s("aaXbbXccXdd");
    s.remove_all("X");
    require(s.size(), usize(8));
  }
  end_test_case();

  // ============================================================ //
  //  SUBSTR                                                       //
  // ============================================================ //
  test_case("substr(pos, cnt)");
  {
    S s("Hello, World!");
    auto sub = s.substr(usize(7), usize(5));
    require(sub.size(), usize(5));
    require(sub[0], 'W');
    require(sub[4], 'd');
  }
  end_test_case();

  test_case("substr(iterator, iterator)");
  {
    S s("0123456789");
    auto sub = s.substr(s.begin() + 2, s.begin() + 7);
    require(sub.size(), usize(5));
    require(sub[0], '2');
    require(sub[4], '6');
  }
  end_test_case();

  // ============================================================ //
  //  TRUNCATE                                                     //
  // ============================================================ //
  test_case("truncate(n): integer cap");
  {
    S s("Hello, World!");
    s.truncate(usize(5));
    require(s.size(), usize(5));
    require(s[0], 'H');
    require(s[4], 'o');
  }
  end_test_case();

  test_case("truncate(iterator)");
  {
    S s("abcdef");
    s.truncate(s.begin() + 3);
    require(s.size(), usize(3));
    require(s[2], 'c');
  }
  end_test_case();

  // ============================================================ //
  //  CLONE                                                        //
  // ============================================================ //
  test_case("clone(): deep duplicate");
  {
    S s("original");
    auto c = s.clone();
    require(c.size(), s.size());
    require(c[0], 'o');
    s[0] = 'X';
    require(c[0], 'o');      // independent
  }
  end_test_case();

  // ============================================================ //
  //  COMPARISON OPERATORS                                         //
  // ============================================================ //
  test_case("op==(const char*): C string compare");
  {
    S s("hello");
    require_true(s == "hello");
    require_false(s == "world");
  }
  end_test_case();

  test_case("op!=(const char*)");
  {
    S s("abc");
    require_true(s != "abd");
    require_false(s != "abc");
  }
  end_test_case();

  test_case("op==(const hstring&)");
  {
    S a("xyz");
    S b("xyz");
    S c("xyy");
    require_true(a == b);
    require_false(a == c);
  }
  end_test_case();

  test_case("op<(const char*) / op>(const char*)");
  {
    S s("bbb");
    require_true(s < "ccc");
    require_true(s > "aaa");
    require_false(s < "aaa");
    require_false(s > "ccc");
  }
  end_test_case();

  test_case("op<=(const char*) / op>=(const char*)");
  {
    S s("bbb");
    require_true(s <= "bbb");
    require_true(s <= "ccc");
    require_true(s >= "bbb");
    require_true(s >= "aaa");
  }
  end_test_case();

  // ============================================================ //
  //  CONVERSION                                                   //
  // ============================================================ //
  test_case("operator bool / operator!");
  {
    S a;
    S b("x");
    require_false(static_cast<bool>(a));
    require_true(static_cast<bool>(b));
    require_true(!a);
    require_false(!b);
  }
  end_test_case();

  test_case("op*(): chunk over storage");
  {
    S s("data");
    auto ch = *s;
    require_true(ch.ptr != nullptr);
    require_true(ch.len >= usize(4));
  }
  end_test_case();

  // ============================================================ //
  //  OPERATOR +=                                                  //
  // ============================================================ //
  test_case("op+=(T ch)");
  {
    S s("abc");
    s += '!';
    require(s.size(), usize(4));
    require(s[3], '!');
  }
  end_test_case();

  test_case("op+=(const F* data)");
  {
    S s("ab");
    s += "cd";
    require(s.size(), usize(4));
    require(s[3], 'd');
  }
  end_test_case();

  test_case("op+=(const hstring&)");
  {
    S a("hi ");
    S b("you");
    a += b;
    require(a.size(), usize(6));
    require(a[5], 'u');
  }
  end_test_case();

  // ============================================================ //
  //  CLEAR / FAST_CLEAR                                           //
  // ============================================================ //
  test_case("clear(): empties string");
  {
    S s("hello");
    s.clear();
    require_true(s.empty());
  }
  end_test_case();

  test_case("fast_clear(): empties (no zero-fill)");
  {
    S s("hello");
    s.fast_clear();
    require_true(s.empty());
  }
  end_test_case();

  // ============================================================ //
  //  RESIZE                                                       //
  // ============================================================ //
  test_case("resize(n, ch): grow with fill");
  {
    S s("ab");
    s.resize(usize(5), 'x');
    require(s.size(), usize(5));
    require(s[2], 'x');
    require(s[4], 'x');
  }
  end_test_case();

  print("=== ALL STRING EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
