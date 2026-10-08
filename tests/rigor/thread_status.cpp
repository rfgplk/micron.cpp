//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/thread/status_thread.hpp"
#include "../../src/sec/seccomp.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../snowball/snowball.hpp"

namespace
{
thread_local u64 tls_value = 41;
struct thread_state {
  micron::atomic_token<u32> entered{0}, release{0}, failures{0};
};

void
work(void *opaque)
{
  auto &s = *static_cast<thread_state *>(opaque);
  if ( tls_value != 41 ) s.failures.fetch_add(1, micron::memory_order_relaxed);
  ++tls_value;
  // Exercise the normal per-thread allocator cleanup path.
  auto *bytes = new byte[4096];
  for ( usize i = 0; i < 4096; ++i ) bytes[i] = i;
  if ( bytes[4095] != 255 ) s.failures.fetch_add(1, micron::memory_order_relaxed);
  delete[] bytes;
  s.entered.fetch_add(1, micron::memory_order_release);
  micron::wait_futex(s.release.ptr(), 0u);
}

void
empty(void *) { }
}

int
main()
{
  using namespace micron;
  sb::test_case("recoverable threads preserve TLS, join ownership and repeated allocator lifetime");
  static_assert(!micron::is_move_constructible_v<status_thread<>>);
  tls_value = 99;
  for ( u32 pass = 0; pass < 8; ++pass ) {
    thread_state s; status_thread<> threads[4];
    for ( auto &t : threads ) sb::require_true(t.start(work, &s) == 0 && t.id() > 0 && t.mapped_bytes() <= t.reservation_bytes);
    while ( s.entered.get(memory_order_acquire) != 4 ) micron::yield();
    for ( auto &t : threads ) {
      sb::require_true(t.try_join() == -error::busy && t.start(empty, nullptr) == -error::busy);
    }
    release_futex(s.release.ptr(), 1u, 4);
    for ( auto &t : threads ) sb::require_true(t.join() == 0 && t.try_join() == 0 && !t.id() && !t.mapped_bytes());
    sb::require_true(!s.failures.get() && tls_value == 99);
    sb::require_true(threads[0].start(empty, nullptr) == 0 && threads[0].join() == 0);
  }
  sb::end_test_case();

  sb::test_case("allocation, guard and clone failures return status with no live child or retained mapping");
  for ( int call : {SYS_mmap, SYS_mprotect, SYS_clone} ) {
    const int pid = micron::try_fork(); sb::require_true(pid >= 0);
    if ( !pid ) {
      status_thread<> thread;
      sb::require_true(thread.start(nullptr, nullptr) == -error::invalid_arg);
      sec::seccomp::filter_builder<64> filter;
      filter.require_native_arch().deny_errno(call, error::out_of_memory);
      if ( call == SYS_clone ) filter.deny_errno(SYS_clone3, error::out_of_memory);
      filter.default_allow(); sb::require_true(sec::seccomp::load(filter) == 0);
      sb::require_true(thread.start(empty, nullptr) < 0 && !thread.id() && !thread.mapped_bytes());
      sb::require_true(thread.start(empty, nullptr) < 0 && thread.join() == 0);
      micron::sys_group_exit(1);
    }
    int result = 0;
    sb::require_true(micron::waitpid(pid, &result, 0) == pid && micron::wifexited(result) && micron::wexitstatus(result) == 1);
  }
  sb::end_test_case();
  sb::test_case("single futex waits expose kernel failure without aborting or pretending to acquire a permit");
  const int pid = micron::try_fork(); sb::require_true(pid >= 0);
  if ( !pid ) {
    atomic_token<u32> word{0};
    sb::require_true(try_wait_futex(nullptr, 0) == -error::invalid_arg && try_wait_futex(word.ptr(), 1) == 0);
    sec::seccomp::filter_builder<32> filter;
    filter.require_native_arch().deny_errno(SYS_futex, error::io_error).default_allow();
    sb::require_true(sec::seccomp::load(filter) == 0 && try_wait_futex(word.ptr(), 0) == -error::io_error);
    micron::sys_group_exit(1);
  }
  int result = 0;
  sb::require_true(micron::waitpid(pid, &result, 0) == pid && micron::wifexited(result) && micron::wexitstatus(result) == 1);
  sb::end_test_case(); return 1;
}
