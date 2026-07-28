// Copyright (c) 2024- David Lucius Severus
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"
#include "../../src/string/format.hpp"
#include "../../src/string/strings.hpp"
#include "../snowball/snowball.hpp"

using namespace snowball;

namespace fmt = micron::format;

static bool
terminated(const micron::string &s)
{
  return s.max_size() > s.size() && s.c_str()[s.size()] == '\0' && micron::strlen(s.c_str()) == s.size();
}

static void
test_no_slack_import()
{
  sb::print("=== assignment does not import the source's slack ===");

  sb::test_case("fat-but-short source does not inflate the destination");
  {
    micron::string a;
    a.reserve(1u << 20);
    a = "hi";
    sb::require_true(a.max_size() >= (1u << 20));

    micron::string b;
    usize fresh = b.max_size();
    b = a;
    sb::require(b.size(), usize{ 2 });
    sb::require_true(b == a);
    sb::require_true(terminated(b));
    sb::require_true(b.max_size() == fresh);
    sb::require_true(b.max_size() < (1u << 16));
  }
  sb::end_test_case();

  sb::test_case("slack does not compound down an assignment chain");
  {
    micron::string src;
    src.reserve(1u << 20);
    src = "seed";

    micron::string chain[10];
    usize fresh = chain[0].max_size();
    chain[0] = src;
    for ( int i = 1; i < 10; ++i ) chain[i] = chain[i - 1];

    for ( int i = 0; i < 10; ++i ) {
      sb::require(chain[i].size(), usize{ 4 });
      sb::require_true(chain[i] == src);
      sb::require_true(terminated(chain[i]));
      sb::require_true(chain[i].max_size() == fresh);
    }
  }
  sb::end_test_case();

  sb::test_case("destination keeps its OWN high-water mark (grow-only, never shrinks)");
  {
    micron::string b;
    b.reserve(1u << 16);
    usize reserved = b.max_size();
    micron::string a("tiny");
    b = a;
    sb::require(b.size(), usize{ 4 });
    sb::require_true(b.max_size() == reserved);
    sb::require_true(terminated(b));
  }
  sb::end_test_case();

  sb::test_case("assignment still grows when the source is genuinely longer");
  {
    micron::string a;
    for ( int i = 0; i < 5000; ++i ) a.push_back('x');
    micron::string b("small");
    b = a;
    sb::require(b.size(), usize{ 5000 });
    sb::require_true(b == a);
    sb::require_true(terminated(b));
    sb::require_true(b.max_size() > 5000);
  }
  sb::end_test_case();
}

