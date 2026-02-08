//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/locks.hpp"
#include "../stack.hpp"
#include "../vector/ivector.hpp"

#include "../algorithm/algorithm.hpp"
#include "../linux/sys/system.hpp"

#include "../numerics.hpp"
#include "../type_traits.hpp"
#include "cpu.hpp"
#include "thread.hpp"

// #define ABSURD_THREAD_COUNT
// #define HIGH_THREAD_COUNT

namespace micron
{

// NOTE: maximum of 1024 threads, this is a conservative limit, but there's almost no legitimate instance where you would
// want to be running more threads than this due to a) few machines have this many cores, b) see a
// this is solely meant as a memory saving measure, if you need more threads increase the limit or define one of the
// macros below
#if defined(ABSURD_THREAD_COUNT)
constexpr static const u32 maximum_threads = (1 << 15);     // 32768
#elseif defined(MICRON_HIGH_THREAD_COUNT)
constexpr static const u32 maximum_threads = (1 << 12);     // 4096
#elif !defined(MICRON_LOW_THREAD_COUNT)
constexpr static const u32 maximum_threads = (1 << 10);     // 1024
#else
constexpr static const u32 maximum_threads = (1 << 8);     // 256
#endif

template <typename Tr> struct thread_t {
  uid_t uid;     // NOTE: requires CAP_SETUID
  pthread_attr_t attributes;
  cpu_t<true> cpu_mask;     // cpu mask for that specific thread
  Tr thread;
};

// NOTE: not for external usage, always call from /spawn.hpp
namespace arena
{
class __empty_arena
{
};

// Arenas are structures meant to hold and oversee a collection of threads
// __default_arena is the standard arena structure, containing regular threads, nothing special.
// runs on a stack for performance reasons, it's possible to stall out or overload the arena by having the last added
// thread busy while other threads wait for recollection, be warned, use parallel arenas if those workloads are likely
template <typename Tr = thread<thread_stack_size>>
  requires(!micron::is_same_v<Tr, auto_thread<>>)     // we don't want to use auto_threads in an arena
class __default_arena
{
  micron::fsstack<thread_t<Tr>, maximum_threads> threads;
  micron::mutex mtx;

  addr_t *
  __create_stack(void) const
  {
    addr_t *fstack = micron::addrmap(thread_stack_size);
    if ( micron::mmap_failed(fstack) )
      throw except::thread_error("micron arena::__create_stack(): failed to allocate stack");
    return fstack;
  }

public:
  ~__default_arena()
  {
    // forcing stops
    force_clean();
  }
  __default_arena(void) : threads() {}
  __default_arena(const __default_arena &) = delete;
  __default_arena(__default_arena &&) = delete;
  __default_arena &operator=(const __default_arena &) = delete;
  __default_arena &operator=(__default_arena &&) = delete;
  // thread creation functions
  // Create a new thread automatically, without any config. Uses generally standard best practices config options
  template <typename Func, typename... Args>
    requires(micron::is_invocable_v<Func, Args...>)
  auto &
  create(Func f, Args &&...args)
  {
    micron::lock_guard l(mtx);
    // safety check
    if ( threads.full_or_overflowed() ) [[unlikely]]
      throw except::thread_error("micron arena::create(): Arena is full or corrupted");
    addr_t *stack_ptr = __create_stack();
    pthread_attr_t attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);
    pthread::set_stack_thread(attrs, stack_ptr, thread_stack_size);
    threads.emplace(posix::getuid(), attrs, cpu_t<true>(), Tr{ stack_ptr, attrs, f, micron::forward<Args>(args)... });
    // posix::get_priority(prio_process, 0)
    return threads.top();
  }
  // Create a new thread at a specific core/unit
  template <typename Func, typename... Args>
    requires(micron::is_invocable_v<Func, Args...>)
  auto &
  create_at(size_t n, Func f, Args &&...args)
  {
    micron::lock_guard l(mtx);
    if ( threads.full_or_overflowed() ) [[unlikely]]
      throw except::thread_error("micron arena::create(): Arena is full or corrupted");
    cpu_t<true> c;
    c.clear();
    c.set_core(n);
    addr_t *stack_ptr = __create_stack();
    pthread_attr_t attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);
    pthread::set_stack_thread(attrs, stack_ptr, thread_stack_size);
    threads.emplace(posix::getuid(), attrs, c, Tr{ stack_ptr, micron::move(attrs), f, micron::forward<Args>(args)... });

