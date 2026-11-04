//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/linux/elf/tls.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"
#include "../../src/thread/thread_types/void_thread.hpp"

#include "../snowball/snowball.hpp"

extern "C" int __cxa_atexit(void (*)(void *), void *, void *) noexcept;

namespace
{
u64 module_id;
const int initial = 41;
u32 destroyed = 0;
u32 failed = 0;
u32 ready = 0;
#if defined(__micron_tdtor_real)
u32 guest_destroyed = 0;
#endif

int *
value()
{
  micron::elf::tls_index_t index{ static_cast<unsigned long>(module_id), 0 };
  return static_cast<int *>(__tls_get_addr(&index));
}

#if defined(__micron_tdtor_real)
void
guest_destructor(void *expected)
{
  auto *p = value();
  if ( !p || *p != static_cast<int>(reinterpret_cast<uintptr_t>(expected)) ) __atomic_fetch_add(&failed, 1u, __ATOMIC_RELAXED);
  __atomic_fetch_add(&guest_destroyed, 1u, __ATOMIC_RELAXED);
}
#endif

struct probe {
  bool first_touch = false;

  ~probe()
  {
    auto *p = value();
    if ( !p || *p != (first_touch ? initial : 42) ) __atomic_fetch_add(&failed, 1u, __ATOMIC_RELAXED);
    __atomic_fetch_add(&destroyed, 1u, __ATOMIC_RELAXED);
#if defined(__micron_tdtor_real)
    if ( !micron::__push_thread_dtor(guest_destructor, reinterpret_cast<void *>(static_cast<uintptr_t>(first_touch ? initial : 42))) )
      __atomic_fetch_add(&failed, 1u, __ATOMIC_RELAXED);
#endif
  }
};

void
worker(bool first_touch)
{
  thread_local probe p;
  p.first_touch = first_touch;
  if ( !first_touch ) {
    auto *v = value();
    if ( !v || *v != initial ) __atomic_fetch_add(&failed, 1u, __ATOMIC_RELAXED);
    if ( v ) *v = 42;
  }
  __atomic_store_n(&ready, 1u, __ATOMIC_RELEASE);
}

void
cancel_worker()
{
  worker(false);
  for ( ;; ) micron::micthread::cancel();
}

#if !defined(__micron_freestanding)
void *
foreign(void *first_touch)
{
  worker(first_touch != nullptr);
  return nullptr;
}
#endif

void
check(u32 expected)
{
  sb::require(destroyed, expected);
#if defined(__micron_tdtor_real)
  sb::require(guest_destroyed, expected);
#endif
  sb::require(failed, 0u);
  sb::require(micron::elf::__tls_thread_head == nullptr);
}

void
check_main(void *count)
{
  micron::__run_thread_dtors();
  sb::require(destroyed, static_cast<u32>(reinterpret_cast<uintptr_t>(count)));
#if defined(__micron_tdtor_real)
  sb::require(guest_destroyed, destroyed);
#endif
  sb::require(failed, 0u);
  sb::require(value() && *value() == 42);
  micron::elf::tls_unregister(module_id);
}
}      // namespace

int
main()
{
#if defined(__micron_freestanding) && defined(MICRON_TEST_LEGACY_CLONE)
  micron::posix::__clone3_gate.demote();
#endif
  module_id = micron::elf::tls_register(reinterpret_cast<const u8 *>(&initial), sizeof(initial), sizeof(initial), alignof(int));
  sb::require(module_id != 0);
  u32 expected = 0;

  sb::test_case("concurrent first use publishes one cleanup key and independent TLS records");
  {
    u32 arrived = 0, go = 0;
    auto job = [&] {
      __atomic_fetch_add(&arrived, 1u, __ATOMIC_RELEASE);
      while ( !__atomic_load_n(&go, __ATOMIC_ACQUIRE) ) __cpu_pause();
      worker(false);
    };
    micron::auto_thread<> threads[] = { { job }, { job }, { job }, { job } };
    while ( __atomic_load_n(&arrived, __ATOMIC_ACQUIRE) != 4 ) __cpu_pause();
    __atomic_store_n(&go, 1u, __ATOMIC_RELEASE);
    for ( auto &thread : threads ) sb::require(thread.join() == 0);
    expected += 4;
    check(expected);
  }

  for ( unsigned mode = 0; mode < 2; ++mode ) {
    const bool first_touch = mode != 0;
    sb::test_case(first_touch ? "first dynamic TLS access during destruction" : "dynamic TLS values survive C++ TLS destruction");
    for ( unsigned round = 0; round < 4; ++round ) {
      micron::auto_thread<> thread(worker, first_touch);
      sb::require(thread.join() == 0);
      check(++expected);
    }

    sb::test_case("pool worker teardown retains dynamic TLS until its destructors finish");
    auto *stack = micron::addrmap(micron::thread_stack_size);
    sb::require(!micron::mmap_failed(stack));
    auto attrs = micron::__thread_attr_with_stack(micron::posix::getpid(), micron::posix::sched_other, stack, micron::thread_stack_size);
    ready = 0;
    {
      micron::void_thread<> thread(attrs);
      auto job = [first_touch] { worker(first_touch); };
      thread[job];
      while ( !__atomic_load_n(&ready, __ATOMIC_ACQUIRE) ) __cpu_pause();
      sb::require(thread.join() == 0);
    }
    micron::munmap(stack, micron::thread_stack_size);
    check(++expected);

#if !defined(__micron_freestanding)
    sb::test_case("foreign pthread cleanup follows C++ TLS destruction");
    pthread_t thread{};
    sb::require(pthread_create(&thread, nullptr, foreign, first_touch ? reinterpret_cast<void *>(1) : nullptr) == 0);
    sb::require(pthread_join(thread, nullptr) == 0);
    check(++expected);
#endif
  }

  sb::test_case("abrupt cancellation reclaims dynamic TLS without native unwinding");
  {
    ready = 0;
    micron::auto_thread<> thread(cancel_worker);
    while ( !__atomic_load_n(&ready, __ATOMIC_ACQUIRE) ) __cpu_pause();
    sb::require(thread.cancel() == 0);
#if defined(__micron_freestanding)
    ++expected;
#endif
    check(expected);
  }

  sb::test_case("explicit dynamic TLS cleanup is idempotent");
  sb::require(value() && *value() == initial);
  micron::elf::__tls_release_thread();
  micron::elf::__tls_release_thread();
  sb::require(micron::elf::__tls_thread_head == nullptr);
  sb::require(value() && *value() == initial);
  micron::elf::__tls_release_thread();

  sb::test_case("main TLS remains available to process finalizers");
  worker(false);
  sb::require(__cxa_atexit(check_main, reinterpret_cast<void *>(static_cast<uintptr_t>(expected + 1)), nullptr) == 0);
  return 1;
}
