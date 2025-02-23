#pragma once

#include "../mutex/locks.hpp"
#include "../stack.hpp"
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
#ifdef ABSURD_THREAD_COUNT
const static constexpr int maximum_threads = 65536;
#elseif HIGH_THREAD_COUNT
const static constexpr int maximum_threads = 4096;
#else
const static constexpr int maximum_threads = 1024;
#endif
template <typename Tr> struct thread_t {
  uid_t uid;                // NOTE: requires CAP_SETUID
  int priority;             // niceness
  cpu_t<true> cpu_mask;     // cpu mask for that specific thread
  __thread_pointer<Tr> thread;
};

// NOTE: not for external usage, always call from /spawn.hpp
namespace arena
{
class __empty_arena
{
};
template <typename Tr = thread<>>
  requires(!std::is_same_v<Tr, auto_thread<>>)     // we don't want to use auto_threads in an arena
class __default_arena
{
  micron::sstack<thread_t<Tr>, maximum_threads> threads;
  micron::mutex mtx;

public:
  ~__default_arena()
  {
    // this makes sure all the threads are cleaned up properly (you can remove this for a perf bump, but depending on the
    // type of Tr, it might not be the best idea)
    for ( size_t k = 0; k < threads.size(); k++ )
      solo::join(threads[k].thread);
  }
  __default_arena(void) : threads() {}
  __default_arena(const __default_arena &) = delete;
  __default_arena(__default_arena &&) = delete;
  __default_arena &operator=(const __default_arena &) = delete;
  __default_arena &operator=(__default_arena &&) = delete;
  template <typename Func, typename... Args>
    requires(std::is_invocable_v<Func, Args...>)
  auto&
  create(Func f, Args... args)
  {
    micron::lock_guard l(mtx);
    // NOTE: "A child created by fork(2) inherits its parent's nice value.  The nice value is preserved across execve(2)"
    threads.push({ .uid = ::syscall(SYS_getuid),
                   .priority = posix::get_priority(PRIO_PROCESS, 0),
                   .cpu_mask = {},
                   .thread = solo::spawn<Tr>(f, args...) });
    return threads.top();
  }
  template <typename Func, typename... Args>
    requires(std::is_invocable_v<Func, Args...>)
  auto&
  create_at(size_t n, Func f, Args... args)
  {
    micron::lock_guard l(mtx);
    cpu_t<true> c;
    c.clear();
    c.set_core(n);
    threads.push({ .uid = posix::getuid(),
                   .priority = posix::get_priority(PRIO_PROCESS, 0),
                   .cpu_mask = c,
                   .thread = solo::spawn<Tr>(f, args...) });
    ::syscall(SYS_sched_setaffinity, threads.top().thread->thread_id(), sizeof(c), &c);
    return threads.top();
  }
  void
  clean(void)
  {
    micron::lock_guard l(mtx);
    while ( !threads.empty() ) {
      if ( !solo::is_joinable(threads.top().thread) )
        return;
      threads.pop();
    }
  }
  void
  lower_priority(thread_t<Tr>& rf, const int n = 1) {
    micron::lock_guard l(mtx);
    micron::set_priority(rf.priority + n, rf.thread->thread_id());
  }
  void
  increase_priority(thread_t<Tr>& rf, const int n = 1) {
    micron::lock_guard l(mtx);
    micron::set_priority(rf.priority - n, rf.thread->thread_id());
    // NOTE: can't go below 0 without root or CAP_SYS_NICE
  }
};
// for parallel structures
class __parallel_arena
{
};
// for async tasks
class __concurrent_arena
{
};
};

using standard_arena = arena::__default_arena<micron::group_thread<>>;

template <typename A = arena::__default_arena<>> void make_arena() {};

};
