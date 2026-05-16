//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// arm32 portability tests for micron::crossbeam.
//
// the cross-toolchain headers for armv7-a/linaro clash with std::thread,
// std::atomic, and std::mutex; this file therefore uses only micron
// primitives + snowball, and exercises the queue from a single thread.
// the goal is to validate the portability fixes, not to re-run the MPMC
// stress (which the x86 crossbeam.cpp test already covers):
//
//   1.  __next_pow2 must compile under sizeof(usize) == 4 (no n >> 32 UB).
//   2.  cache-line padding must be valid (no negative-array UB) even if
//       sizeof(atomic_token<usize>) is small (4 bytes on arm32).
//   3.  __tail / __head sit on distinct cache lines.
//   4.  push/pop unify their lifetime trait so non-trivially-copyable T
//       round-trips with balanced ctor/dtor counts.
//   5.  capacity stays power-of-two and >=N for assorted N.

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/memory/cache.hpp"
#include "../../src/queue/crossbeam.hpp"
#include "../../src/std.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"

namespace
{

struct Tracked {
  static inline usize ctor = 0;
  static inline usize dtor = 0;
  int v;

  Tracked() : v(0) { ++ctor; }

  explicit Tracked(int x) : v(x) { ++ctor; }

  Tracked(const Tracked &o) : v(o.v) { ++ctor; }

  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = 0;
    ++ctor;
  }

  Tracked &
  operator=(const Tracked &o)
  {
    v = o.v;
    return *this;
  }

  Tracked &
  operator=(Tracked &&o) noexcept
  {
    v = o.v;
    o.v = 0;
    return *this;
  }

  ~Tracked() { ++dtor; }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }
};

void
reset_tracked()
{
  Tracked::ctor = 0;
  Tracked::dtor = 0;
}

static_assert(sizeof(micron::crossbeam<int, 16>) > 0, "crossbeam must instantiate on this arch");
static_assert(alignof(micron::crossbeam<int, 16>) >= micron::cache_line_size(),
              "crossbeam must be cache-line aligned for false-sharing avoidance");
static_assert(sizeof(micron::crossbeam<int, 256>) >= 2 * micron::cache_line_size(), "head and tail must sit in separate cache lines");

};      // namespace

int
main(void)
{
  sb::print("=== CROSSBEAM ARM32 PORTABILITY TESTS ===");

  sb::test_case("capacity rounds up to next power of two");
  {
    micron::crossbeam<int, 1> q1;
    sb::require(q1.capacity() == 1ULL);
    micron::crossbeam<int, 2> q2;
    sb::require(q2.capacity() == 2ULL);
    micron::crossbeam<int, 5> q5;
    sb::require(q5.capacity() == 8ULL);

    micron::crossbeam<int, 200> q256;
    sb::require(q256.capacity() == 256ULL);
  }
  sb::end_test_case();

  sb::test_case("class size is a multiple of cache line (no negative pad)");
  {

    constexpr usize cl = micron::cache_line_size();
    sb::require(sizeof(micron::crossbeam<int, 16>) >= 2 * cl);
    sb::require(sizeof(micron::crossbeam<long long, 16>) >= 2 * cl);
  }
  sb::end_test_case();

  sb::test_case("construction - empty");
  {
    micron::crossbeam<int, 16> q;
    sb::require(q.empty());
    sb::require(q.size() == 0ULL);
    sb::require(q.capacity() == 16ULL);
  }
  sb::end_test_case();

  sb::test_case("push then pop");
  {
    micron::crossbeam<int, 16> q;
    sb::require(q.push(42));
    int v = 0;
    sb::require(q.pop(v));
    sb::require(v == 42);
    sb::require(q.empty());
  }
  sb::end_test_case();

  sb::test_case("push full returns false");
  {
    micron::crossbeam<int, 4> q;
    sb::require(q.push(1));
    sb::require(q.push(2));
    sb::require(q.push(3));
    sb::require(q.push(4));
    sb::require(!q.push(5));
  }
  sb::end_test_case();

  sb::test_case("pop empty returns false");
  {
    micron::crossbeam<int, 16> q;
    int v = 0;
    sb::require(!q.pop(v));
  }
  sb::end_test_case();

  sb::test_case("FIFO order preserved");
  {
    micron::crossbeam<int, 64> q;
    for ( int i = 0; i < 20; ++i ) sb::require(q.push(i));
    for ( int i = 0; i < 20; ++i ) {
      int v = -1;
      sb::require(q.pop(v));
      sb::require(v == i);
    }
  }
  sb::end_test_case();

  sb::test_case("wrap-around at small capacity");
  {
    micron::crossbeam<int, 4> q;

    for ( int round = 0; round < 100; ++round ) {
      sb::require(q.push(round));
      int v = 0;
      sb::require(q.pop(v));
      sb::require(v == round);
    }
  }
  sb::end_test_case();

  sb::test_case("non-trivially-copyable T: push/pop balances ctor/dtor");
  {
    reset_tracked();
    {
      micron::crossbeam<Tracked, 8> q;

      for ( int i = 0; i < 6; ++i ) sb::require(q.push(Tracked{ i }));

      for ( int i = 0; i < 6; ++i ) {
        Tracked out;
        sb::require(q.pop(out));
        sb::require(out.v == i);
      }
      sb::require(q.empty());
    }

    sb::require(Tracked::ctor == Tracked::dtor);
  }
  sb::end_test_case();

  sb::test_case("non-trivially-copyable T: queue dtor cleans pending slots");
  {
    reset_tracked();
    {
      micron::crossbeam<Tracked, 8> q;

      for ( int i = 0; i < 4; ++i ) sb::require(q.push(Tracked{ i * 7 }));
    }
    sb::require(Tracked::ctor == Tracked::dtor);
  }
  sb::end_test_case();

  sb::test_case("emplace constructs in-place; pop destructs");
  {
    reset_tracked();
    {
      micron::crossbeam<Tracked, 8> q;
      sb::require(q.emplace(123));
      sb::require(q.emplace(456));
      Tracked out;
      sb::require(q.pop(out));
      sb::require(out.v == 123);
      sb::require(q.pop(out));
      sb::require(out.v == 456);
    }
    sb::require(Tracked::ctor == Tracked::dtor);
  }
  sb::end_test_case();

  sb::test_case("long sequential stress with int");
  {
    micron::crossbeam<int, 16> q;
    long long expected_sum = 0;
    long long got_sum = 0;

    for ( int burst = 0; burst < 500; ++burst ) {
      for ( int i = 0; i < 8; ++i ) {
        const int v = burst * 8 + i;
        sb::require(q.push(v));
        expected_sum += v;
      }
      for ( int i = 0; i < 8; ++i ) {
        int v = 0;
        sb::require(q.pop(v));
        got_sum += v;
      }
    }
    sb::require(expected_sum == got_sum);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
