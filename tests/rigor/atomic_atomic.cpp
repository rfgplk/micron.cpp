//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Coverage notes:
//   * atomic_token<T>: full surface.
//   * atomic<T>: operator overloads that go through lock_check()/unlock() —
//     the public __store/__get/compare_exchange_* methods are broken
//     (they operate on the lock token, not the wrapped value); not tested
//     here. Documented in the bonus findings report.
//   * atomic_ptr<T>: full surface.
//   * atomic_ref<T>: only `load()` is sound; store/exchange/CAS create a
//     temporary atomic_token from *ptr and operate on the copy, so they're
//     no-ops on the referenced storage. Not tested here; documented bug.

#include "../../src/atomic/atomic.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#include "../support/mt.hpp"      // mtest::parallel (micron auto_thread; NOT <thread>)
#include <vector>

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

struct BigPod {
  ::i64 a, b, c, d;
};

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== ATOMIC<T> + ATOMIC_TOKEN<T> + ATOMIC_PTR TESTS ===");

  // ── atomic_token: construction ──────────────────────────────────────────

  test_case("atomic_token<i32> default construction reads ATOMIC_OPEN (0)");
  {
    atomic_token<i32> t;
    require(t.get() == (i32)0);
  }
  end_test_case();

  test_case("atomic_token<bool>(true) reads true");
  {
    atomic_token<bool> t(true);
    require_true(t.get());
  }
  end_test_case();

  test_case("atomic_token<u32>(0xAB) reads 0xAB");
  {
    atomic_token<u32> t(0xABu);
    require(t.get() == (u32)0xABu);
  }
  end_test_case();

  // ── atomic_token: store / get with each memory order ───────────────────

  test_case("atomic_token store/get with explicit acquire/release pair");
  {
    atomic_token<u32> t(0);
    t.store(42, memory_order::release);
    require(t.get(memory_order::acquire) == (u32)42);
  }
  end_test_case();

  // ── atomic_token: copy / move ──────────────────────────────────────────

  test_case("atomic_token copy constructor snapshots value");
  {
    atomic_token<u32> a(7);
    atomic_token<u32> b(a);
    require(b.get() == (u32)7);
  }
  end_test_case();

  test_case("atomic_token move constructor zeros source");
  {
    atomic_token<u32> a(99);
    atomic_token<u32> b(micron::move(a));
    require(b.get() == (u32)99);
    require(a.get() == (u32)0);
  }
  end_test_case();

  // ── atomic_token: arithmetic ───────────────────────────────────────────

  test_case("atomic_token fetch_add returns old, increments");
  {
    atomic_token<u32> t(10);
    u32 old = t.fetch_add(5, memory_order::seq_cst);
    require(old == (u32)10);
    require(t.get() == (u32)15);
  }
  end_test_case();

  test_case("atomic_token fetch_sub returns old, decrements");
  {
    atomic_token<u32> t(10);
    u32 old = t.fetch_sub(3, memory_order::seq_cst);
    require(old == (u32)10);
    require(t.get() == (u32)7);
  }
  end_test_case();

  test_case("atomic_token add_fetch / sub_fetch return new");
  {
    atomic_token<u32> t(10);
    require(t.add_fetch(5, memory_order::seq_cst) == (u32)15);
    require(t.sub_fetch(7, memory_order::seq_cst) == (u32)8);
  }
  end_test_case();

  test_case("atomic_token operator++ / operator-- (pre-form) returns new");
  {
    // NOTE: operator++/-- on atomic_token<T> only compiles for T = int because
    // the literal `1` is `int`; passing it through atom::add_fetch<T> with
    // T = u32 deduces conflicting types. Documented bonus bug.
    atomic_token<int> t(5);
    require(++t == 6);
    require(--t == 5);
  }
  end_test_case();

  // ── atomic_token: CAS ──────────────────────────────────────────────────

  test_case("atomic_token compare_and_swap success/failure");
  {
    atomic_token<u32> t(10);
    require_true(t.compare_and_swap(10, 20));
    require(t.get() == (u32)20);
    require_false(t.compare_and_swap(99, 30));
    require(t.get() == (u32)20);
  }
  end_test_case();

  test_case("atomic_token compare_exchange_strong updates expected on failure");
  {
    atomic_token<u32> t(7);
    u32 expected = 99;
    bool ok = t.compare_exchange_strong(expected, 100, memory_order::seq_cst);
    require_false(ok);
    require(expected == (u32)7);
    require(t.get() == (u32)7);
  }
  end_test_case();

  test_case("atomic_token compare_exchange_weak in retry loop");
  {
    atomic_token<u32> t(0);
    u32 expected = 0;
    while ( !t.compare_exchange_weak(expected, expected + 1, memory_order::seq_cst, memory_order::relaxed) ) {
    }
    require(t.get() == (u32)1);
  }
  end_test_case();

  // ── atomic_token: swap ─────────────────────────────────────────────────

  test_case("atomic_token swap returns old, stores new");
  {
    atomic_token<u32> t(5);
    u32 old = t.swap(42);
    require(old == (u32)5);
    require(t.get() == (u32)42);
  }
  end_test_case();

  // ── atomic_token: comparisons ──────────────────────────────────────────

  test_case("atomic_token comparison operators against value");
  {
    atomic_token<i32> t(5);
    require_true(t == 5);
    require_false(t != 5);
    require_true(t < 6);
    require_true(t > 4);
    require_true(t <= 5);
    require_true(t >= 5);
  }
  end_test_case();

  test_case("atomic_token comparison between two tokens");
  {
    atomic_token<i32> a(5), b(5), c(6);
    require_true(a == b);
    require_true(a != c);
    require_true(a < c);
    require_true(c > a);
  }
  end_test_case();

  test_case("atomic_token operator() returns get()");
  {
    atomic_token<i32> t(11);
    require(t() == (i32)11);
  }
  end_test_case();

  test_case("atomic_token ptr() returns underlying storage address");
  {
    atomic_token<i32> t(3);
    *t.ptr() = 9;      // direct write (non-atomic)
    require(t.get() == (i32)9);
  }
  end_test_case();

  // ── atomic_token: 4-thread contention ──────────────────────────────────

  test_case("atomic_token 4-thread fetch_add stress");
  {
    atomic_token<u64> counter(0);
    constexpr int kT = 4, kIters = 10000;
    mtest::parallel(kT, [&counter](int) {
      for ( int j = 0; j < kIters; ++j ) counter.fetch_add(1, memory_order::seq_cst);
    });
    require(counter.get() == (u64)(kT * kIters));
  }
  end_test_case();

  // ── atomic<T> operator overloads (lock-based wrapper) ──────────────────

  test_case("atomic<int> default ctor zero");
  {
    atomic<int> a;
    int *p = a.get();
    require(*p == 0);
    a.release();
  }
  end_test_case();

  test_case("atomic<int> variadic ctor forwards args");
  {
    atomic<int> a(42);
    int *p = a.get();
    require(*p == 42);
    a.release();
  }
  end_test_case();

  test_case("atomic<int> operator++ / operator-- / operator+= correctness");
  {
    atomic<int> a(10);
    ++a;
    require(a == 11);
    --a;
    require(a == 10);
    a += 5;
    require(a == 15);
    a -= 3;
    require(a == 12);
  }
  end_test_case();

  test_case("atomic<int> bitwise compound assignments");
  {
    atomic<int> a(0xF0);
    a &= 0xAA;
    require(a == 0xA0);
    a |= 0x0F;
    require(a == 0xAF);
    a ^= 0xFF;
    require(a == 0x50);
  }
  end_test_case();

  test_case("atomic<int> 4-thread operator++ stress");
  {
    atomic<int> a(0);
    constexpr int kT = 4, kIters = 5000;
    mtest::parallel(kT, [&a](int) {
      for ( int j = 0; j < kIters; ++j ) ++a;
    });
    require(a == kT * kIters);
  }
  end_test_case();

  test_case("atomic<BigPod> get()/release() serialize across threads");
  {
    atomic<BigPod> p{};
    {
      auto *q = p.get();
      q->a = 0;
      q->b = 0;
      q->c = 0;
      q->d = 0;
      p.release();
    }
    constexpr int kT = 4, kIters = 1000;
    mtest::parallel(kT, [&p](int) {
      for ( int j = 0; j < kIters; ++j ) {
        auto *q = p.get();
        ++q->a;
        ++q->b;
        ++q->c;
        ++q->d;
        p.release();
      }
    });
    auto *q = p.get();
    require(q->a == (i64)(kT * kIters));
    require(q->b == (i64)(kT * kIters));
    require(q->c == (i64)(kT * kIters));
    require(q->d == (i64)(kT * kIters));
    p.release();
  }
  end_test_case();

  // ── atomic_ptr ─────────────────────────────────────────────────────────

  test_case("atomic_ptr<int*> default null");
  {
    atomic_ptr<int *> p;
    require(p.load() == nullptr);
  }
  end_test_case();

  test_case("atomic_ptr<int*> load / store / exchange");
  {
    int x = 5, y = 7;
    atomic_ptr<int *> p(&x);
    require(p.load() == &x);
    p.store(&y);
    require(p.load() == &y);
    int *old = p.exchange(&x);
    require(old == &y);
    require(p.load() == &x);
  }
  end_test_case();

  test_case("atomic_ptr<int*> compare_exchange_strong");
  {
    int x = 1, y = 2, z = 3;
    atomic_ptr<int *> p(&x);
    int *expected = &x;
    require_true(p.compare_exchange_strong(expected, &y));
    require(p.load() == &y);
    expected = &z;      // wrong
    require_false(p.compare_exchange_strong(expected, &x));
    require(expected == &y);
  }
  end_test_case();

  test_case("atomic_ptr<char*> pointer arithmetic (+, -, ++, --, +=)");
  {
    char buf[16] = { 0 };
    atomic_ptr<char *> p(buf);
    require((p + (usize)4) == buf + 4);
    require((p - (usize)0) == buf);
    p += 2;
    require(p.load() == buf + 2);
    p -= 1;
    require(p.load() == buf + 1);
    ++p;
    require(p.load() == buf + 2);
    --p;
    require(p.load() == buf + 1);
  }
  end_test_case();

  // ── atomic_ref::load (the only sound op) ───────────────────────────────

  test_case("atomic_ref<u32> load() observes external value");
  {
    u32 raw = 42;
    atomic_ref<u32> r(raw);
    require(r.load() == (u32)42);
    raw = 99;
    require(r.load() == (u32)99);
  }
  end_test_case();

  sb::print("=== ALL ATOMIC<T> + ATOMIC_TOKEN<T> + ATOMIC_PTR TESTS PASSED ===");
  return 1;
}