static void
test_assign_invariants()
{
  sb::print("=== assignment invariants ===");

  sb::test_case("self-assignment is a no-op");
  {
    micron::string a("self assignment target");
    usize cap = a.max_size();
    a = a;
    sb::require(a.size(), usize{ 22 });
    sb::require_true(a == "self assignment target");
    sb::require_true(a.max_size() == cap);
    sb::require_true(terminated(a));
  }
  sb::end_test_case();

  sb::test_case("long -> short leaves no stale tail visible past the new length");
  {
    micron::string a("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    micron::string b("bb");
    a = b;
    sb::require(a.size(), usize{ 2 });
    sb::require_true(a == "bb");
    sb::require_true(terminated(a));

    for ( usize i = a.size(); i < 32; ++i ) sb::require_true(a.data()[i] == '\0');
  }
  sb::end_test_case();

  sb::test_case("empty source assigns cleanly");
  {
    micron::string a("nonempty");
    micron::string e;
    a = e;
    sb::require(a.size(), usize{ 0 });
    sb::require_true(a.empty());
    sb::require_true(terminated(a));
  }
  sb::end_test_case();

  sb::test_case("repeated reassignment stays stable");
  {
    micron::string a;
    for ( int i = 0; i < 200; ++i ) {
      micron::string t(i % 2 ? "alternating value one" : "two");
      a = t;
      sb::require_true(a == t);
      sb::require_true(terminated(a));
    }
  }
  sb::end_test_case();

  sb::test_case("move assignment still transfers the buffer whole");
  {
    micron::string a;
    a.reserve(1u << 16);
    a = "moved";
    usize cap = a.max_size();
    micron::string b;
    b = micron::move(a);
    sb::require(b.size(), usize{ 5 });
    sb::require_true(b == "moved");
    sb::require_true(b.max_size() == cap);
  }
  sb::end_test_case();
}

static void
test_sibling_operators()
{
  sb::print("=== cross-type assignment operators ===");

  sb::test_case("from sstring<N>: NUL slot is reserved and written");
  {
    micron::sstr<64> s("from a stack string");
    micron::string a("PRIOR CONTENT THAT IS MUCH LONGER THAN THE SOURCE");
    a = s;
    sb::require(a.size(), usize{ 19 });
    sb::require_true(a == "from a stack string");
    sb::require_true(terminated(a));
  }
  sb::end_test_case();

  sb::test_case("from sstring<N> into a fresh destination");
  {
    micron::sstr<32> s("stack");
    micron::string a;
    a = s;
    sb::require(a.size(), usize{ 5 });
    sb::require_true(a == "stack");
    sb::require_true(terminated(a));
  }
  sb::end_test_case();

  sb::test_case("from an OVERSIZED char buffer: copies to the NUL, not the extent");
  {
    char buf[64] = { 0 };
    buf[0] = 's';
    buf[1] = 'h';
    buf[2] = 'o';
    buf[3] = 'r';
    buf[4] = 't';
    micron::string a("PRIOR");
    a = buf;
    sb::require(a.size(), usize{ 5 });
    sb::require_true(a == "short");
    sb::require_true(terminated(a));
  }
  sb::end_test_case();

  sb::test_case("from a tight string literal");
  {
    micron::string a;
    a = "literal";
    sb::require(a.size(), usize{ 7 });
    sb::require_true(a == "literal");
    sb::require_true(terminated(a));
  }
  sb::end_test_case();

  sb::test_case("literal assignment does not import slack either");
  {
    micron::string a;
    usize fresh = a.max_size();
    a = "tiny";
    sb::require_true(a.max_size() == fresh);
    sb::require_true(terminated(a));
  }
  sb::end_test_case();
}

static void
test_replace_n_guard()
{
  sb::print("=== replace_n bounds guard ===");

  sb::test_case("replace_n with a shorter replacement is unaffected");
  {
    micron::string s("aXbXcXd");
    auto r = fmt::replace_n(s, "X", "", 2);
    sb::require_true(r == "abcXd");
    sb::require_true(terminated(r));
  }
  sb::end_test_case();

  sb::test_case("replace_n honours the count");
  {
    micron::string s("a.b.c.d");
    auto r = fmt::replace_n(s, ".", "--", 2);
    sb::require_true(r == "a--b--c.d");
    sb::require_true(terminated(r));
  }
  sb::end_test_case();

  sb::test_case("replace_n growing within the buffer stays well-formed");
  {
    micron::string s("a-b-c");
    auto r = fmt::replace_n(s, "-", "###", 2);
    sb::require_true(r == "a###b###c");
    sb::require_true(terminated(r));
  }
  sb::end_test_case();

  sb::test_case("replace_n growing past the buffer throws instead of corrupting the heap");
  {

    if constexpr ( micron::except::__use_exceptions ) {
      micron::string s;
      while ( s.size() + 8 < s.max_size() ) s.push_back('q');
      s.push_back('Z');
      usize cap = s.max_size();

      usize target = micron::string(s).max_size();
      micron::string wide;
      while ( wide.size() < target - s.size() + 8 ) wide.push_back('X');

      bool threw = false;
      try {
        auto r = fmt::replace_n(s, "Z", wide.c_str(), 1);
        sb::require_true(r.size() < r.max_size());
        sb::require_true(terminated(r));
      } catch ( ... ) {
        threw = true;
      }
      sb::require_true(threw);
      sb::require_true(s.max_size() == cap);
      sb::require_true(terminated(s));
    }
  }
  sb::end_test_case();
}

int
main()
{
  sb::print("=== STRING ASSIGNMENT SLACK ===");
  test_no_slack_import();
  test_assign_invariants();
  test_sibling_operators();
  test_replace_n_guard();
  sb::print("=== STRING ASSIGNMENT SLACK PASSED ===");
  return 1;
}
