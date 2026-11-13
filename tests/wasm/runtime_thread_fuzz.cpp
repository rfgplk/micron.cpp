//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#ifndef MICRON_WASM_FUZZ_SEED
#define MICRON_WASM_FUZZ_SEED 0x63a9d7e1u
#endif
#ifndef MICRON_WASM_FUZZ_STEPS
#define MICRON_WASM_FUZZ_STEPS 512
#endif

#include "../../src/bits/eh/cxa_eh_globals.hpp"
#include "../../src/bits/wasm/threads.hpp"
#include "../../src/control.hpp"
#include "../../src/errno.hpp"
#include "fuzz_support.hpp"

#define CHECK(expression)                                                                                                                  \
  do {                                                                                                                                     \
    if ( !WASM_FUZZ_CHECK(expression) ) return 6;                                                                                          \
  } while ( false )

using wasm_fuzz::next;

static unsigned constructed = 0, destroyed = 0, hooks = 0, dynamic_hooks = 0, stack_destroyed = 0, unwind_destroyed = 0, failures = 0;
static unsigned guard_attempts[16]{}, guard_successes[16]{};
static thread_local unsigned tls_token = 0, phase = 0;

static constexpr unsigned
guard_value(unsigned i)
{
  return ((i + 3) * 7919u) ^ 0x41b7u;
}

struct tls_lifetime {
  unsigned owner = micron::wasm::thread_id();

  tls_lifetime() { __atomic_fetch_add(&constructed, 1u, __ATOMIC_RELAXED); }

  ~tls_lifetime()
  {
    if ( owner != micron::wasm::thread_id() || !tls_token || phase ) __atomic_fetch_add(&failures, 1u, __ATOMIC_RELAXED);
    phase = 1;
    micron::cancellation_point();
    __atomic_fetch_add(&destroyed, 1u, __ATOMIC_RELAXED);
  }
};

static thread_local tls_lifetime tls;

static void
thread_hook() noexcept
{
  if ( phase != 1 ) __atomic_fetch_add(&failures, 1u, __ATOMIC_RELAXED);
  phase = 2;
  __atomic_fetch_add(&hooks, 1u, __ATOMIC_RELAXED);
}

static void
dynamic_hook() noexcept
{
  if ( phase != 2 ) __atomic_fetch_add(&failures, 1u, __ATOMIC_RELAXED);
  phase = 3;
  __atomic_fetch_add(&dynamic_hooks, 1u, __ATOMIC_RELAXED);
}

template<unsigned I>
static unsigned
guarded()
{
  static unsigned value = [] {
    const auto attempt = __atomic_fetch_add(&guard_attempts[I], 1u, __ATOMIC_RELAXED);
#if defined(__micron_eh)
    if ( I % 3 == 0 && !attempt ) throw I;
#else
    (void)attempt;
#endif
    micron::wasm::yield();
    __atomic_fetch_add(&guard_successes[I], 1u, __ATOMIC_RELAXED);
    return guard_value(I);
  }();
  return value;
}

static unsigned (*const guards[])()
    = { guarded<0>, guarded<1>, guarded<2>,  guarded<3>,  guarded<4>,  guarded<5>,  guarded<6>,  guarded<7>,
        guarded<8>, guarded<9>, guarded<10>, guarded<11>, guarded<12>, guarded<13>, guarded<14>, guarded<15> };

struct job {
  unsigned token, guard, cancel, unwind, iteration;
  unsigned ready = 0, gate = 0;
};

struct stack_lifetime {
  ~stack_lifetime() { __atomic_fetch_add(&stack_destroyed, 1u, __ATOMIC_RELAXED); }
};

#if defined(__micron_eh)
__attribute__((noinline)) static void
raise(unsigned value)
{
  struct scope {
    ~scope() { __atomic_fetch_add(&unwind_destroyed, 1u, __ATOMIC_RELAXED); }
  } cleanup;

  throw value;
}
#endif

