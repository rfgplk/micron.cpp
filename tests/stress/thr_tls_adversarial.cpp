//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/atomic/atomic.hpp"
#include "../../src/attributes.hpp"
#include "../../src/bits/__thread_exit_hook.hpp"
#include "../../src/exit.hpp"
#include "../../src/syscall.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/threads.hpp"

#include "../snowball/snowball.hpp"
#include "../support/lifetime.hpp"
#include "../support/mt.hpp"

using namespace snowball;

namespace
{

constexpr int PROBES = 40;
constexpr int WORKERS = 6;
constexpr u64 ROUNDS = 40;

micron::atomic_token<u64> g_ctor{ 0 };
micron::atomic_token<u64> g_dtor{ 0 };
micron::atomic_token<u64> g_bad_magic{ 0 };
micron::atomic_token<u64> g_wrong_thread{ 0 };
micron::atomic_token<u64> g_double{ 0 };

constexpr u64 PROBE_MAGIC = 0x7105DEADBEEF7105ull;
constexpr u64 PROBE_DEAD = 0xD1EDD1EDD1EDD1EDull;

[[nodiscard]] inline i32
this_tid(void) noexcept
{
  return static_cast<i32>(micron::syscall(SYS_gettid));
}

struct probe {
  u64 magic;
  i32 owner;

  probe(void) noexcept : magic(PROBE_MAGIC), owner(this_tid()) { g_ctor.fetch_add(1, micron::memory_order_relaxed); }

  ~probe(void) noexcept
  {
    if ( magic == PROBE_DEAD ) {
      g_double.fetch_add(1, micron::memory_order_relaxed);
      return;
    }
    if ( magic != PROBE_MAGIC ) {
      g_bad_magic.fetch_add(1, micron::memory_order_relaxed);
      return;
    }
    if ( owner != this_tid() ) g_wrong_thread.fetch_add(1, micron::memory_order_relaxed);
    magic = PROBE_DEAD;
    g_dtor.fetch_add(1, micron::memory_order_relaxed);
  }
};

template<int N>
[[gnu::noinline]] probe &
slot(void) noexcept
{
  static thread_local probe __p{};
  return __p;
}

template<int N>
void
touch_upto(void) noexcept
{
  (void)slot<N>().owner;
  if constexpr ( N > 0 ) touch_upto<N - 1>();
}

micron::atomic_token<u64> g_reentrant_ctor{ 0 };
micron::atomic_token<u64> g_reentrant_dtor{ 0 };

struct tail_probe {
  tail_probe(void) noexcept { g_reentrant_ctor.fetch_add(1, micron::memory_order_relaxed); }

  ~tail_probe(void) noexcept { g_reentrant_dtor.fetch_add(1, micron::memory_order_relaxed); }
};

[[gnu::noinline]] tail_probe &
tail(void) noexcept
{
  static thread_local tail_probe __t{};
  return __t;
}

struct spawner_probe {
  bool armed;

  explicit spawner_probe(bool a) noexcept : armed(a) { }

  ~spawner_probe(void) noexcept
  {
    if ( armed ) (void)&tail();
  }
};

[[gnu::noinline]] void
arm_reentrant(void) noexcept
{
  static thread_local spawner_probe __s{ true };
  (void)__s.armed;
}

micron::atomic_token<u64> g_main_ctor{ 0 };
micron::atomic_token<u64> g_main_dtor{ 0 };

struct main_probe {
  main_probe(void) noexcept { g_main_ctor.fetch_add(1, micron::memory_order_relaxed); }

  ~main_probe(void) noexcept { g_main_dtor.fetch_add(1, micron::memory_order_relaxed); }
};

[[gnu::noinline]] main_probe &
main_tls(void) noexcept
{
  static thread_local main_probe __m{};
  return __m;
}

void
__check_main_tls(void)
{
  const u64 n = g_main_dtor.get(micron::memory_order_acquire);
  if ( n != 1u ) {
    sb::print("MAIN THREAD_LOCAL NOT DESTROYED EXACTLY ONCE: dtor ran ", static_cast<usize>(n), " times");
    micron::sys_group_exit(6);
  }
}

gdestructor_ void
__check_main_tls_fini(void)
{
  __check_main_tls();
}

}      // namespace