    // threads.emplace(posix::getuid(), posix::get_priority(prio_process, 0), c, Tr{ f, micron::forward<Args>(args)...
    // });
    posix::sched_setaffinity(threads.top().thread.thread_id(), sizeof(c.get()), c.get());
    return threads.top();
  }
  // Create a new thread at a specific core that is currently more free than the rest
  template <typename Func, typename... Args>
    requires(micron::is_invocable_v<Func, Args...>)
  auto &
  create_burden(Func f, Args &&...args)
  {
    micron::lock_guard l(mtx);
    if ( threads.full_or_overflowed() ) [[unlikely]]
      throw except::thread_error("micron arena::create(): Arena is full or corrupted");
    fvector<i32> cores(cpu_count());

    cpu_t<true> c;
    for ( u64 i = 0; i < threads.size(); ++i ) {
      c.clear();
      posix::sched_getaffinity(threads[i].thread.thread_id(), sizeof(c.get()), c.get());
      for ( u64 k = 0; k < cores.size(); ++k )
        if ( c[k] )
          cores[k]++;
    }

    fvector<i32>::const_iterator smallest = min_at(cores);
    if ( smallest != cores.cbegin() ) {
      c.clear();
      c.set_core(smallest - cores.cbegin());
      goto start_thread;
    }
    c.clear();
    c.set_core(0);
  start_thread:
    addr_t *stack_ptr = __create_stack();
    pthread_attr_t attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);
    pthread::set_stack_thread(attrs, stack_ptr, thread_stack_size);
    threads.emplace(posix::getuid(), attrs, c, Tr{ stack_ptr, micron::move(attrs), f, micron::forward<Args>(args)... });

    // threads.emplace(posix::getuid(), posix::get_priority(prio_process, 0), c, Tr{ f, micron::forward<Args>(args)...
    // });
    posix::sched_setaffinity(threads.top().thread.thread_id(), sizeof(c.get()), c.get());
    return threads.top();
  }

  // cleanup functions
  void
  clean(void)
  {
    micron::lock_guard l(mtx);
    int r = 0;
    // NOTE: think about checking if thread is alive first before try_joining, although i believe the end result is the
    // same
    while ( !threads.empty() ) {
      // the thread might be busy, in which case stall until it ends operation
      if ( r = solo::try_join(threads.top().thread); r == error::busy ) {
        cpu_pause();
        continue;
      } else if ( r == error::invalid_arg ) {
        micron::abort();
      }
      threads.pop();
    }
  }
  void
  force_clean(void)
  {
    micron::lock_guard l(mtx);
    int r = 0;
    while ( !threads.empty() ) {
      // the thread might be busy, in which case stall until it ends operation
      if ( r = solo::try_join(threads.top().thread); r == error::busy ) {
        solo::terminate(threads.top().thread);
      } else if ( r == error::invalid_arg )
        micron::abort();
      threads.pop();
    }
  }

  // NOTE: retries technically isn't forever, but it works
  i32
  join_all(u64 retries = numeric_limits<u64>::max())
  {
    micron::lock_guard l(mtx);
    int r = 0;

    while ( !threads.empty() ) {
      if ( threads.top().thread.alive() ) {
      retry_join:
        if ( r = solo::try_join(threads.top().thread); r == error::busy ) {
          cpu_pause();
          if ( --retries != 0 )
            goto retry_join;
          else
            goto force_join;
        } else if ( r == error::invalid_arg ) {
          return r;
        } else if ( r == 0 or r == error::no_process ) {
          threads.pop();
        }
      } else {
      force_join:
        solo::join(threads.top().thread);
        threads.pop();
      }
    }
    return 0;
  }
  i32
  join(pthread_t pid, u64 retries = numeric_limits<u64>::max())
  {
    // don't pop
    micron::lock_guard l(mtx);
    int r = 0;

    for ( u64 i = 0; i < threads.size(); ++i ) {
      if ( threads[i].thread.native_handle() == pid ) {
      retry_join:
        if ( r = solo::try_join(threads.top().thread); r == error::busy ) {
          cpu_pause();
          if ( --retries != 0 )
            goto retry_join;
        }
      } else if ( r == error::invalid_arg ) {
        return r;
      } else if ( r == 0 or r == error::no_process ) {
        return 0;
      }
    }
    return 0;
  }

  size_t
  count() const
  {
    return threads.size();
  }
  void
  lower_priority(thread_t<Tr> &rf, const int n = 1)
  {
    micron::lock_guard l(mtx);
    micron::set_priority(rf.priority + n, rf.thread.thread_id());
  }
  void
  increase_priority(thread_t<Tr> &rf, const int n = 1)
  {
    micron::lock_guard l(mtx);
    micron::set_priority(rf.priority - n, rf.thread.thread_id());
    // NOTE: can't go below 0 without root or CAP_SYS_NICE
  }
  // check if arena controls thread
  bool
  contains(const thread_t<Tr> &t)
  {
    micron::lock_guard l(mtx);
    for ( u64 i = 0; i < threads.size(); ++i )
      if ( threads[i].thread.native_handle() == t.native_handle() )
        return true;
    return false;
  }
  bool
  contains(const pthread_t pid)
  {
    micron::lock_guard l(mtx);
    for ( u64 i = 0; i < threads.size(); ++i )
      if ( threads[i].thread.native_handle() == pid )
        return true;
    return false;
  }
};