static int
run(void *p)
{
  auto &j = *static_cast<job *>(p);
  wasm_fuzz::step = j.iteration;
  if ( !WASM_FUZZ_CHECK(!tls_token && !phase && !__micron_errno) ) return -10;
  (void)&tls;
  tls_token = j.token;
  __micron_errno = static_cast<int>(j.token);
  micron::__dynamic_tls_exit_hook = dynamic_hook;
  stack_lifetime local;
  __atomic_store_n(&j.ready, 1u, __ATOMIC_RELEASE);
  (void)micron::wasm::wake32(&j.ready);
  while ( !__atomic_load_n(&j.gate, __ATOMIC_ACQUIRE) ) (void)micron::wasm::wait32(&j.gate, 0);
  if ( j.cancel ) {
    if ( !WASM_FUZZ_CHECK(micron::wasm::cancellation_requested()) ) return -11;
    return static_cast<int>(0x40000000u | (j.token & 0xffffu));
  }
  if ( !WASM_FUZZ_CHECK(!micron::wasm::cancellation_requested()) ) return -12;
  micron::cancellation_point();
  unsigned value = 0;
#if defined(__micron_eh)
  for ( unsigned attempt = 0; attempt < 2; ++attempt ) {
    try {
      value = guards[j.guard]();
      break;
    } catch ( unsigned index ) {
      if ( !WASM_FUZZ_CHECK(index == j.guard && j.guard % 3 == 0 && attempt == 0) ) return -13;
    }
  }
  if ( j.unwind ) {
    try {
      raise(j.token);
      (void)WASM_FUZZ_CHECK(false);
      return -14;
    } catch ( unsigned token ) {
      if ( !WASM_FUZZ_CHECK(token == j.token) ) return -15;
    }
  }
  if ( !WASM_FUZZ_CHECK(!__cxa_get_globals()->uncaughtExceptions && !__cxa_get_globals()->caughtExceptions) ) return -16;
#else
  value = guards[j.guard]();
#endif
  if ( !WASM_FUZZ_CHECK(value == guard_value(j.guard) && tls_token == j.token && __micron_errno == static_cast<int>(j.token)) ) return -17;
  return static_cast<int>((value ^ j.token) & 0x3fffffffu);
}

int
main()
{
#if defined(__micron_wasm_threads)
  using namespace micron::wasm;
  unsigned seed = wasm_fuzz::seed, total = 0, expected_unwinds = 0;
  bool used[16]{};
  micron::__thread_exit_hook = thread_hook;
  for ( unsigned step = 0; step < wasm_fuzz::steps; ++step ) {
    wasm_fuzz::step = step;
    const unsigned count = 1 + next(seed) % 3;
    job jobs[3]{};
    thread_handle handles[3]{}, stale[3]{};
    unsigned order[3]{ 0, 1, 2 };
    for ( unsigned i = 0; i < count; ++i ) {
      const unsigned token = (next(seed) & 0x3fffffffu) + 1;
      jobs[i] = { token, next(seed) % 16, next(seed) % 5 == 0, next(seed) % 3 == 0, step };
      CHECK(spawn(run, &jobs[i], handles[i]) == 0);
      stale[i] = handles[i];
      ++total;
    }
    for ( unsigned i = 0; i < count; ++i ) {
      while ( !__atomic_load_n(&jobs[i].ready, __ATOMIC_ACQUIRE) ) (void)wait32(&jobs[i].ready, 0);
      if ( jobs[i].cancel ) {
        CHECK(request_cancel(handles[i]) == 0);
        CHECK(__set_park(handles[i], false) == -125);
      } else {
        used[jobs[i].guard] = true;
        CHECK(__set_park(handles[i], true) == 0);
        CHECK(__set_park(handles[i], false) == 0);
#if defined(__micron_eh)
        expected_unwinds += jobs[i].unwind;
#endif
      }
    }
    for ( unsigned i = 0; i < count; ++i ) {
      __atomic_store_n(&jobs[i].gate, 1u, __ATOMIC_RELEASE);
      (void)wake32(&jobs[i].gate);
      const unsigned other = next(seed) % count;
      const auto tmp = order[i];
      order[i] = order[other];
      order[other] = tmp;
    }
    for ( unsigned k = 0; k < count; ++k ) {
      const auto i = order[k];
      int value = 0;
      const int expected = jobs[i].cancel ? static_cast<int>(0x40000000u | (jobs[i].token & 0xffffu))
                                          : static_cast<int>((guard_value(jobs[i].guard) ^ jobs[i].token) & 0x3fffffffu);
      CHECK(join(handles[i], &value) == 0);
      CHECK(value == expected);
      CHECK(join(stale[i]) == -22);
      CHECK(request_cancel(stale[i]) == -22);
    }
    CHECK(constructed == total && destroyed == total);
    CHECK(hooks == total && dynamic_hooks == total && stack_destroyed == total);
    CHECK(unwind_destroyed == expected_unwinds && !failures);
    CHECK(!__atomic_load_n(&__live_threads, __ATOMIC_ACQUIRE));
  }
  micron::__thread_exit_hook = nullptr;
  for ( unsigned i = 0; i < 16; ++i ) {
    const unsigned success = used[i] ? 1 : 0;
    unsigned attempts = success;
#if defined(__micron_eh)
    if ( used[i] && i % 3 == 0 ) ++attempts;
#endif
    CHECK(guard_successes[i] == success && guard_attempts[i] == attempts);
  }
#endif
  return wasm_fuzz::finish("thread-lifecycle");
}