int
main(void)
{
  using namespace micron;

  micron::atexit(&__check_main_tls);

  sb::print("=== THREAD_LOCAL ADVERSARIAL ===");
  sb::print("    probes/worker: ", static_cast<usize>(PROBES), "  workers: ", static_cast<usize>(WORKERS),
            "  rounds: ", static_cast<usize>(ltest::scaled(ROUNDS)), "  scale: ", static_cast<usize>(ltest::stress_scale));

#if defined(__micron_tdtor_real)
  sb::print("    per-thread dtor list: micron's own, cap ", static_cast<usize>(micron::__tdtor_cap));
  const bool own_list = true;
  const u32 cap = micron::__tdtor_cap;
#else
  sb::print("    per-thread dtor list: the host libc's (hosted build) - cap gates skipped");
  const bool own_list = false;
  const u32 cap = 0u;
#endif

  if ( !micron::threads_available() ) {
    sb::print("threads unavailable - SKIPPED");
    return 1;
  }

  (void)main_tls();
  require(g_main_ctor.get(memory_order_acquire), static_cast<u64>(1));

  const i32 wm0 = ltest::fd_watermark();

  test_case("many thread_locals per worker: destroyed on their own thread, none twice");
  {
    g_ctor.store(0, memory_order_release);
    g_dtor.store(0, memory_order_release);
    g_bad_magic.store(0, memory_order_release);
    g_wrong_thread.store(0, memory_order_release);
    g_double.store(0, memory_order_release);
#if defined(__micron_tdtor_real)
    const u32 dropped0 = __atomic_load_n(&micron::__tdtor_dropped, __ATOMIC_ACQUIRE);
#endif

    mtest::parallel(WORKERS, [](int) { touch_upto<PROBES - 1>(); });

    const u64 built = g_ctor.get(memory_order_acquire);
    const u64 destroyed = g_dtor.get(memory_order_acquire);
    require(built, static_cast<u64>(WORKERS) * static_cast<u64>(PROBES));

    require(g_bad_magic.get(memory_order_acquire), static_cast<u64>(0));
    require(g_double.get(memory_order_acquire), static_cast<u64>(0));
    require(g_wrong_thread.get(memory_order_acquire), static_cast<u64>(0));

    if ( own_list && cap < static_cast<u32>(PROBES) ) {
#if defined(__micron_tdtor_real)

      const u32 dropped1 = __atomic_load_n(&micron::__tdtor_dropped, __ATOMIC_ACQUIRE);
      const u64 lost = static_cast<u64>(dropped1 - dropped0);
      require_true(destroyed < built);
      require(built - destroyed, lost);
      sb::print("     cap ", static_cast<usize>(cap), " < probes ", static_cast<usize>(PROBES), ": destroyed ",
                static_cast<usize>(destroyed), " of ", static_cast<usize>(built), ", __tdtor_dropped +", static_cast<usize>(lost));
#endif
    } else {

      require(destroyed, built);
#if defined(__micron_tdtor_real)
      const u32 dropped1 = __atomic_load_n(&micron::__tdtor_dropped, __ATOMIC_ACQUIRE);
      require(static_cast<u64>(dropped1 - dropped0), static_cast<u64>(0));
#endif
      sb::print("     built=", static_cast<usize>(built), " destroyed=", static_cast<usize>(destroyed), " foreign=0 double=0 bad_magic=0");
    }
  }
  end_test_case();

  test_case("recycled TLS frames: no registration survives its thread");
  {
    g_ctor.store(0, memory_order_release);
    g_dtor.store(0, memory_order_release);
    g_bad_magic.store(0, memory_order_release);
    g_wrong_thread.store(0, memory_order_release);
    g_double.store(0, memory_order_release);

    const u64 gens = ltest::scaled(ROUNDS);
    constexpr int SMALL = 4;
    for ( u64 g = 0; g < gens; ++g ) {
      auto t = solo::spawn<auto_thread<>>([]() { touch_upto<SMALL - 1>(); });
      solo::join(t);
    }
    const u64 built = g_ctor.get(memory_order_acquire);
    require(built, gens * static_cast<u64>(SMALL));
    require(g_dtor.get(memory_order_acquire), built);
    require(g_bad_magic.get(memory_order_acquire), static_cast<u64>(0));
    require(g_double.get(memory_order_acquire), static_cast<u64>(0));
    require(g_wrong_thread.get(memory_order_acquire), static_cast<u64>(0));
    sb::print("     generations=", static_cast<usize>(gens), " probes=", static_cast<usize>(built), " all destroyed on their owner");
  }
  end_test_case();

  test_case("re-entrant registration: a dtor's fresh thread_local is still drained");
  {
    g_reentrant_ctor.store(0, memory_order_release);
    g_reentrant_dtor.store(0, memory_order_release);

    const u64 gens = ltest::scaled(ROUNDS / 2);
    for ( u64 g = 0; g < gens; ++g ) {
      auto t = solo::spawn<auto_thread<>>([]() { arm_reentrant(); });
      solo::join(t);
    }

    require(g_reentrant_ctor.get(memory_order_acquire), gens);
    require(g_reentrant_dtor.get(memory_order_acquire), gens);
    sb::print("     re-entrant registrations=", static_cast<usize>(gens), " all drained");
  }
  end_test_case();

  test_case("concurrent live frames: per-thread lists stay independent");
  {
    g_ctor.store(0, memory_order_release);
    g_dtor.store(0, memory_order_release);
    g_bad_magic.store(0, memory_order_release);
    g_wrong_thread.store(0, memory_order_release);
    g_double.store(0, memory_order_release);

    const u64 waves = ltest::scaled(ROUNDS / 4);
    constexpr int SMALL = 8;
    for ( u64 w = 0; w < waves; ++w ) mtest::parallel(WORKERS, [](int) { touch_upto<SMALL - 1>(); });

    const u64 built = g_ctor.get(memory_order_acquire);
    require(built, waves * static_cast<u64>(WORKERS) * static_cast<u64>(SMALL));
    require(g_dtor.get(memory_order_acquire), built);
    require(g_bad_magic.get(memory_order_acquire), static_cast<u64>(0));
    require(g_double.get(memory_order_acquire), static_cast<u64>(0));
    require(g_wrong_thread.get(memory_order_acquire), static_cast<u64>(0));
  }
  end_test_case();

  const i32 wm1 = ltest::fd_watermark();
  require(wm0, wm1);

  require(g_main_dtor.get(memory_order_acquire), static_cast<u64>(0));

  sb::print("=== ALL THREAD_LOCAL ADVERSARIAL TESTS PASSED (main TLS graded at exit) ===");
  return 1;
}
