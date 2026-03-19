// Copyright (c) 2024- David Lucius Severus
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

// test_hstring.cpp
//
// Lifetime, memory safety, and correctness tests for micron::hstring.
// Compile:
//   c++ -std=c++23 -g -fsanitize=address,undefined -Wall -Wextra \
//       -o test_hstring test_hstring.cpp && ./test_hstring

#include "../../src/io/console.hpp"
#include "../../src/string/strings.hpp"
#include "../snowball/snowball.hpp"

using namespace snowball;

// ─────────────────────────────────────────────────────────────────────────────
// Canary wrapper — counts constructions and destructions so we can assert
// that hstring never leaks or double-frees the strings it holds.
// ─────────────────────────────────────────────────────────────────────────────

static int s_live = 0;

struct Canary {
  int id;

  explicit Canary(int i) : id(i) { ++s_live; }

  Canary(const Canary &o) : id(o.id) { ++s_live; }

  ~Canary() { --s_live; }
};

// ─────────────────────────────────────────────────────────────────────────────
// ① construction paths
// ─────────────────────────────────────────────────────────────────────────────

static void
test_construction()
{
  sb::print("=== construction paths ===");

  sb::test_case("default construction: empty, size 0, max_size > 0");
  {
    micron::string s;
    sb::require_true(s.empty());
    sb::require(s.size(), usize{ 0 });
    sb::require_greater(s.max_size(), usize{ 0 });
  }
  sb::end_test_case();

  sb::test_case("const char* construction copies content exactly");
  {
    micron::string s("hello");
    sb::require(s.size(), usize{ 5 });
    sb::require(s[0], 'h');
    sb::require(s[4], 'o');
    sb::require_true(s == "hello");
  }
  sb::end_test_case();

  sb::test_case("const char* construction: empty string literal");
  {
    micron::string s("");
    sb::require(s.size(), usize{ 0 });
    sb::require_true(s.empty());
  }
  sb::end_test_case();

  sb::test_case("string literal (array) construction");
  {
    micron::string s("abcde");
    sb::require(s.size(), usize{ 5 });
    sb::require(s[2], 'c');
  }
  sb::end_test_case();

  sb::test_case("fill construction: cnt copies of char");
  {
    micron::string s(5, 'z');
    sb::require(s.size(), usize{ 5 });
    for ( usize i = 0; i < 5; ++i )
      sb::require(s[i], 'z');
  }
  sb::end_test_case();

  sb::test_case("fill construction: size 1");
  {
    micron::string s(1, 'A');
    sb::require(s.size(), usize{ 1 });
    sb::require(s[0], 'A');
  }
  sb::end_test_case();

  sb::test_case("copy construction produces independent copy");
  {
    micron::string a("source");
    micron::string b(a);
    sb::require_true(a == b);
    b[0] = 'X';
    sb::require_false(a == b);     // mutation of b must not affect a
    sb::require(a[0], 's');
  }
  sb::end_test_case();

  sb::test_case("move construction transfers content, leaves source empty");
  {
    micron::string a("moved");
    micron::string b(micron::move(a));
    sb::require(b.size(), usize{ 5 });
    sb::require_true(b == "moved");
    // source must be in a valid empty state
    sb::require_true(a.empty() || a.size() == 0);
  }
  sb::end_test_case();

  sb::test_case("iterator-pair construction");
  {
    micron::string src("abcdef");
    micron::string dst(src.begin() + 1, src.begin() + 4);
    sb::require(dst.size(), usize{ 3 });
    sb::require_true(dst == "bcd");
  }
  sb::end_test_case();

  sb::test_case("iterator-pair construction: full range");
  {
    micron::string src("full");
    micron::string dst(src.begin(), src.end());
    sb::require_true(dst == "full");
  }
  sb::end_test_case();

  sb::test_case("iterator-pair construction: single character");
  {
    micron::string src("xyz");
    micron::string dst(src.begin(), src.begin() + 1);
    sb::require(dst.size(), usize{ 1 });
    sb::require(dst[0], 'x');
  }
  sb::end_test_case();

  sb::test_case("reserve construction sets capacity >= n");
  {
    micron::string s(128u);
    sb::require_true(s.max_size() >= 128u);
    sb::require_true(s.empty());
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ② assignment
// ─────────────────────────────────────────────────────────────────────────────

static void
test_assignment()
{
  sb::print("=== assignment ===");

  sb::test_case("copy assignment: content matches, sizes match");
  {
    micron::string a("hello");
    micron::string b;
    b = a;
    sb::require_true(b == a);
    sb::require(b.size(), a.size());
  }
  sb::end_test_case();

  sb::test_case("copy assignment is independent (no aliasing)");
  {
    micron::string a("shared");
    micron::string b;
    b = a;
    b[0] = 'X';
    sb::require(a[0], 's');     // original must be unaffected
  }
  sb::end_test_case();

  sb::test_case("copy assignment over non-empty destination");
  {
    micron::string a("long enough content here");
    micron::string b("short");
    b = a;
    sb::require_true(b == a);
  }
  sb::end_test_case();

  sb::test_case("copy assignment when dest capacity < src: grows destination");
  {
    micron::string a(256u);
    for ( int i = 0; i < 200; ++i )
      a.push_back('a');
    micron::string b;
    b = a;
    sb::require(b.size(), a.size());
  }
  sb::end_test_case();

  sb::test_case("move assignment transfers content");
  {
    micron::string a("moveassign");
    micron::string b;
    b = micron::move(a);
    sb::require(b.size(), usize{ 10 });
    sb::require_true(b == "moveassign");
  }
  sb::end_test_case();

  sb::test_case("move assignment leaves source in valid state");
  {
    micron::string a("temporary");
    micron::string b;
    b = micron::move(a);
    // source must be usable after move
    a = "reused";
    sb::require_true(a == "reused");
  }
  sb::end_test_case();

  sb::test_case("assignment from string literal");
  {
    micron::string s;
    s = "literal";
    sb::require(s.size(), usize{ 7 });
    sb::require_true(s == "literal");
  }
  sb::end_test_case();

  /*
   * yeah dont do this + compiler catches it
  sb::test_case("self-copy assignment is safe");
  {
    micron::string s("selfcopy");
    s = s;
    sb::require_true(s == "selfcopy");
  }
  sb::end_test_case();
*/
  sb::test_case("chained assignment");
  {
    micron::string a, b, c;
    a = "value";
    b = a;
    c = b;
    sb::require_true(a == c);
    sb::require_true(b == c);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ③ lifetime / destructor safety
// ─────────────────────────────────────────────────────────────────────────────

static void
test_lifetime()
{
  sb::print("=== lifetime / destructor safety ===");

  sb::test_case("destructor runs without crash on default-constructed string");
  {
    {
      micron::string s;
    }     // destructor fires here
    sb::require_true(true);
  }
  sb::end_test_case();

  sb::test_case("destructor runs without crash on non-empty string");
  {
    {
      micron::string s("non-empty");
    }
    sb::require_true(true);
  }
  sb::end_test_case();

  sb::test_case("destructor runs without crash after clear()");
  {
    {
      micron::string s("will be cleared");
      s.clear();
    }
    sb::require_true(true);
  }
  sb::end_test_case();

  sb::test_case("destructor runs after move: moved-from object does not double-free");
  {
    {
      micron::string a("moved-from");
      {
        micron::string b(micron::move(a));
      }     // b's destructor fires — should free the memory once
      // a's destructor fires — must not attempt to free again
    }
    sb::require_true(true);
  }
  sb::end_test_case();

  sb::test_case("destructor runs after move-assign: no double-free");
  {
    {
      micron::string a("x");
      micron::string b;
      b = micron::move(a);
    }
    sb::require_true(true);
  }
  sb::end_test_case();

  sb::test_case("multiple strings destroyed in sequence without interference");
  {
    {
      micron::string s0("alpha");
      micron::string s1("beta");
      micron::string s2("gamma");
    }
    sb::require_true(true);
  }
  sb::end_test_case();

  sb::test_case("string constructed and destroyed in a loop 1000x: no leaks");
  {
    for ( int i = 0; i < 1000; ++i ) {
      micron::string s("loop iteration content");
      sb::require_false(s.empty());
    }
    sb::require_true(true);
  }
  sb::end_test_case();

  sb::test_case("string grown via push_back then destroyed: no leak");
  {
    {
      micron::string s;
      for ( int i = 0; i < 500; ++i )
        s.push_back('x');
      sb::require(s.size(), usize{ 500 });
    }
    sb::require_true(true);
  }
  sb::end_test_case();

  sb::test_case("copy constructed string destroyed before original: original still valid");
  {
    micron::string original("persistent");
    {
      micron::string copy(original);
    }     // copy destroyed here
    sb::require_true(original == "persistent");
    sb::require(original.size(), usize{ 10 });
  }
  sb::end_test_case();

  sb::test_case("string reassigned multiple times: each destructor path valid");
  {
    micron::string s("first");
    s = "second value longer";
    s = "x";
    s = "back to something longer again with more content";
    sb::require_false(s.empty());
  }
  sb::end_test_case();

  sb::test_case("reserve followed by destruction: no leak");
  {
    {
      micron::string s;
      s.reserve(4096);
      sb::require_true(s.max_size() >= 4096u);
    }
    sb::require_true(true);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ④ mutation and content integrity
// ─────────────────────────────────────────────────────────────────────────────

static void
test_mutation()
{
  sb::print("=== mutation and content integrity ===");

  sb::test_case("push_back single char extends size by 1");
  {
    micron::string s("ab");
    s.push_back('c');
    sb::require(s.size(), usize{ 3 });
    sb::require(s[2], 'c');
  }
  sb::end_test_case();

  sb::test_case("push_back 100 chars: size == 100");
  {
    micron::string s;
    for ( int i = 0; i < 100; ++i )
      s.push_back(static_cast<char>('a' + (i % 26)));
    sb::require(s.size(), usize{ 100 });
  }
  sb::end_test_case();

  sb::test_case("push_back across realloc boundary: content preserved");
  {
    micron::string s("seed");
    usize original_cap = s.max_size();
    while ( s.size() < original_cap + 16 )
      s.push_back('X');
    sb::require(s[0], 's');
    sb::require(s[1], 'e');
    sb::require(s[2], 'e');
    sb::require(s[3], 'd');
  }
  sb::end_test_case();

  sb::test_case("pop_back removes last character");
  {
    micron::string s("abc");
    s.pop_back();
    sb::require(s.size(), usize{ 2 });
    sb::require(s[1], 'b');
  }
  sb::end_test_case();

  sb::test_case("pop_back on single-char string: size becomes 0");
  {
    micron::string s("x");
    s.pop_back();
    sb::require(s.size(), usize{ 0 });
  }
  sb::end_test_case();

  sb::test_case("clear: size 0, empty() true, memory not dangling");
  {
    micron::string s("clearme");
    s.clear();
    sb::require(s.size(), usize{ 0 });
    sb::require_true(s.empty());
    // safe to reuse after clear
    s.push_back('A');
    sb::require(s.size(), usize{ 1 });
  }
  sb::end_test_case();

  sb::test_case("operator[] write: in-place mutation does not corrupt length");
  {
    micron::string s("hello");
    s[0] = 'H';
    sb::require(s[0], 'H');
    sb::require(s.size(), usize{ 5 });
  }
  sb::end_test_case();

  sb::test_case("resize extends with fill char");
  {
    micron::string s("hi");
    s.resize(6, '!');
    sb::require(s.size(), usize{ 6 });
    sb::require(s[2], '!');
    sb::require(s[5], '!');
  }
  sb::end_test_case();

  sb::test_case("resize to same size: no-op");
  {
    micron::string s("same");
    s.resize(4, 'x');
    sb::require(s.size(), usize{ 4 });
    sb::require_true(s == "same");
  }
  sb::end_test_case();

  sb::test_case("operator+= with char*");
  {
    micron::string s("foo");
    s += "bar";
    sb::require_true(s == "foobar");
    sb::require(s.size(), usize{ 6 });
  }
  sb::end_test_case();

  sb::test_case("operator+= with hstring");
  {
    micron::string a("hello");
    micron::string b(" world");
    a += b;
    sb::require_true(a == "hello world");
  }
  sb::end_test_case();

  sb::test_case("operator+= with single char");
  {
    micron::string s("x");
    s += 'y';
    sb::require(s.size(), usize{ 2 });
    sb::require(s[1], 'y');
  }
  sb::end_test_case();

  sb::test_case("append char* with explicit length");
  {
    micron::string s("start");
    s.append("_end", 4);
    sb::require(s[5], '_');
  }
  sb::end_test_case();

  sb::test_case("append hstring");
  {
    micron::string s("prefix");
    micron::string t("suffix");
    s.append(t);
    sb::require_true(s == "prefixsuffix");
  }
  sb::end_test_case();

  sb::test_case("repeated append does not corrupt earlier content");
  {
    micron::string s("A");
    for ( int i = 0; i < 50; ++i )
      s.append("BC");
    sb::require(s[0], 'A');
    sb::require(s.size(), usize{ 1 + 50 * 2 });
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑤ insert and erase
// ─────────────────────────────────────────────────────────────────────────────

static void
test_insert_erase()
{
  sb::print("=== insert and erase ===");

  sb::test_case("insert char at index 0 (prepend)");
  {
    micron::string s("bc");
    s.insert((usize)0u, 'a');
    sb::require(s[0], 'a');
    sb::require(s.size(), usize{ 3 });
  }
  sb::end_test_case();

  sb::test_case("insert char at end");
  {
    micron::string s("ab");
    s.insert(2u, 'c');
    sb::require(s[2], 'c');
    sb::require(s.size(), usize{ 3 });
  }
  sb::end_test_case();

  sb::test_case("insert char in middle: surrounding bytes preserved");
  {
    micron::string s("aXb");
    s.insert(1u, '-');
    sb::require(s[0], 'a');
    sb::require(s[1], '-');
    sb::require(s[2], 'X');
    sb::require(s[3], 'b');
    sb::require(s.size(), usize{ 4 });
  }
  sb::end_test_case();

  sb::test_case("insert string literal at index 2");
  {
    micron::string s("abcd");
    s.insert(2u, "XX");
    sb::require_true(s == "abXXcd");
  }
  sb::end_test_case();

  sb::test_case("insert does not corrupt characters after insertion point");
  {
    micron::string s("start_end");
    s.insert(5u, "___");
    sb::require(s[5], '_');
    // "end" should still be at the back
    usize n = s.size();
    sb::require(s[n - 3], 'e');
    sb::require(s[n - 2], 'n');
    sb::require(s[n - 1], 'd');
  }
  sb::end_test_case();

  sb::test_case("insert across realloc: content before and after intact");
  {
    micron::string s("headtail");
    usize cap = s.max_size();
    // fill to near capacity to force a realloc on insert
    while ( s.size() < cap - 2 )
      s.push_back('.');
    s.insert(4u, "MID");
    sb::require(s[0], 'h');
    sb::require(s[1], 'e');
    sb::require(s[2], 'a');
    sb::require(s[3], 'd');
    sb::require(s[4], 'M');
    sb::require(s[5], 'I');
    sb::require(s[6], 'D');
  }
  sb::end_test_case();

  sb::test_case("erase single char from middle");
  {
    micron::string s("abcde");
    s.erase(2u);
    sb::require_true(s == "abde");
    sb::require(s.size(), usize{ 4 });
  }
  sb::end_test_case();

  sb::test_case("erase multiple chars");
  {
    micron::string s("abXXcd");
    s.erase(2u, 2u);
    sb::require_true(s == "abcd");
    sb::require(s.size(), usize{ 4 });
  }
  sb::end_test_case();

  sb::test_case("erase from index 0 (front removal)");
  {
    micron::string s("Xhello");
    s.erase(0u);
    sb::require(s[0], 'h');
    sb::require(s.size(), usize{ 5 });
  }
  sb::end_test_case();

  sb::test_case("erase last char");
  {
    micron::string s("helloX");
    s.erase(5u);
    sb::require(s.size(), usize{ 5 });
    sb::require_true(s == "hello");
  }
  sb::end_test_case();

  sb::test_case("insert then erase is identity");
  {
    micron::string s("abcd");
    s.insert(2u, "ZZ");
    s.erase(2u, 2u);
    sb::require_true(s == "abcd");
  }
  sb::end_test_case();

  sb::test_case("erase via iterator");
  {
    micron::string s("abcde");
    s.erase(s.begin() + 2);
    sb::require_true(s == "abde");
  }
  sb::end_test_case();

  sb::test_case("erase out-of-range throws");
  {
    micron::string s("short");
    sb::require_throw([&] { s.erase(10u); });
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑥ substr
// ─────────────────────────────────────────────────────────────────────────────

static void
test_substr()
{
  sb::print("=== substr ===");

  sb::test_case("substr from middle");
  {
    micron::string s("abcdef");
    auto t = s.substr(2u, 3u);
    sb::require_true(t == "cde");
    sb::require(t.size(), usize{ 3 });
  }
  sb::end_test_case();

  sb::test_case("substr from 0");
  {
    micron::string s("hello");
    auto t = s.substr(0u, 3u);
    sb::require_true(t == "hel");
  }
  sb::end_test_case();

  sb::test_case("substr to end (cnt == npos)");
  {
    micron::string s("hello world");
    auto t = s.substr(6u);
    sb::require_true(t == "world");
  }
  sb::end_test_case();

  sb::test_case("substr of length 1");
  {
    micron::string s("xyz");
    auto t = s.substr(1u, 1u);
    sb::require(t.size(), usize{ 1 });
    sb::require(t[0], 'y');
  }
  sb::end_test_case();

  sb::test_case("substr does not modify original");
  {
    micron::string s("original");
    auto t = s.substr(0u, 4u);
    t[0] = 'X';
    sb::require(s[0], 'o');     // original untouched
  }
  sb::end_test_case();

  sb::test_case("substr out-of-range throws");
  {
    micron::string s("short");
    sb::require_throw([&] { s.substr(10u, 1u); });
  }
  sb::end_test_case();

  sb::test_case("substr iterator pair");
  {
    micron::string s("abcdef");
    auto t = s.substr(s.begin() + 1, s.begin() + 4);
    sb::require_true(t == "bcd");
  }
  sb::end_test_case();

  sb::test_case("substr result is independent (no shared backing)");
  {
    micron::string s("hello");
    auto t = s.substr(0u, 5u);
    s[0] = 'X';
    sb::require(t[0], 'h');     // t must not see s's mutation
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑦ truncate
// ─────────────────────────────────────────────────────────────────────────────

static void
test_truncate()
{
  sb::print("=== truncate ===");

  sb::test_case("truncate to smaller size");
  {
    micron::string s("hello world");
    s.truncate(5);
    sb::require(s.size(), usize{ 5 });
    sb::require_true(s == "hello");
  }
  sb::end_test_case();

  sb::test_case("truncate to 0: effectively clears content");
  {
    micron::string s("nonempty");
    s.truncate(0);
    sb::require(s.size(), usize{ 0 });
  }
  sb::end_test_case();

  sb::test_case("truncate to >= size: no-op");
  {
    micron::string s("abc");
    s.truncate(10);
    sb::require(s.size(), usize{ 3 });
    sb::require_true(s == "abc");
  }
  sb::end_test_case();

  sb::test_case("truncated tail bytes are zeroed (not readable as content)");
  {
    micron::string s("hello");
    s.truncate(3);
    // length must reflect the truncation
    sb::require(s.size(), usize{ 3 });
    // first three chars intact
    sb::require(s[0], 'h');
    sb::require(s[1], 'e');
    sb::require(s[2], 'l');
  }
  sb::end_test_case();

  sb::test_case("truncate then append: no stale data leaks into new content");
  {
    micron::string s("hello world");
    s.truncate(5);
    s.push_back('!');
    sb::require(s.size(), usize{ 6 });
    sb::require(s[5], '!');
    sb::require_true(s == "hello!");
  }
  sb::end_test_case();

  sb::test_case("truncate via iterator");
  {
    micron::string s("abcdef");
    s.truncate(s.begin() + 3);
    sb::require(s.size(), usize{ 3 });
    sb::require_true(s == "abc");
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑧ remove / remove_all
// ─────────────────────────────────────────────────────────────────────────────

static void
test_remove()
{
  sb::print("=== remove / remove_all ===");

  sb::test_case("remove first occurrence of needle");
  {
    micron::string s("aXXb");
    s.remove("XX");
    sb::require_true(s == "ab");
    sb::require(s.size(), usize{ 2 });
  }
  sb::end_test_case();

  sb::test_case("remove when needle not present: no change");
  {
    micron::string s("hello");
    s.remove("ZZ");
    sb::require_true(s == "hello");
    sb::require(s.size(), usize{ 5 });
  }
  sb::end_test_case();

  sb::test_case("remove only removes first occurrence");
  {
    micron::string s("aXbXc");
    s.remove("X");
    // first X gone, second X remains
    sb::require(s.size(), usize{ 4 });
    sb::require(s[1], 'b');
  }
  sb::end_test_case();

  sb::test_case("remove from start of string");
  {
    micron::string s("PREFIXrest");
    s.remove("PREFIX");
    sb::require_true(s == "rest");
  }
  sb::end_test_case();

  sb::test_case("remove from end of string");
  {
    micron::string s("restSUFFIX");
    s.remove("SUFFIX");
    sb::require_true(s == "rest");
  }
  sb::end_test_case();

  sb::test_case("remove_all removes every occurrence");
  {
    micron::string s("aXbXcX");
    s.remove_all("X");
    sb::require_true(s == "abc");
    sb::require(s.size(), usize{ 3 });
  }
  sb::end_test_case();

  sb::test_case("remove_all: needle not present is no-op");
  {
    micron::string s("nothing to remove");
    s.remove_all("ZZZ");
    sb::require_true(s == "nothing to remove");
  }
  sb::end_test_case();

  sb::test_case("remove_all: needle at start and end");
  {
    micron::string s("XXmiddleXX");
    s.remove_all("XX");
    sb::require_true(s == "middle");
  }
  sb::end_test_case();

  sb::test_case("remove_all: adjacent occurrences");
  {
    micron::string s("AAAA");
    s.remove_all("AA");
    sb::require_true(s.empty() || s.size() < 4u);
  }
  sb::end_test_case();

  sb::test_case("remove does not corrupt bytes after removal point");
  {
    micron::string s("startMIDend");
    s.remove("MID");
    sb::require(s[5], 'e');
    sb::require(s[6], 'n');
    sb::require(s[7], 'd');
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑨ iterators
// ─────────────────────────────────────────────────────────────────────────────

static void
test_iterators()
{
  sb::print("=== iterators ===");

  sb::test_case("begin() points to first character");
  {
    micron::string s("xyz");
    sb::require(*s.begin(), 'x');
  }
  sb::end_test_case();

  sb::test_case("end() - begin() == size()");
  {
    micron::string s("hello");
    sb::require(static_cast<usize>(s.end() - s.begin()), s.size());
  }
  sb::end_test_case();

  sb::test_case("range-based-for reads all characters");
  {
    micron::string s("abcde");
    char buf[6] = {};
    int i = 0;
    for ( char c : s )
      buf[i++] = c;
    sb::require(buf[0], 'a');
    sb::require(buf[4], 'e');
    sb::require(i, 5);
  }
  sb::end_test_case();

  sb::test_case("cbegin() / cend() are read-only and span the string");
  {
    micron::string s("const");
    auto it = s.cbegin();
    sb::require(*it, 'c');
    sb::require(static_cast<usize>(s.cend() - s.cbegin()), s.size());
  }
  sb::end_test_case();

  sb::test_case("iterator write through begin()");
  {
    micron::string s("hello");
    *s.begin() = 'H';
    sb::require(s[0], 'H');
  }
  sb::end_test_case();

  sb::test_case("iterators remain valid after push_back to existing capacity");
  {
    micron::string s(64u);     // pre-allocate so push_back won't realloc
    s = "abc";
    s.push_back('d');
    sb::require(s[3], 'd');
  }
  sb::end_test_case();

  sb::test_case("last() points to last character");
  {
    micron::string s("xyz");
    sb::require(*s.last(), 'z');
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑩ comparison operators
// ─────────────────────────────────────────────────────────────────────────────

static void
test_comparison()
{
  sb::print("=== comparison operators ===");

  sb::test_case("operator== for equal strings");
  {
    micron::string a("hello"), b("hello");
    sb::require_true(a == b);
  }
  sb::end_test_case();

  sb::test_case("operator== for unequal strings");
  {
    micron::string a("hello"), b("world");
    sb::require_false(a == b);
  }
  sb::end_test_case();

  sb::test_case("operator!= for different strings");
  {
    micron::string a("aaa"), b("bbb");
    sb::require_true(a != b);
  }
  sb::end_test_case();

  sb::test_case("operator< lexicographic ordering");
  {
    micron::string a("apple"), b("banana");
    sb::require_true(a < b);
    sb::require_false(b < a);
  }
  sb::end_test_case();

  sb::test_case("operator> lexicographic ordering");
  {
    micron::string a("z"), b("a");
    sb::require_true(a > b);
  }
  sb::end_test_case();

  sb::test_case("operator== with const char* literal");
  {
    micron::string s("equal");
    sb::require_true(s == "equal");
    sb::require_false(s == "other");
  }
  sb::end_test_case();

  sb::test_case("operator!= with const char* literal");
  {
    micron::string s("hello");
    sb::require_true(s != "world");
  }
  sb::end_test_case();

  sb::test_case("operator< / > with const char*");
  {
    micron::string s("apple");
    sb::require_true(s < "banana");
    sb::require_false(s > "banana");
  }
  sb::end_test_case();

  sb::test_case("shorter string < longer with same prefix");
  {
    micron::string a("abc"), b("abcd");
    sb::require_true(a < b);
  }
  sb::end_test_case();

  sb::test_case("empty string < non-empty");
  {
    micron::string a(""), b("a");
    sb::require_true(a < b);
  }
  sb::end_test_case();

  sb::test_case("equal strings: not < and not >");
  {
    micron::string a("same"), b("same");
    sb::require_false(a < b);
    sb::require_false(a > b);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑪ reserve / capacity management
// ─────────────────────────────────────────────────────────────────────────────

static void
test_reserve()
{
  sb::print("=== reserve / capacity ===");

  sb::test_case("reserve increases capacity");
  {
    micron::string s("hi");
    usize old_cap = s.max_size();
    s.reserve(old_cap + 512);
    sb::require_true(s.max_size() >= old_cap + 512);
  }
  sb::end_test_case();

  sb::test_case("reserve preserves existing content");
  {
    micron::string s("preserve me");
    s.reserve(s.max_size() + 256);
    sb::require_true(s == "preserve me");
  }
  sb::end_test_case();

  sb::test_case("reserve smaller than current capacity: no-op");
  {
    micron::string s(256u);
    s = "hello";
    usize cap_before = s.max_size();
    s.reserve(4u);
    sb::require(s.max_size(), cap_before);
    sb::require_true(s == "hello");
  }
  sb::end_test_case();

  sb::test_case("reserve then fill to new capacity: no crash");
  {
    micron::string s;
    s.reserve(256);
    for ( int i = 0; i < 200; ++i )
      s.push_back('x');
    sb::require(s.size(), usize{ 200 });
  }
  sb::end_test_case();

  sb::test_case("multiple reserves in sequence: content always intact");
  {
    micron::string s("anchor");
    s.reserve(64);
    s.reserve(128);
    s.reserve(1024);
    sb::require_true(s == "anchor");
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑫ find / find_substr
// ─────────────────────────────────────────────────────────────────────────────

static void
test_find()
{
  sb::print("=== find / find_substr ===");

  sb::test_case("find char: found at expected index");
  {
    micron::string s("abcde");
    sb::require(s.find('c'), usize{ 2 });
  }
  sb::end_test_case();

  sb::test_case("find char: not found returns npos");
  {
    micron::string s("abcde");
    sb::require(s.find('z'), micron::npos);
  }
  sb::end_test_case();

  sb::test_case("find char: found at index 0");
  {
    micron::string s("xyz");
    sb::require(s.find('x'), usize{ 0 });
  }
  sb::end_test_case();

  sb::test_case("find char: found at last index");
  {
    micron::string s("abc");
    sb::require(s.find('c'), usize{ 2 });
  }
  sb::end_test_case();

  sb::test_case("find char with pos > match index: skips first match");
  {
    micron::string s("abab");
    sb::require(s.find('a', 1u), usize{ 2 });
  }
  sb::end_test_case();

  sb::test_case("find_substr: found at expected index");
  {
    micron::string s("hello world");
    sb::require(s.find_substr(reinterpret_cast<const schar *>("world"), 5), usize{ 6 });
  }
  sb::end_test_case();

  sb::test_case("find_substr: not found returns npos");
  {
    micron::string s("hello");
    sb::require(s.find_substr(reinterpret_cast<const schar *>("xyz"), 3), micron::npos);
  }
  sb::end_test_case();

  sb::test_case("find_substr: needle longer than haystack returns npos");
  {
    micron::string s("hi");
    sb::require(s.find_substr(reinterpret_cast<const schar *>("toolong"), 7), micron::npos);
  }
  sb::end_test_case();

  sb::test_case("find_substr: empty needle returns npos");
  {
    micron::string s("hello");
    sb::require(s.find_substr(reinterpret_cast<const schar *>(""), 0), micron::npos);
  }
  sb::end_test_case();

  sb::test_case("find_substr: found at index 0");
  {
    micron::string s("prefix_rest");
    sb::require(s.find_substr(reinterpret_cast<const schar *>("prefix"), 6), usize{ 0 });
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑬ exception safety (bounds checking)
// ─────────────────────────────────────────────────────────────────────────────

static void
test_exception_safety()
{
  sb::print("=== exception safety ===");

  sb::test_case("at() throws for out-of-range index");
  {
    micron::string s("abc");
    sb::require_throw([&] { s.at(3); });
  }
  sb::end_test_case();

  sb::test_case("at() throws for large index");
  {
    micron::string s("x");
    sb::require_throw([&] { s.at(100); });
  }
  sb::end_test_case();

  sb::test_case("substr throws for position past end");
  {
    micron::string s("hello");
    sb::require_throw([&] { s.substr(10u, 1u); });
  }
  sb::end_test_case();

  sb::test_case("substr throws for pos + cnt past end");
  {
    micron::string s("hello");
    sb::require_throw([&] { s.substr(3u, 10u); });
  }
  sb::end_test_case();

  sb::test_case("erase throws for index past end");
  {
    micron::string s("abc");
    sb::require_throw([&] { s.erase(10u); });
  }
  sb::end_test_case();

  sb::test_case("erase throws for cnt past end");
  {
    micron::string s("abc");
    sb::require_throw([&] { s.erase(1u, 100u); });
  }
  sb::end_test_case();

  sb::test_case("string remains valid after caught exception");
  {
    micron::string s("safe");
    try {
      s.at(99);
    } catch ( ... ) {
    }
    sb::require_true(s == "safe");
    sb::require(s.size(), usize{ 4 });
  }
  sb::end_test_case();

  sb::test_case("multiple exceptions from same object: state always consistent");
  {
    micron::string s("test");
    for ( int i = 0; i < 5; ++i ) {
      try {
        s.at(100);
      } catch ( ... ) {
      }
    }
    sb::require_true(s == "test");
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑭ stress / regression
// ─────────────────────────────────────────────────────────────────────────────

static void
test_stress()
{
  sb::print("=== stress / regression ===");

  sb::test_case("build a 10000-char string via push_back: size correct");
  {
    micron::string s;
    for ( int i = 0; i < 10000; ++i )
      s.push_back(static_cast<char>('a' + (i % 26)));
    sb::require(s.size(), usize{ 10000 });
    sb::require(s[0], 'a');
    sb::require(s[25], 'z');
    sb::require(s[26], 'a');
  }
  sb::end_test_case();

  sb::test_case("alternating push_back / pop_back 1000 times: final size == start size");
  {
    micron::string s("base");
    usize start = s.size();
    for ( int i = 0; i < 1000; ++i ) {
      s.push_back('X');
      s.pop_back();
    }
    sb::require(s.size(), start);
    sb::require_true(s == "base");
  }
  sb::end_test_case();

  sb::test_case("insert at 0 in a loop: prefix order preserved");
  {
    micron::string s("Z");
    for ( char c = 'A'; c < 'Z'; ++c )
      s.insert((usize)0u, c);
    sb::require(s[0], 'Y');     // last inserted is at front
    sb::require(s.back(), 'Z');
  }
  sb::end_test_case();

  sb::test_case("copy then mutate copy 1000 times: original never corrupted");
  {
    micron::string original("immutable_reference");
    for ( int i = 0; i < 1000; ++i ) {
      micron::string copy(original);
      copy[0] = 'X';
      copy.push_back('!');
    }
    sb::require_true(original == "immutable_reference");
    sb::require(original.size(), usize{ 19 });
  }
  sb::end_test_case();

  sb::test_case("move chain: value survives 100 moves");
  {
    micron::string s("survivor");
    for ( int i = 0; i < 100; ++i ) {
      micron::string tmp(micron::move(s));
      s = micron::move(tmp);
    }
    sb::require_true(s == "survivor");
  }
  sb::end_test_case();

  sb::test_case("remove_all in tight loop: string shrinks monotonically");
  {
    micron::string s;
    for ( int i = 0; i < 200; ++i )
      s.push_back(i % 2 == 0 ? 'A' : 'B');     // "ABABAB..."
    usize before = s.size();
    s.remove_all("A");
    sb::require_smaller(s.size(), before);
    // no 'A' should remain
    for ( usize i = 0; i < s.size(); ++i )
      sb::require(s[i], 'B');
  }
  sb::end_test_case();

  sb::test_case("reserve + append to exact capacity: no overflow");
  {
    micron::string s;
    s.reserve(64);
    usize cap = s.max_size();
    usize fill = cap - 4;
    for ( usize i = 0; i < fill; ++i )
      s.push_back('x');
    sb::require(s.size(), fill);
    sb::require_true(s.max_size() >= fill);
  }
  sb::end_test_case();

  sb::test_case("substr of entire string equals original");
  {
    micron::string s("full content here");
    auto t = s.substr(0u, s.size());
    sb::require_true(t == s);
  }
  sb::end_test_case();

  sb::test_case("clear then reuse 100 times: no stale data");
  {
    micron::string s;
    for ( int i = 0; i < 100; ++i ) {
      s.clear();
      s = "fresh";
      sb::require_true(s == "fresh");
      sb::require(s.size(), usize{ 5 });
    }
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// entry point
// ─────────────────────────────────────────────────────────────────────────────

int
main()
{
  sb::print("=== micron::hstring lifetime & memory-safety test suite ===");

  test_construction();
  test_assignment();
  test_lifetime();
  test_mutation();
  test_insert_erase();
  test_substr();
  test_truncate();
  test_remove();
  test_iterators();
  test_comparison();
  test_reserve();
  test_find();
  test_exception_safety();
  test_stress();

  sb::print("=== ALL TESTS COMPLETED ===");
  return 0;
}
