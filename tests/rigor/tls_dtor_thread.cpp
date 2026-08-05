//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/atomic/atomic.hpp"
#include "../../src/attributes.hpp"
#include "../../src/exit.hpp"
#include "../../src/syscall.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"
#include "../support/mt.hpp"

using namespace snowball;

namespace
{

constexpr u64 __probe_magic = 0x5EC0FFEEB0BAF00Dull;
constexpr int __workers = 6;

micron::atomic_token<u32> g_ctor{ 0 };
micron::atomic_token<u32> g_dtor{ 0 };
micron::atomic_token<u32> g_bad_magic{ 0 };
micron::atomic_token<u32> g_wrong_thread{ 0 };
micron::atomic_token<u32> g_main_dtor{ 0 };

inline i32
this_tid(void) noexcept
{
  return static_cast<i32>(micron::syscall(SYS_gettid));
}

struct probe {
  u64 magic;
  i32 owner_tid;
  bool is_main;

  explicit probe(bool main_thread) noexcept : magic(__probe_magic), owner_tid(this_tid()), is_main(main_thread)
  {
    g_ctor.fetch_add(1, micron::memory_order_acq_rel);
  }

  ~probe() noexcept
  {
    if ( magic != __probe_magic ) {
      g_bad_magic.fetch_add(1, micron::memory_order_acq_rel);
      return;
    }
    if ( owner_tid != this_tid() ) g_wrong_thread.fetch_add(1, micron::memory_order_acq_rel);
    magic = 0;
    if ( is_main )
      g_main_dtor.fetch_add(1, micron::memory_order_acq_rel);
    else
      g_dtor.fetch_add(1, micron::memory_order_acq_rel);
  }
};

[[gnu::noinline]] probe &
worker_probe(void) noexcept
{
  static thread_local probe __p{ false };
  return __p;
}

[[gnu::noinline]] probe &
main_probe(void) noexcept
{
  static thread_local probe __p{ true };
  return __p;
}

gdestructor_ void
__check_main_probe(void)
{
  const u32 n = g_main_dtor.get(micron::memory_order_acquire);
  const u32 bad = g_bad_magic.get(micron::memory_order_acquire);
  const u32 wrong = g_wrong_thread.get(micron::memory_order_acquire);
  if ( n == 1 && bad == 0 && wrong == 0 ) return;
  sb::print("FAIL: main-thread thread_local not destroyed exactly once on main");
  micron::sys_group_exit(6);
}

}      // namespace

int
main(int, char **)
{
  sb::print("=== TLS DTOR OWNERSHIP RIGOR ===");

  micron::atexit(&__check_main_probe);

  const i32 main_tid = this_tid();
  (void)main_probe();

  test_case("every worker runs its own thread_local dtor before the join returns");
  {
    mtest::parallel(__workers, [](int) {
      probe &p = worker_probe();
      require(p.magic == __probe_magic, true);
      require(p.owner_tid == this_tid(), true);
    });

    require(g_dtor.get(micron::memory_order_acquire), static_cast<u32>(__workers));
    require(g_ctor.get(micron::memory_order_acquire), static_cast<u32>(__workers + 1));
  }
  end_test_case();

  test_case("no dtor saw recycled storage or a foreign thread");
  {
    require(g_bad_magic.get(micron::memory_order_acquire), 0u);
    require(g_wrong_thread.get(micron::memory_order_acquire), 0u);
  }
  end_test_case();

  test_case("main's thread_local is still alive and still owned by main");
  {
    probe &p = main_probe();
    require(p.magic == __probe_magic, true);
    require(p.owner_tid == main_tid, true);
    require(g_main_dtor.get(micron::memory_order_acquire), 0u);
  }
  end_test_case();

  test_case("a second round reuses the frames without resurrecting a dead registration");
  {
    mtest::parallel(__workers, [](int) { (void)worker_probe(); });
    require(g_dtor.get(micron::memory_order_acquire), static_cast<u32>(__workers * 2));
    require(g_bad_magic.get(micron::memory_order_acquire), 0u);
    require(g_wrong_thread.get(micron::memory_order_acquire), 0u);
  }
  end_test_case();

  sb::print("=== TLS DTOR OWNERSHIP PASSED ===");
  return 1;
}