template <typename Tr = thread<thread_stack_size>>
  requires(!micron::is_same_v<Tr, auto_thread<>>)     // we don't want to use auto_threads in an arena
class __parallel_arena
{
  micron::fsstack<thread_t<Tr>, maximum_threads> threads;
  micron::mutex mtx;

  addr_t *
  __create_stack(void) const
  {
    addr_t *fstack = micron::addrmap(thread_stack_size);
    if ( micron::mmap_failed(fstack) )
      throw except::thread_error("micron arena::__create_stack(): failed to allocate stack");
    return fstack;
  }

public:
  ~__parallel_arena()
  {
    // forcing stops
    force_clean();
  }
  __parallel_arena(void) : threads() {}
  __parallel_arena(const __parallel_arena &) = delete;
  __parallel_arena(__parallel_arena &&) = delete;
  __parallel_arena &operator=(const __parallel_arena &) = delete;
  __parallel_arena &operator=(__parallel_arena &&) = delete;
  // thread creation functions
  // Create a new thread automatically, without any config. Uses generally standard best practices config options
  template <typename Func, typename... Args>
    requires(micron::is_invocable_v<Func, Args...>)
  auto &
  create(Func f, Args &&...args)
  {
    micron::lock_guard l(mtx);
    // safety check
    if ( threads.full_or_overflowed() ) [[unlikely]]
      throw except::thread_error("micron arena::create(): Arena is full or corrupted");
    addr_t *stack_ptr = __create_stack();
    pthread_attr_t attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);
    pthread::set_stack_thread(attrs, stack_ptr, thread_stack_size);
    threads.emplace(posix::getuid(), attrs, cpu_t<true>(), Tr{ stack_ptr, attrs, f, micron::forward<Args>(args)... });
    // posix::get_priority(prio_process, 0)
    return threads.top();
  }
  // Create a new thread at a specific core/unit
  template <typename Func, typename... Args>
    requires(micron::is_invocable_v<Func, Args...>)
  auto &
  create_at(size_t n, Func f, Args &&...args)
  {
    micron::lock_guard l(mtx);
    if ( threads.full_or_overflowed() ) [[unlikely]]
      throw except::thread_error("micron arena::create(): Arena is full or corrupted");
    cpu_t<true> c;
    c.clear();
    c.set_core(n);
    addr_t *stack_ptr = __create_stack();
    pthread_attr_t attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);
    pthread::set_stack_thread(attrs, stack_ptr, thread_stack_size);
    threads.emplace(posix::getuid(), attrs, c, Tr{ stack_ptr, micron::move(attrs), f, micron::forward<Args>(args)... });

    // threads.emplace(posix::getuid(), posix::get_priority(prio_process, 0), c, Tr{ f, micron::forward<Args>(args)...
    // });
    posix::sched_setaffinity(threads.top().thread.thread_id(), sizeof(c.get()), c.get());
    return threads.top();
  }
  // Create a new thread at a specific core that is currently more free than the rest
  template <typename Func, typename... Args>
    requires(micron::is_invocable_v<Func, Args...>)
  auto &
  create_burden(Func f, Args &&...args)
  {
    micron::lock_guard l(mtx);
    if ( threads.full_or_overflowed() ) [[unlikely]]
      throw except::thread_error("micron arena::create(): Arena is full or corrupted");
    fvector<i32> cores(cpu_count());

    cpu_t<true> c;
    for ( u64 i = 0; i < threads.size(); ++i ) {
      c.clear();
      posix::sched_getaffinity(threads[i].thread.thread_id(), sizeof(c.get()), c.get());
      for ( u64 k = 0; k < cores.size(); ++k )
        if ( c[k] )
          cores[k]++;
    }

    fvector<i32>::const_iterator smallest = min_at(cores);
    if ( smallest != cores.cbegin() ) {
      c.clear();
      c.set_core(smallest - cores.cbegin());
      goto start_thread;
    }
    c.clear();
    c.set_core(0);
  start_thread:
    addr_t *stack_ptr = __create_stack();
    pthread_attr_t attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);
    pthread::set_stack_thread(attrs, stack_ptr, thread_stack_size);
    threads.emplace(posix::getuid(), attrs, c, Tr{ stack_ptr, micron::move(attrs), f, micron::forward<Args>(args)... });

    // threads.emplace(posix::getuid(), posix::get_priority(prio_process, 0), c, Tr{ f, micron::forward<Args>(args)...
    // });
    posix::sched_setaffinity(threads.top().thread.thread_id(), sizeof(c.get()), c.get());
    return threads.top();
  }

  // cleanup functions
  void
  clean(void)
  {
    micron::lock_guard l(mtx);
    int r = 0;
    // NOTE: think about checking if thread is alive first before try_joining, although i believe the end result is the
    // same
    while ( !threads.empty() ) {
      // the thread might be busy, in which case stall until it ends operation
      if ( r = solo::try_join(threads.top().thread); r == error::busy ) {
        cpu_pause();
        continue;
      } else if ( r == error::invalid_arg ) {
        micron::abort();
      }
      threads.pop();
    }
  }
  void
  force_clean(void)
  {
    micron::lock_guard l(mtx);
    int r = 0;
    while ( !threads.empty() ) {
      // the thread might be busy, in which case stall until it ends operation
      if ( r = solo::try_join(threads.top().thread); r == error::busy ) {
        solo::terminate(threads.top().thread);
      } else if ( r == error::invalid_arg )
        micron::abort();
      threads.pop();
    }
  }

  // NOTE: retries technically isn't forever, but it works
  i32
  join_all(u64 retries = numeric_limits<u64>::max())
  {
    micron::lock_guard l(mtx);
    int r = 0;

    while ( !threads.empty() ) {
      if ( threads.top().thread.alive() ) {
      retry_join:
        if ( r = solo::try_join(threads.top().thread); r == error::busy ) {
          cpu_pause();
          if ( --retries != 0 )
            goto retry_join;
          else
            goto force_join;
        } else if ( r == error::invalid_arg ) {
          return r;
        } else if ( r == 0 or r == error::no_process ) {
          threads.pop();
        }
      } else {
      force_join:
        solo::join(threads.top().thread);
        threads.pop();
      }
    }
    return 0;
  }
  i32
  join(pthread_t pid, u64 retries = numeric_limits<u64>::max())
  {
    // don't pop
    micron::lock_guard l(mtx);
    int r = 0;

    for ( u64 i = 0; i < threads.size(); ++i ) {
      if ( threads[i].thread.native_handle() == pid ) {
      retry_join:
        if ( r = solo::try_join(threads.top().thread); r == error::busy ) {
          cpu_pause();
          if ( --retries != 0 )
            goto retry_join;
        }
      } else if ( r == error::invalid_arg ) {
        return r;
      } else if ( r == 0 or r == error::no_process ) {
        return 0;
      }
    }
    return 0;
  }

  size_t
  count() const
  {
    return threads.size();
  }
  void
  lower_priority(thread_t<Tr> &rf, const int n = 1)
  {
    micron::lock_guard l(mtx);
    micron::set_priority(rf.priority + n, rf.thread.thread_id());
  }
  void
  increase_priority(thread_t<Tr> &rf, const int n = 1)
  {
    micron::lock_guard l(mtx);
    micron::set_priority(rf.priority - n, rf.thread.thread_id());
    // NOTE: can't go below 0 without root or CAP_SYS_NICE
  }
  // check if arena controls thread
  bool
  contains(const thread_t<Tr> &t)
  {
    micron::lock_guard l(mtx);
    for ( u64 i = 0; i < threads.size(); ++i )
      if ( threads[i].thread.native_handle() == t.native_handle() )
        return true;
    return false;
  }
  bool
  contains(const pthread_t pid)
  {
    micron::lock_guard l(mtx);
    for ( u64 i = 0; i < threads.size(); ++i )
      if ( threads[i].thread.native_handle() == pid )
        return true;
    return false;
  }
};

// for async tasks
class __concurrent_arena
{
};
};

};
