//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// a seqlock is only worth anything if a reader can never observe a HALF-written value. the payload
// here is deliberately multi-word with an internal invariant (b == -a, and a checksum over both),
// because a single word cannot tear and would make every assertion below vacuous.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/mutex/locks/seqlock.hpp"
#include "../../src/mutex/locks/ttas_lock.hpp"

#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../support/mt.hpp"

#include "../snowball/snowball.hpp"

#include "../support/lockcheck.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

constexpr u64 SEED_SEQ = 0x5E910CC0FFEEULL;

// eight words, so a torn read is both possible and detectable
struct payload {
  i64 a;
  i64 b;      // invariant: b == -a
  u64 tag;
  u64 sum;      // invariant: sum == tag * 3 + 7
  u64 pad[4];

  [[nodiscard]] bool
  consistent() const noexcept
  {
    if ( b != -a ) return false;
    if ( sum != tag * 3ull + 7ull ) return false;
    for ( int i = 0; i < 4; ++i )
      if ( pad[i] != tag ) return false;
    return true;
  }
};

payload
make(u64 t) noexcept
{
  payload p{};
  p.a = static_cast<i64>(t);
  p.b = -static_cast<i64>(t);
  p.tag = t;
  p.sum = t * 3ull + 7ull;
  for ( int i = 0; i < 4; ++i ) p.pad[i] = t;
  return p;
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== SEQLOCK TESTS ===");

  test_case("default ctor: sequence even, not writing");
  {
    seqlock<payload> s;
    require(s.sequence() == 0u);
    require_false(s.writing());
  }
  end_test_case();

  test_case("value ctor and load roundtrip");
  {
    seqlock<payload> s{ make(42) };
    const payload got = s.load();
    require(got.tag == 42ull);
    require_true(got.consistent());
  }
  end_test_case();

  test_case("store advances the sequence by two and leaves it even");
  {
    seqlock<payload> s{ make(1) };
    const u32 s0 = s.sequence();
    s.store(make(2));
    require(s.sequence() == s0 + 2u);
    require_false(s.writing());
    require(s.load().tag == 2ull);
    s.store(make(3));
    require(s.sequence() == s0 + 4u);
    require(s.load().tag == 3ull);
  }
  end_test_case();

  test_case("write(fn) mutates in place under the same window");
  {
    seqlock<payload> s{ make(10) };
    const u32 s0 = s.sequence();
    s.write([](payload &p) { p = make(11); });
    require(s.sequence() == s0 + 2u);
    require(s.load().tag == 11ull);
    require_true(s.load().consistent());
  }
  end_test_case();

  test_case("try_load succeeds on a quiet lock");
  {
    seqlock<payload> s{ make(7) };
    payload out{};
    require_true(s.try_load(out));
    require(out.tag == 7ull);
  }
  end_test_case();

  // ---- the property the type exists for ----
  test_case("readers never observe a torn value under a continuous writer");
  {
    seqlock<payload> s{ make(0) };
    atomic_token<bool> stop(false);
    atomic_token<u64> reads(0);
    atomic_token<u64> torn(0);
    atomic_token<u64> retried(0);
    const int kReaders = static_cast<int>(lcheck::wide_threads);

    mtest::parallel(kReaders + 1, [&](int tid) {
      if ( tid == kReaders ) {      // the single writer
        for ( u64 i = 1; i <= 200000ull; ++i ) s.store(make(i));
        stop.store(true, memory_order::release);
        return;
      }
      u64 n = 0;
      while ( !stop.get(memory_order::acquire) ) {
        const payload p = s.load();
        if ( !p.consistent() ) torn.fetch_add(1, memory_order::acq_rel);
        ++n;
        payload one{};
        if ( !s.try_load(one) ) retried.fetch_add(1, memory_order::acq_rel);
      }
      reads.fetch_add(n, memory_order::acq_rel);
    });

    sb::print("     reads=", static_cast<usize>(reads.get(memory_order::acquire)), " torn=",
              static_cast<usize>(torn.get(memory_order::acquire)), " try_load retries=",
              static_cast<usize>(retried.get(memory_order::acquire)));

    require(torn.get(memory_order::acquire) == 0ull);
    require_true(reads.get(memory_order::acquire) > 0ull);
    // if try_load never once collided the writer was too slow to overlap and the case proved
    // nothing about the retry path
    require_true(retried.get(memory_order::acquire) > 0ull);
    require_true(s.load().consistent());
  }
  end_test_case();

  test_case("the writer is never blocked by readers: it completes while they hammer");
  {
    seqlock<payload> s{ make(0) };
    atomic_token<bool> stop(false);
    atomic_token<bool> writer_done(false);
    const int kReaders = static_cast<int>(lcheck::over_threads);

    mtest::parallel(kReaders + 1, [&](int tid) {
      if ( tid == kReaders ) {
        for ( u64 i = 1; i <= 50000ull; ++i ) s.store(make(i));
        writer_done.store(true, memory_order::release);
        stop.store(true, memory_order::release);
        return;
      }
      while ( !stop.get(memory_order::acquire) ) {
        const payload p = s.load();
        if ( !p.consistent() ) micron::abort(6);      // fail loudly from the worker
      }
    });

    require_true(writer_done.get(memory_order::acquire));
    require(s.load().tag == 50000ull);
  }
  end_test_case();

  // ---- multiple writers need a real Lock parameter ----
  test_case("seqlock<T, ttas_lock> serialises concurrent writers");
  {
    seqlock<payload, ttas_lock> s{ make(0) };
    atomic_token<bool> stop(false);
    atomic_token<u64> torn(0);
    const int kWriters = 4;
    const int kReaders = static_cast<int>(lcheck::wide_threads);

    mtest::parallel(kWriters + kReaders, [&](int tid) {
      if ( tid < kWriters ) {
        u64 v = SEED_SEQ + static_cast<u64>(tid);
        for ( int i = 0; i < 20000; ++i ) s.store(make(lcheck::xs64(v) & 0xFFFFULL));
        if ( tid == 0 ) stop.store(true, memory_order::release);
        return;
      }
      while ( !stop.get(memory_order::acquire) ) {
        const payload p = s.load();
        if ( !p.consistent() ) torn.fetch_add(1, memory_order::acq_rel);
      }
    });

    require(torn.get(memory_order::acquire) == 0ull);
    require_false(s.writing());
    require_true(s.load().consistent());
  }
  end_test_case();

  test_case("sequence stays even at rest after heavy write traffic");
  {
    seqlock<payload> s{ make(0) };
    for ( u64 i = 1; i <= 5000ull; ++i ) s.store(make(i));
    require((s.sequence() & 1u) == 0u);
    require(s.sequence() == 10000u);
  }
  end_test_case();

  test_case("non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<seqlock<payload>>);
    static_assert(!is_move_constructible_v<seqlock<payload>>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL SEQLOCK TESTS PASSED ===");
  return 1;
}
