//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// The atomic tier: atomic<T> (lock-guarded, arbitrary T), atomic_token<T> (lock-free primitive),
// and atomic_ptr<T>.
//
// Two API shapes that are easy to get wrong and are pinned here:
//
//   atomic<T>::get() LOCKS and returns T*; the caller MUST call release(). It is not a reader.
//   atomic_ptr<T> takes the WHOLE pointer type -- atomic_ptr<int*>::P is int*, not int** -- so
//   there is no integral assignment and `ptr = 0x50` does not compile.
#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/control.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/sync/channel.hpp"
#include "../../src/thread/thread.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

constexpr static const u32 N_THREADS = 4;
constexpr static const u32 N_BUMPS = 20000;

static micron::atomic_token<u64> g_counter{ 0 };

static void
bump(void)
{
  for ( u32 i = 0; i < N_BUMPS; ++i ) g_counter.fetch_add(1, micron::memory_order_acq_rel);
}

int
main(void)
{
  test_case("atomic<T>: assignment, comparison, and the locked get()/release() pair");
  {
    mc::atomic<char> a;
    a = 10;
    require_true(a == static_cast<char>(10));

    char *p = a.get();      // takes the lock
    require_true(p != nullptr);
    require_true(*p == 10);
    *p = 20;
    a.release();            // NOTE: get() without release() leaks the lock

    require_true(a == static_cast<char>(20));
    require_true(a != static_cast<char>(10));

    require_true(a += static_cast<char>(5));
    require_true(a == static_cast<char>(25));
  }
  end_test_case();

  test_case("atomic_ptr<T> stores the whole pointer type");
  {
    mc::atomic_ptr<int *> ptr;
    require_true(ptr.get() == nullptr);
    require_true(!static_cast<bool>(ptr));

    // the parameter type is int*, so an integral literal has to be spelled as one
    int *const fake = reinterpret_cast<int *>(0x50);
    ptr = fake;
    require_true(ptr.get() == fake);
    require_true(ptr == fake);
    require_true(static_cast<bool>(ptr));

    int real = 7;
    int *prev = ptr.exchange(&real);
    require_true(prev == fake);
    require_true(*ptr == 7);

    int *expected = &real;
    require_true(ptr.compare_exchange_strong(expected, nullptr));
    require_true(ptr.get() == nullptr);

    expected = &real;      // no longer the stored value
    require_true(!ptr.compare_exchange_strong(expected, &real));
    require_true(expected == nullptr);      // the failed CAS wrote back what was there
  }
  end_test_case();

  test_case("atomic_token<u64> counts correctly under contention");
  {
    g_counter.store(0, micron::memory_order_relaxed);
    {
      micron::auto_thread<> th[N_THREADS] = { micron::auto_thread<>(bump), micron::auto_thread<>(bump), micron::auto_thread<>(bump),
                                              micron::auto_thread<>(bump) };
      (void)th;
    }      // auto_thread joins in its destructor
    require_true(g_counter.get(micron::memory_order_acquire) == static_cast<u64>(N_THREADS) * N_BUMPS);
  }
  end_test_case();

  sb::print("=== ALL ATOMICS TESTS PASSED ===");
  return 1;
}
