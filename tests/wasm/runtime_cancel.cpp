//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/control.hpp"
#include "../../src/sync/pause.hpp"
#include "../../src/thread/thread.hpp"

static unsigned tls_destroyed = 0, packet_destroyed = 0, hooks = 0, dynamic_hooks = 0, failures = 0, returned = 0, caught = 0,
                stack_destroyed = 0;
static unsigned outer_attempts = 0, inner_attempts = 0;
static thread_local unsigned phase = 0;

struct job {
  unsigned ready = 0, gate = 0;
  bool guard = false, sleeping = false;
};

static thread_local job *current = nullptr;

struct local_lifetime {
  unsigned owner = micron::wasm::thread_id();

  ~local_lifetime()
  {
    unsigned marker;
    const auto top = reinterpret_cast<usize>(micron::wasm::stack_begin()) + micron::wasm::stack_size();
    const auto address = reinterpret_cast<usize>(&marker);
    if ( owner != micron::wasm::thread_id() || phase || address >= top || top - address > 8192 )
      __atomic_fetch_add(&failures, 1u, __ATOMIC_RELAXED);
    phase = 1;
    micron::cancellation_point();
    __atomic_fetch_add(&tls_destroyed, 1u, __ATOMIC_RELAXED);
  }
};

static thread_local local_lifetime local;

static void
thread_hook() noexcept
{
  if ( phase != 1 ) __atomic_fetch_add(&failures, 1u, __ATOMIC_RELAXED);
  phase = 2;
  micron::cancellation_point();
  __atomic_fetch_add(&hooks, 1u, __ATOMIC_RELAXED);
}

static void
dynamic_hook() noexcept
{
  if ( phase != 2 ) __atomic_fetch_add(&failures, 1u, __ATOMIC_RELAXED);
  phase = 3;
  micron::park();
  __atomic_fetch_add(&dynamic_hooks, 1u, __ATOMIC_RELAXED);
}

static void
publish(unsigned &word, unsigned value)
{
  __atomic_store_n(&word, value, __ATOMIC_RELEASE);
  (void)micron::wasm::wake32(&word);
}

static void
await(unsigned &word, unsigned expected)
{
  while ( __atomic_load_n(&word, __ATOMIC_ACQUIRE) != expected )
    (void)micron::wasm::wait32(&word, __atomic_load_n(&word, __ATOMIC_RELAXED), 1000000);
}

__attribute__((noinline)) static void
checkpoint(job &j)
{
#if defined(__micron_eh)
  if ( __atomic_load_n(&j.gate, __ATOMIC_RELAXED) == 99 ) throw 17;
#endif
  micron::cancellation_point();
}

__attribute__((noinline)) static void
may_throw(job &j)
{
#if defined(__micron_eh)
  if ( __atomic_load_n(&j.gate, __ATOMIC_RELAXED) == 99 ) throw 88;
#else
  (void)j;
#endif
}

__attribute__((noinline)) static void
descend(job &j, unsigned depth) noexcept
{
  volatile unsigned char frame[4096];
  may_throw(j);
  frame[0] = static_cast<unsigned char>(depth);
  if ( depth ) {
    descend(j, depth - 1);
  } else {
    publish(j.ready, 1);
    while ( !__atomic_load_n(&j.gate, __ATOMIC_ACQUIRE) ) micron::cancellation_point();
  }
  may_throw(j);
  if ( frame[0] != static_cast<unsigned char>(depth) ) __atomic_fetch_add(&failures, 1u, __ATOMIC_RELAXED);
}

static int
inner_guard()
{
  static int value = [] {
    if ( __atomic_add_fetch(&inner_attempts, 1u, __ATOMIC_RELAXED) == 1 ) {
      publish(current->ready, 1);
      while ( !__atomic_load_n(&current->gate, __ATOMIC_ACQUIRE) ) checkpoint(*current);
    }
    return 81;
  }();
  return value;
}

static int
outer_guard()
{
  static int value = [] {
    __atomic_fetch_add(&outer_attempts, 1u, __ATOMIC_RELAXED);
    return inner_guard() + 1;
  }();
  return value;
}

struct task {
  job *work;
  bool owns = true;

