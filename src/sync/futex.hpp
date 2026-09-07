//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

#include "../errno.hpp"
#include "../bits/__posix_time_types.hpp"
#include "../port/wait.hpp"
#include "../type_traits.hpp"

#include "../atomic/atomic.hpp"
#include "../atomic/intrin.hpp"
#include "../except.hpp"

namespace micron
{

// Phase 4: the futex op vocabulary and the raw syscall live in port/backends/wait_linux.hpp.
// They are re-exported here under their original names, so all nine keep-set consumers
// (mutex/locks/{futex_mutex,shared_mutex}, sync/{latch,event_count,semaphore,futex_future},
// math/compute) are untouched. This header is now a shim: it owns the typed helpers, not the ABI.
//
// Phase 5: THE RE-EXPORT IS LINUX-ONLY, and so is __futex. A futex is a userspace-to-kernel
// construct; inside a kernel module there is nothing on the other side of it, and the kernel
// backend answers the portable wait/wake pair out of a hashed waitqueue instead. Leaving these
// ungated made every container reach a name that cannot exist -- barebones_core.cpp under
// MICRON_PORT_KERNEL was 33 errors, all of them these, none of them the allocator.
//
// The typed helpers below are backend-neutral BECAUSE they now go through port::wait/port::wake,
// which is what wait_linux.hpp:94 calls "the portable pair -- what a kernel or metal backend owes".
// On linux port::wait with a literal -1 folds to exactly the __futex_linux call these used to make.
#if defined(__micron_port_linux)
using port::futex_clock_realtime;
using port::futex_cmp_requeue;
using port::futex_cmp_requeue_pi;
using port::futex_fd;
using port::futex_futex_lock_pi2;
using port::futex_lock_pi;
using port::futex_private_flag;
using port::futex_requeue;
using port::futex_trylock_pi;
using port::futex_unlock_pi;
using port::futex_wait;
using port::futex_wait_bitset;
using port::futex_wait_requeue_pi;
using port::futex_wake;
using port::futex_wake_bitset;
using port::futex_wake_op;

[[gnu::always_inline]] inline auto
__futex(u32 *addr, int futex, u32 val, timespec_t *timeout, u32 *addr2, u32 val2)
{
  return port::__futex_linux(addr, futex, val, timeout, addr2, val2);
}
#endif

template<typename T>
  requires(sizeof(T) == 4)
void
wait_futex(T *ptr, decay_t<T> expected)
{
  while ( atom::load(ptr, (int)memory_order_relaxed) == expected ) {
    // -1 is "wait forever". -11 is EAGAIN (the word already changed, which is not an error) and
    // -4 is EINTR; every other negative return is a broken wait and must not be spun on.
    auto ret = port::wait(reinterpret_cast<u32 *>(const_cast<decay_t<T> *>(ptr)), static_cast<u32>(expected), -1);
    if ( ret < 0 and ret != -11 and ret != -4 ) {
      micron::exc<except::thread_error>("futex wait failed");
    }
  }
}

template<typename T>
  requires(sizeof(T) == 4)
void
release_futex(T *ptr, decay_t<T> to_store, u32 cnt = 1)
{
  atom::store(ptr, to_store, (int)memory_order_release);
  port::wake(reinterpret_cast<u32 *>(ptr), static_cast<i32>(cnt));
}

template<typename T>
  requires(sizeof(T) == 4)
void
wake_futex(T *ptr, int cnt = 1)
{
  port::wake(reinterpret_cast<u32 *>(const_cast<decay_t<T> *>(ptr)), static_cast<i32>(cnt));
}

// __D is the unlocked value
// __L is the locked value
template<typename T = u32, T __D = 0, T __L = 1>
  requires(micron::is_integral_v<T> && __D != __L)
struct futex {
  T __value;
  ~futex() = default;
  futex(void) : __value(__D) { };
  futex(const futex &) = delete;
  futex(futex &&) = delete;
  futex &operator=(const futex &) = delete;
  futex &operator=(futex &&) = delete;

  void
  wait()
  {
    for ( ;; ) {
      T expected = __D;
      if ( atom::cmp_exchange_weak(&__value, &expected, __L) ) return;
      auto ret = port::wait(reinterpret_cast<u32 *>(&__value), static_cast<u32>(expected), -1);
      if ( ret < 0 && ret != -11 && ret != -4 ) {
        micron::exc<except::thread_error>("futex wait failed");
      }
    }
  }

  void
  release()
  {
    atom::store(&__value, __D, atomic_seq_cst);
    port::wake(reinterpret_cast<u32 *>(&__value), 1);
  }
};

};      // namespace micron
