#include "../../src/io/console.hpp"

#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

int
main(int, char **)
{
  sb::print("=== SSTRING TESTS ===");

  // -----------------------------------------------------------------------
  // Construction
  // -----------------------------------------------------------------------

  test_case("Default construction");
  {
    micron::sstring<64> s;
    require(s.empty(), true);
    require(s.size(), 0u);
    require(s.capacity(), 64u);
    require(s.max_size(), 64u);
    // internal buffer must be all zeroes
    for ( size_t i = 0; i < 64; ++i )
      require(s[i], '\0');
  }
  end_test_case();

  test_case("Construction from const char*");
  {
    micron::sstring<64> s("hello");
    require(s.size(), 5u);
    require(s[0], 'h');
    require(s[4], 'o');
    require(s[5], '\0');     // null term present
  }
  end_test_case();

  test_case("Construction from string literal (template array)");
  {
    micron::sstring<64> s("world");
    require(s.size(), 5u);
    require(s[0], 'w');
    require(s[4], 'd');
  }
  end_test_case();

  test_case("Construction from char* (non-const)");
  {
    char buf[] = "mutable";
    micron::sstring<64> s(buf);
    require(s.size(), 7u);
    require(s[0], 'm');
  }
  end_test_case();

  test_case("Construction from iterator range");
  {
    micron::sstring<64> src("hello world");
    micron::sstring<64> s(src.begin(), src.begin() + 5);
    require(s.size(), 5u);
    require(s == micron::sstring<64>("hello"), true);
  }
  end_test_case();

  test_case("Construction from const_iterator range");
  {
    micron::sstring<64> src("hello world");
    micron::sstring<64> s(src.cbegin() + 6, src.cend());
    require(s.size(), 5u);
    require(s == micron::sstring<64>("world"), true);
  }
  end_test_case();

  test_case("Copy construction — same size");
  {
    micron::sstring<64> s1("copytest");
    micron::sstring<64> s2(s1);
    require(s1 == s2, true);
    require(s2.size(), s1.size());
  }
  end_test_case();

  test_case("Copy construction — larger target");
  {
    micron::sstring<32> s1("small");
    micron::sstring<64> s2(s1);
    require(s2.size(), 5u);
    require(s2 == micron::sstring<64>("small"), true);
  }
  end_test_case();

  test_case("Copy construction — smaller target (truncates)");
  {
    micron::sstring<64> s1("toolongstring");
    micron::sstring<8> s2(s1);
    // length must be clamped to 8
    require(s2.size() <= 8u, true);
  }
  end_test_case();

  test_case("Move construction");
  {
    micron::sstring<64> s1("moveme");
    micron::sstring<64> s2(micron::move(s1));
    require(s2.size(), 6u);
    require(s2[0], 'm');
    require(s1.size(), 0u);     // source cleared
  }
  end_test_case();

  test_case("Construction from const char* too large throws");
  {
    require_throw([&] { micron::sstring<4> s("toolong"); });
  }
  end_test_case();

  test_case("Construction from char* too large throws");
  {
    require_throw([&] {
      char buf[] = "toolongstring";
      micron::sstring<4> s(buf);
    });
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Assignment
  // -----------------------------------------------------------------------

  test_case("Copy assignment — same size");
  {
    micron::sstring<64> s1("assign");
    micron::sstring<64> s2;
    s2 = s1;
    require(s2 == s1, true);
    require(s2.size(), 6u);
  }
  end_test_case();

  test_case("Move assignment");
  {
    micron::sstring<64> s1("moveassign");
    micron::sstring<64> s2;
    s2 = micron::move(s1);
    require(s2.size(), 10u);
    require(s1.size(), 0u);
  }
  end_test_case();

  test_case("Assignment from const char*");
  {
    micron::sstring<64> s;
    s = "assigned";
    require(s.size(), 8u);
    require(s[0], 'a');
  }
  end_test_case();

  test_case("Assignment from char*");
  {
    micron::sstring<64> s;
    char buf[] = "mutable";
    s = buf;
    require(s.size(), 7u);
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Element access
  // -----------------------------------------------------------------------

  test_case("operator[] read");
  {
    micron::sstring<64> s("abcdef");
    require(s[0], 'a');
    require(s[5], 'f');
  }
  end_test_case();

  test_case("operator[] write");
  {
    micron::sstring<64> s("abcdef");
    s[0] = 'Z';
    require(s[0], 'Z');
  }
  end_test_case();

  test_case("at() in range");
  {
    micron::sstring<64> s("abcdef");
    require(s.at(0), 'a');
    require(s.at(5), 'f');
  }
  end_test_case();

  test_case("at() out of range throws");
  {
    micron::sstring<64> s("abc");
    require_throw([&] { s.at(10); });
    require_throw([&] { s.at(3); });     // length == 3, index 3 is past end
  }
  end_test_case();

  test_case("at() with iterator in range");
  {
    micron::sstring<64> s("hello");
    auto it = s.begin() + 2;
    require(s.at(it), 2u);
  }
  end_test_case();

  test_case("at() with iterator out of range throws");
  {
    micron::sstring<64> s("hello");
    require_throw([&] {
      auto bad = s.begin() - 1;
      s.at(bad);
    });
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Iterators
  // -----------------------------------------------------------------------

  test_case("begin/end iteration");
  {
    micron::sstring<64> s("abc");
    size_t count = 0;
    for ( auto it = s.begin(); it != s.end(); ++it )
      ++count;
    require(count, 3u);
  }
  end_test_case();

  test_case("cbegin/cend iteration");
  {
    micron::sstring<64> s("xyz");
    const micron::sstring<64> &cs = s;
    require(*cs.cbegin(), 'x');
    require(*(cs.cend() - 1), 'z');
  }
  end_test_case();

  test_case("last() points to final element");
  {
    micron::sstring<64> s("abc");
    require(*s.last(), 'c');
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Capacity / state
  // -----------------------------------------------------------------------

  test_case("empty() and size()");
  {
    micron::sstring<64> s;
    require(s.empty(), true);
    s = "hi";
    require(s.empty(), false);
    require(s.size(), 2u);
  }
  end_test_case();

  test_case("capacity() always returns N");
  {
    micron::sstring<128> s("anything");
    require(s.capacity(), 128u);
  }
  end_test_case();

  test_case("operator! returns true when empty");
  {
    micron::sstring<64> s;
    require(!s, true);
    s = "x";
    require(!s, false);
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Modifiers
  // -----------------------------------------------------------------------

  test_case("clear() zeroes and resets length");
  {
    micron::sstring<64> s("clearme");
    s.clear();
    require(s.empty(), true);
    require(s.size(), 0u);
    require(s[0], '\0');
  }
  end_test_case();

  test_case("fast_clear() resets length only");
  {
    micron::sstring<64> s("fastclear");
    s.fast_clear();
    require(s.size(), 0u);
    // data may still be there — just length is 0
  }
  end_test_case();

  test_case("push_back(char)");
  {
    micron::sstring<64> s;
    s.push_back('a');
    s.push_back('b');
    s.push_back('c');
    require(s.size(), 3u);
    require(s[0], 'a');
    require(s[2], 'c');
  }
  end_test_case();

  test_case("push_back at capacity is silently ignored");
  {
    micron::sstring<4> s("abc");     // size 3, capacity 4 — one slot left for null
    s.push_back('d');                // length becomes 4, but length+1 < N fails so ignored
    // result depends on (length + 1) < N check; length=3, N=4: 3+1=4 is NOT < 4, so ignored
    require(s.size(), 3u);
  }
  end_test_case();

  test_case("pop_back()");
  {
    micron::sstring<64> s("hello");
    s.pop_back();
    require(s.size(), 4u);
    require(s[4], '\0');
  }
  end_test_case();

  test_case("pop_back() on empty is safe");
  {
    micron::sstring<64> s;
    s.pop_back();
    require(s.size(), 0u);
  }
  end_test_case();

  test_case("null_term() writes null at length position");
  {
    micron::sstring<64> s;
    s._buf_set_length(5);
    s[0] = 'h';
    s[1] = 'e';
    s[2] = 'l';
    s[3] = 'l';
    s[4] = 'o';
    s.null_term();
    require(s[5], '\0');
  }
  end_test_case();

  test_case("set_size() / _buf_set_length()");
  {
    micron::sstring<64> s("hello");
    s.set_size(3);
    require(s.size() == 3u);
  }
  end_test_case();

  test_case("adjust_size() syncs length from null terminator");
  {
    micron::sstring<64> s;
    s[0] = 'a';
    s[1] = 'b';
    s[2] = 'c';
    s[3] = '\0';
    s.adjust_size();
    require(s.size(), 3u);
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // operator+=
  // -----------------------------------------------------------------------

  test_case("operator+=(sstring)");
  {
    micron::sstring<64> s("foo");
    micron::sstring<64> t("bar");
    s += t;
    require(s == micron::sstring<64>("foobar"), true);
    require(s.size(), 6u);
  }
  end_test_case();

  test_case("operator+=(char)");
  {
    micron::sstring<64> s("ab");
    s += 'c';
    require(s.size(), 3u);
    require(s[2], 'c');
  }
  end_test_case();

  test_case("operator+=(const char[])");
  {
    micron::sstring<64> s("foo");
    s += "bar";
    require(s == micron::sstring<64>("foobar"), true);
  }
  end_test_case();

  test_case("operator+= overflow throws");
  {
    require_throw([&] {
      micron::sstring<8> s("hello");     // size 5
      s += "world!";                     // would need 11
    });
  }
  end_test_case();

  test_case("append(sstring)");
  {
    micron::sstring<64> s("hello");
    micron::sstring<64> t(" world");
    s.append(t);
    require(s == micron::sstring<64>("hello world"), true);
  }
  end_test_case();

  test_case("append empty sstring is no-op");
  {
    micron::sstring<64> s("hello");
    micron::sstring<64> empty;
    s.append(empty);
    require(s.size(), 5u);
  }
  end_test_case();

  test_case("append_null(char array)");
  {
    micron::sstring<64> s("foo");
    s.append_null("bar");
    require(s == micron::sstring<64>("foobar"), true);
    require(s.size(), 6u);
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Insert
  // -----------------------------------------------------------------------

  test_case("insert(index, char) at beginning");
  {
    micron::sstring<64> s("bcd");
    s.insert(0u, 'a');
    require(s == micron::sstring<64>("abcd"), true);
    require(s.size(), 4u);
  }
  end_test_case();

  test_case("insert(index, char) in middle");
  {
    micron::sstring<64> s("acd");
    s.insert(1u, 'b');
    require(s == micron::sstring<64>("abcd"), true);
  }
  end_test_case();

  test_case("insert(index, char) with cnt > 1");
  {
    micron::sstring<64> s("ad");
    s.insert(1u, 'b', 2);
    require(s == micron::sstring<64>("abbd"), true);
    require(s.size(), 4u);
  }
  end_test_case();

  test_case("insert(index, char[]) literal");
  {
    micron::sstring<64> s("abcd");
    s.insert(2u, "XX");
    require(s == micron::sstring<64>("abXXcd"), true);
  }
  end_test_case();

  test_case("insert(iterator, char)");
  {
    micron::sstring<64> s("abc");
    s.insert(s.begin() + 1, 'Z');
    require(s == micron::sstring<64>("aZbc"), true);
  }
  end_test_case();

  test_case("insert(iterator, char[])");
  {
    micron::sstring<64> s("ac");
    s.insert(s.begin() + 1, "XX");
    require(s == micron::sstring<64>("aXXc"), true);
  }
  end_test_case();

  test_case("insert(iterator, sstring lvalue)");
  {
    micron::sstring<64> s("ac");
    micron::sstring<64> mid("bb");
    s.insert(s.begin() + 1, mid);
    require(s == micron::sstring<64>("abbc"), true);
  }
  end_test_case();

  test_case("insert(iterator, sstring rvalue)");
  {
    micron::sstring<64> s("ac");
    micron::sstring<64> mid("bb");
    s.insert(s.begin() + 1, micron::move(mid));
    require(s == micron::sstring<64>("abbc"), true);
    require(mid.size(), 0u);     // source cleared
  }
  end_test_case();

  test_case("insert out of range throws");
  {
    require_throw([&] {
      micron::sstring<64> s("abc");
      s.insert(10u, 'x');     // ind >= length
    });
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Erase
  // -----------------------------------------------------------------------

  test_case("erase(index, cnt=1) from middle");
  {
    micron::sstring<64> s("abXcd");
    s.erase(2u, 1u);
    require(s == micron::sstring<64>("abcd"), true);
    require(s.size(), 4u);
  }
  end_test_case();

  test_case("erase(index, cnt) multiple chars");
  {
    micron::sstring<64> s("abXXcd");
    s.erase(2u, 2u);
    require(s == micron::sstring<64>("abcd"), true);
  }
  end_test_case();

  test_case("erase(iterator) from middle");
  {
    micron::sstring<64> s("aXb");
    s.erase(s.begin() + 1);
    require(s == micron::sstring<64>("ab"), true);
  }
  end_test_case();

  test_case("erase(const_iterator) from middle");
  {
    micron::sstring<64> s("aXb");
    s.erase(s.cbegin() + 1);
    require(s == micron::sstring<64>("ab"), true);
  }
  end_test_case();

  test_case("erase cnt=0 is no-op");
  {
    micron::sstring<64> s("abc");
    s.erase(1u, 0u);
    require(s.size(), 3u);
  }
  end_test_case();

  test_case("erase out of range throws");
  {
    require_throw([&] {
      micron::sstring<64> s("abc");
      s.erase(10u, 1u);
    });
  }
  end_test_case();

  test_case("erase cnt exceeds available chars throws");
  {
    require_throw([&] {
      micron::sstring<64> s("abc");
      s.erase(2u, 5u);     // only 1 char after index 2
    });
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // substr
  // -----------------------------------------------------------------------

  test_case("substr(pos, cnt) basic");
  {
    micron::sstring<64> s("hello world");
    auto sub = s.substr(6u, 5u);
    require(sub == micron::sstring<64>("world"), true);
    require(sub.size(), 5u);
  }
  end_test_case();

  test_case("substr(pos, cnt) from beginning");
  {
    micron::sstring<64> s("abcdef");
    auto sub = s.substr(0u, 3u);
    require(sub == micron::sstring<64>("abc"), true);
  }
  end_test_case();

  test_case("substr(pos, cnt=npos) to end");
  {
    micron::sstring<64> s("hello world");
    auto sub = s.substr(6u);     // cnt defaults to npos -> to end
    require(sub == micron::sstring<64>("world"), true);
  }
  end_test_case();

  test_case("substr(pos=0, cnt=npos) full copy");
  {
    micron::sstring<64> s("fullcopy");
    auto sub = s.substr();
    require(sub == s, true);
  }
  end_test_case();

  test_case("substr(pos, cnt) out of range throws");
  {
    require_throw([&] {
      micron::sstring<64> s("hello");
      auto sub = s.substr(10u, 1u);
    });
  }
  end_test_case();

  test_case("substr(pos, cnt) pos+cnt > length throws");
  {
    require_throw([&] {
      micron::sstring<64> s("hello");
      auto sub = s.substr(3u, 10u);
    });
  }
  end_test_case();

  test_case("substr(const_iterator, const_iterator) basic");
  {
    micron::sstring<64> s("hello world");
    auto sub = s.substr(s.cbegin() + 6, s.cend());
    require(sub == micron::sstring<64>("world"), true);
    require(sub.size(), 5u);
  }
  end_test_case();

  test_case("substr(const_iterator, nullptr) to end");
  {
    micron::sstring<64> s("hello world");
    auto sub = s.substr(s.cbegin() + 6);
    require(sub == micron::sstring<64>("world"), true);
  }
  end_test_case();

  test_case("substr(iterator) full string from begin");
  {
    micron::sstring<64> s("abc");
    auto sub = s.substr(s.cbegin());
    require(sub == s, true);
  }
  end_test_case();

  test_case("substr iterator reversed throws");
  {
    require_throw([&] {
      micron::sstring<64> s("hello");
      auto sub = s.substr(s.cend(), s.cbegin());
    });
  }
  end_test_case();

  test_case("substr iterator out of range throws");
  {
    require_throw([&] {
      micron::sstring<64> s("hello");
      micron::sstring<64> other("other");
      // iterator from a different string - will be out of [begin, end)
      auto sub = s.substr(other.cbegin(), other.cend());
    });
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // find
  // -----------------------------------------------------------------------

  test_case("find(char) present");
  {
    micron::sstring<64> s("hello");
    require(s.find('l'), 2u);
  }
  end_test_case();

  test_case("find(char) absent returns npos");
  {
    micron::sstring<64> s("hello");
    require(s.find('z'), micron::npos);
  }
  end_test_case();

  test_case("find(char, pos) skips before pos");
  {
    micron::sstring<64> s("hello");
    require(s.find('l', 3u), 3u);     // second 'l' at index 3
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Truncate
  // -----------------------------------------------------------------------

  test_case("truncate(n) shortens string");
  {
    micron::sstring<64> s("hello world");
    s.truncate(5u);
    require(s.size(), 5u);
    require(s == micron::sstring<64>("hello"), true);
    require(s[5], '\0');     // tail zeroed
  }
  end_test_case();

  test_case("truncate(n) no-op when n >= length");
  {
    micron::sstring<64> s("hello");
    s.truncate(10u);
    require(s.size(), 5u);
    s.truncate(5u);
    require(s.size(), 5u);
  }
  end_test_case();

  test_case("truncate(n=0) clears string");
  {
    micron::sstring<64> s("hello");
    s.truncate(0u);
    require(s.size(), 0u);
    require(s[0], '\0');
  }
  end_test_case();

  test_case("truncate(n) zeroes vacated tail");
  {
    micron::sstring<64> s("hello");
    s.truncate(2u);
    require(s[2], '\0');
    require(s[3], '\0');
    require(s[4], '\0');
  }
  end_test_case();

  test_case("truncate(iterator) at midpoint");
  {
    micron::sstring<64> s("hello world");
    s.truncate(s.begin() + 5);
    require(s.size(), 5u);
    require(s == micron::sstring<64>("hello"), true);
  }
  end_test_case();

  test_case("truncate(iterator) at begin clears");
  {
    micron::sstring<64> s("hello");
    s.truncate(s.begin());
    require(s.size(), 0u);
  }
  end_test_case();

  test_case("truncate(iterator) at end is no-op");
  {
    micron::sstring<64> s("hello");
    s.truncate(s.end());
    require(s.size(), 5u);
  }
  end_test_case();

  test_case("truncate(iterator) out of range throws");
  {
    require_throw([&] {
      micron::sstring<64> s("hello");
      micron::sstring<64> other("other");
      s.truncate(other.begin());
    });
  }
  end_test_case();

  test_case("truncate(const_iterator) at midpoint");
  {
    micron::sstring<64> s("hello world");
    s.truncate(s.cbegin() + 5);
    require(s.size(), 5u);
    require(s == micron::sstring<64>("hello"), true);
  }
  end_test_case();

  test_case("truncate(const_iterator) out of range throws");
  {
    require_throw([&] {
      micron::sstring<64> s("hello");
      micron::sstring<64> other("other");
      s.truncate(other.cbegin());
    });
  }
  end_test_case();

  test_case("truncate then append round-trip");
  {
    micron::sstring<64> s("hello world");
    s.truncate(5u);
    s += "!";
    require(s == micron::sstring<64>("hello!"), true);
    require(s.size(), 6u);
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // into_bytes
  // -----------------------------------------------------------------------

  test_case("into_bytes() slice covers live data");
  {
    micron::sstring<64> s("abc");
    auto sl = s.into_bytes();
    require(sl.size(), 3u);
    require((char)sl[0], 'a');
    require((char)sl[2], 'c');
  }
  end_test_case();

  test_case("into_bytes() on empty string has size 0");
  {
    micron::sstring<64> s;
    auto sl = s.into_bytes();
    require(sl.size(), 0u);
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Comparison operators
  // -----------------------------------------------------------------------

  test_case("operator== with sstring");
  {
    micron::sstring<64> a("hello");
    micron::sstring<64> b("hello");
    micron::sstring<64> c("world");
    require(a == b, true);
    require(a == c, false);
  }
  end_test_case();

  test_case("operator== with const char*");
  {
    micron::sstring<64> s("hello");
    require(s == "hello", true);
    require(s == "world", false);
  }
  end_test_case();

  test_case("operator!= with sstring");
  {
    micron::sstring<64> a("hello");
    micron::sstring<64> b("world");
    require(a != b, true);
    require(a != a, false);
  }
  end_test_case();

  test_case("operator!= with const char*");
  {
    micron::sstring<64> s("hello");
    require(s != "world", true);
    require(s != "hello", false);
  }
  end_test_case();

  test_case("operator!= length mismatch");
  {
    micron::sstring<64> a("hi");
    micron::sstring<64> b("hello");
    require(a != b, true);
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Data / c_str / clone
  // -----------------------------------------------------------------------

  test_case("c_str() returns null-terminated C string");
  {
    micron::sstring<64> s("hello");
    const char *ptr = s.c_str();
    require(ptr[0], 'h');
    require(ptr[5], '\0');
  }
  end_test_case();

  test_case("data() returns pointer to internal buffer");
  {
    micron::sstring<64> s("hello");
    require(s.data()[0], 'h');
  }
  end_test_case();

  test_case("cdata() returns const ref to internal array");
  {
    micron::sstring<64> s("hello");
    const auto &arr = s.cdata();
    require(arr[0], 'h');
  }
  end_test_case();

  test_case("clone() produces independent copy");
  {
    micron::sstring<64> s("original");
    auto c = s.clone();
    require(c == s, true);
    c[0] = 'Z';
    require(s[0], 'o');     // original unchanged
  }
  end_test_case();

  test_case("clone_to<>() produces copy in target type");
  {
    micron::sstring<64> s("hello");
    auto c = s.clone_to<micron::sstring<128>>();
    require(c == micron::sstring<128>("hello"), true);
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // addr / operator&
  // -----------------------------------------------------------------------

  test_case("operator&() returns pointer to internal buffer bytes");
  {
    micron::sstring<64> s("abc");
    const byte *p = &s;
    require((char)p[0], 'a');
  }
  end_test_case();

  test_case("addr() returns pointer to sstring object itself");
  {
    micron::sstring<64> s("abc");
    require(s.addr() == reinterpret_cast<micron::sstring<64>*>(&s), true); // will be same addr
    // addr() returns `this` (the sstring*), operator& returns &memory[0]
    require((void *)s.addr(), (void *)&s);
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Chaining
  // -----------------------------------------------------------------------

  test_case("chained push_back");
  {
    micron::sstring<64> s;
    s.push_back('a').push_back('b').push_back('c');
    require(s == micron::sstring<64>("abc"), true);
  }
  end_test_case();

  test_case("chained pop_back");
  {
    micron::sstring<64> s("abc");
    s.pop_back().pop_back();
    require(s == micron::sstring<64>("a"), true);
  }
  end_test_case();

  test_case("chained truncate and push_back");
  {
    micron::sstring<64> s("hello world");
    s.truncate(5u).push_back('!');
    require(s == micron::sstring<64>("hello!"), true);
  }
  end_test_case();

  // -----------------------------------------------------------------------
  // Edge cases / boundary
  // -----------------------------------------------------------------------

  test_case("single character string");
  {
    micron::sstring<4> s("a");
    require(s.size(), 1u);
    require(s[0], 'a');
    require(s[1], '\0');
  }
  end_test_case();

  test_case("string exactly at capacity boundary");
  {
    // N=8, string of length 7 should be fine (null at [7])
    micron::sstring<8> s("1234567");
    require(s.size(), 7u);
  }
  end_test_case();

  test_case("insert then erase identity");
  {
    micron::sstring<64> s("abcd");
    s.insert(2u, "XX");
    require(s == micron::sstring<64>("abXXcd"), true);
    s.erase(2u, 2u);
    require(s == micron::sstring<64>("abcd"), true);
  }
  end_test_case();

  test_case("multiple operations sequence");
  {
    micron::sstring<64> s("hello");
    s += " ";
    s += "world";
    s.truncate(5u);
    s.push_back('!');
    require(s == micron::sstring<64>("hello!"), true);
    require(s.size(), 6u);
  }
  end_test_case();

  test_case("clear then reuse");
  {
    micron::sstring<64> s("first");
    s.clear();
    s = "second";
    require(s == micron::sstring<64>("second"), true);
    require(s.size(), 6u);
  }
  end_test_case();

  test_case("substr zero-length result");
  {
    micron::sstring<64> s("hello");
    auto sub = s.substr(2u, 0u);
    require(sub.size(), 0u);
    require(sub.empty(), true);
  }
  end_test_case();

  test_case("erase from start shifts content correctly");
  {
    micron::sstring<64> s("Xhello");
    s.erase(0u, 1u);
    require(s == micron::sstring<64>("hello"), true);
  }
  end_test_case();

  test_case("erase last character");
  {
    micron::sstring<64> s("hello!");
    s.erase(5u, 1u);
    require(s == micron::sstring<64>("hello"), true);
  }
  end_test_case();

  sb::print("=== ALL SSTRING TESTS PASSED ===");
  return 1;
}