  explicit task(job &j) : work(&j) { }

  task(task &&other) noexcept : work(other.work), owns(other.owns) { other.owns = false; }

  task(const task &) = delete;

  ~task()
  {
    if ( owns ) __atomic_fetch_add(&packet_destroyed, 1u, __ATOMIC_RELAXED);
  }

  int
  operator()()
  {
    struct stack_lifetime {
      ~stack_lifetime() { __atomic_fetch_add(&stack_destroyed, 1u, __ATOMIC_RELAXED); }
    } stack;

    (void)&local;
    current = work;
    micron::__dynamic_tls_exit_hook = dynamic_hook;
#if defined(__micron_eh)
    try {
#endif
      if ( work->guard ) {
        if ( outer_guard() != 82 ) __atomic_fetch_add(&failures, 1u, __ATOMIC_RELAXED);
      } else if ( work->sleeping ) {
        publish(work->ready, 1);
        while ( !__atomic_load_n(&work->gate, __ATOMIC_ACQUIRE) ) micron::sleep_for(1);
      } else {
        constexpr unsigned depth = MICRON_WASM_STACK_SIZE / 16384 < 16 ? MICRON_WASM_STACK_SIZE / 16384 : 16;
        descend(*work, depth);
      }
#if defined(__micron_eh)
    } catch ( ... ) {
      __atomic_fetch_add(&caught, 1u, __ATOMIC_RELAXED);
    }
#endif
    __atomic_fetch_add(&returned, 1u, __ATOMIC_RELAXED);
    return 82;
  }
};

int
main()
{
#if defined(__micron_wasm_threads)
  micron::__thread_exit_hook = thread_hook;
  job running;
  {
    micron::thread<> thread{ task(running) };
    await(running.ready, 1);
    if ( thread.cancel() || thread.alive() || thread.native_handle().slot || thread.join() ) return 6;
  }
  if ( tls_destroyed != 1 || packet_destroyed != 1 || hooks != 1 || dynamic_hooks != 1 || returned || caught || failures
       || stack_destroyed )
    return 6;

  job parked;
  {
    micron::thread<> thread{ task(parked) };
    await(parked.ready, 1);
    if ( thread.sleep() ) return 6;
    auto *word = &micron::wasm::__threads[thread.native_handle().slot].park;
    unsigned attempts = 0;
    while ( !micron::wasm::wake32(word, 1) ) {
      if ( ++attempts == 100000 ) return 6;
      unsigned delay = 0;
      (void)micron::wasm::wait32(&delay, 0, 10000);
    }
    if ( thread.cancel() || thread.alive() || thread.native_handle().slot ) return 6;
  }
  if ( tls_destroyed != 2 || packet_destroyed != 2 || hooks != 2 || dynamic_hooks != 2 || returned || caught || failures
       || stack_destroyed )
    return 6;

  job sleeping{ 0, 0, false, true };
  {
    micron::thread<> thread{ task(sleeping) };
    await(sleeping.ready, 1);
    if ( thread.cancel() || thread.alive() || thread.native_handle().slot ) return 6;
  }
  if ( tls_destroyed != 3 || packet_destroyed != 3 || hooks != 3 || dynamic_hooks != 3 || returned || caught || failures
       || stack_destroyed )
    return 6;

  job initializing{ 0, 0, true };
  {
    micron::thread<> thread{ task(initializing) };
    await(initializing.ready, 1);
    if ( thread.cancel() || thread.alive() || thread.native_handle().slot ) return 6;
  }
  if ( outer_attempts != 1 || inner_attempts != 1 || returned || caught || failures ) return 6;
  job retry{ 0, 1, true };
  {
    micron::thread<> thread{ task(retry) };
    if ( thread.result<int>() != 82 || thread.join() ) return 6;
  }
  micron::__thread_exit_hook = nullptr;
  if ( outer_attempts != 2 || inner_attempts != 2 || returned != 1 || caught || failures || stack_destroyed != 1 ) return 6;
  if ( tls_destroyed != 5 || packet_destroyed != 5 || hooks != 5 || dynamic_hooks != 5 || micron::wasm::__live_threads ) return 6;
#endif
  return 1;
}
