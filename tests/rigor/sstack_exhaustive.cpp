//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Rigor for the in-object static stacks micron::sstack<T,N> (checked) and
// micron::fsstack<T,N> (fast/unchecked). The headline target is the carray-class
// double-construct / double-destroy bug (now killed via union storage): every
// suite runs a lifetime-balance (tracked) and an ASan owning-type gate. Also
// covers the count-ctor 2n/OOB bug, self-assign data-loss, length clamp, the
// renamed rvalue push, and move-only support.

#include "../../src/stacks/sstack.hpp"
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"

#include "../snowball/snowball.hpp"
#include "../support/stack_rigor.hpp"

#include <vector>

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_throw;
using sb::test_case;

using stest::move_only;
using stest::owner;
using stest::splitmix64;
using stest::tracked;

namespace
{

template<class Stack>
bool
stress(int rounds, u64 seed)
{
  u64 rng = seed;
  for ( int r = 0; r < rounds; ++r ) {
    Stack s;      // N == 64 for the types we pass
    std::vector<int> oracle;
    const int ops = 300;
    for ( int i = 0; i < ops; ++i ) {
      const u64 roll = splitmix64(rng);
      const bool can_push = oracle.size() < 60;
      if ( (can_push && (roll & 3) != 0) || oracle.empty() ) {
        const int v = static_cast<int>(splitmix64(rng) & 0xffff);
        if ( oracle.size() >= 64 ) continue;
        s.push(v);
        oracle.push_back(v);
      } else {
        s.pop();
        oracle.pop_back();
      }
      if ( s.size() != oracle.size() ) return false;
      if ( !oracle.empty() && static_cast<int>(s.top()) != oracle.back() ) return false;
    }
  }
  return true;
}

}      // namespace

int
main(void)
{
  print("=== SSTACK / FSTACK EXHAUSTIVE ===");

  // ── micron::sstack (checked) ──────────────────────────────────────────────
  test_case("sstack: basic LIFO + oracle stress");
  {
    micron::sstack<int, 64> s;
    s.push(1);
    s.push(2);
    s.push(3);
    require(s.size() == 3ULL);
    require(static_cast<int>(s.top()) == 3);
    require(static_cast<int>(s.operator()()) == 3);      // top+pop
    require(s.size() == 2ULL);
    require(stress<micron::sstack<int, 64>>(50, 0x515AC0ULL));
  }
  end_test_case();

  test_case("sstack: throws on overflow / empty pop / OOB index");
  {
    micron::sstack<int, 4> s;
    s.push(1);
    s.push(2);
    s.push(3);
    s.push(4);
    require_throw([&] { s.push(5); });      // overflow
    micron::sstack<int, 4> e;
    require_throw([&] { e.pop(); });      // empty pop
    require_throw([&] { (void)e.top(); });
    require_throw([&] { (void)s[99]; });      // OOB index
  }
  end_test_case();

  test_case("sstack: NO double-construct/destroy (lifetime balanced)");
  {
    tracked::reset();
    {
      micron::sstack<tracked, 16> s;
      for ( int i = 0; i < 10; ++i ) s.emplace(i);
      require(s.size() == 10ULL);
      require(tracked::live == 10);           // exactly 10, not 16 (no value-init of the array)
      micron::sstack<tracked, 16> cp(s);      // copy
      micron::sstack<tracked, 16> mv(micron::move(cp));
      for ( int i = 0; i < 3; ++i ) s.pop();
    }
    require(tracked::balanced());      // ctor==dtor, live==0
  }
  end_test_case();

  test_case("sstack: count ctor builds exactly n (not 2n) + no OOB");
  {
    tracked::reset();
    {
      micron::sstack<tracked, 16> s(10);
      require(s.size() == 10ULL);      // was 20 before the fix
      require(tracked::live == 10);
    }
    require(tracked::balanced());
    // n > N must throw, not overflow the in-object array
    require_throw([] { micron::sstack<int, 8> bad(12); });
  }
  end_test_case();

  test_case("sstack: self copy/move-assign preserves contents (no data-loss)");
  {
    micron::sstack<int, 8> s{ 1, 2, 3 };
    s = s;      // must NOT clear-then-read-empty
    require(s.size() == 3ULL && static_cast<int>(s.top()) == 3);
    s = micron::move(s);
    require(s.size() == 3ULL && static_cast<int>(s.top()) == 3);
  }
  end_test_case();

  test_case("sstack: rvalue push actually moves; move-only supported");
  {
    micron::sstack<move_only, 8> s;
    s.push(move_only(1));
    s.push(move_only(2));      // binds push(T&&), not a copy
    require(s.size() == 2ULL && s.top().v == 2);
    micron::sstack<move_only, 8> b(micron::move(s));
    require(b.size() == 2ULL);
    micron::sstack<move_only, 8> c;
    c.swap(b);      // move-only swap
    require(c.size() == 2ULL && b.size() == 0ULL);
  }
  end_test_case();

  test_case("sstack: owning type, balanced (ASan double-free/leak gate)");
  {
    owner::reset();
    {
      micron::sstack<owner, 32> s;
      for ( int i = 0; i < 20; ++i ) s.emplace(i);
      micron::sstack<owner, 32> cp(s);
      cp = s;
      cp = cp;
      micron::sstack<owner, 32> mv(micron::move(cp));
      micron::sstack<owner, 32> sw;
      sw.swap(mv);
      for ( int i = 0; i < 5; ++i ) s.pop();
    }
    require(owner::live == 0);
  }
  end_test_case();

  // ── micron::fsstack (fast / unchecked) ────────────────────────────────────
  test_case("fsstack: union storage, lifetime balanced (no double free)");
  {
    tracked::reset();
    {
      micron::fsstack<tracked, 16> s;
      for ( int i = 0; i < 8; ++i ) s.push(tracked(i));
      require(s.size() == 8ULL);
      require(tracked::live == 8);
      micron::fsstack<tracked, 16> cp(s);
      micron::fsstack<tracked, 16> mv(micron::move(cp));
      while ( !s.empty() ) s.pop();
    }
    require(tracked::balanced());
  }
  end_test_case();

  test_case("fsstack: oracle stress + owning ASan gate");
  {
    require(stress<micron::fsstack<int, 64>>(50, 0xF00DULL));
    owner::reset();
    {
      micron::fsstack<owner, 32> s;
      for ( int i = 0; i < 20; ++i ) s.emplace(i);
      micron::fsstack<owner, 32> cp(s);
      cp = s;
      micron::fsstack<owner, 32> mv(micron::move(cp));
      while ( !s.empty() ) s.pop();
    }
    require(owner::live == 0);
  }
  end_test_case();

  print("=== ALL TESTS PASSED ===");
  return 1;
}
