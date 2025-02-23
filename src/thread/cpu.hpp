#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "scheduling.hpp"

#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include "posix/system.hpp"

#include "../../external/cpuid.h"
#include "../io/console.hpp"
#include "../io/filesystem.hpp"
#include "../slice.hpp"
#include "../string/strings.h"
#include "../string/unistring.hpp"
#include "../types.hpp"

// the definitive header file including all general CPU handling code (affinity, priority, parking cores, thread maps
// etc)

namespace micron
{

// the current runtime
// we won't get end segments due to them changing all the time
struct runtime_t {
  byte *stack;
  byte *heap;
};

struct task_t : public runtime_t {
  posix::pid_t pid;
  posix::pid_t ppid;
  posix::uid_t uid;
  posix::uid_t euid;
  posix::gid_t gid;
  long int priority;
  cpu_set_t affinity;

  task_t(void)
      : runtime_t{ .stack = nullptr, .heap = nullptr }, pid(posix::getpid()), ppid(posix::getppid()),
        uid(posix::getuid()), euid(posix::geteuid()), gid(posix::getgid()), priority(0)
  {
    CPU_ZERO(&affinity);
  }
};

inline runtime_t
load_stack_heap(void)
{
  micron::ustr8 dt;
  fsys::system<micron::io::rd> sys;
  io::path_t path = "/proc/self/maps";
  // path += int_to_string_stack<umax_t, char, 256>(this_pid());
  // path += "/limits";
  sys["/proc/self/maps"] >> dt;
  micron::ustr8::iterator stck = micron::format::find(dt, "[stack]");
  micron::ustr8::iterator heap = micron::format::find(dt, "[heap]");
  // NOTE: the reason this is here is because we might not always have a heap, in which case heap will be nullptr
  if ( !stck and !heap ) {
    runtime_t rt = { .stack = nullptr, .heap = nullptr };
    return rt;
  } else if ( heap ) {
    micron::ustr8::iterator stck_nl = micron::format::find_reverse(dt, stck, "\n") + 1;
    micron::ustr8::iterator heap_nl = micron::format::find_reverse(dt, heap, "\n") + 1;
    micron::ustr8::iterator stck_ptr = micron::format::find(dt, stck_nl, "-");
    micron::ustr8::iterator heap_ptr = micron::format::find(dt, heap_nl, "-");
    byte *sptr = micron::format::to_pointer_addr<byte, micron::ustr8>(stck_nl, stck_ptr);
    byte *hptr = micron::format::to_pointer_addr<byte, micron::ustr8>(heap_nl, heap_ptr);
    runtime_t rt = { .stack = sptr, .heap = hptr };
    return rt;
  } else {
    micron::ustr8::iterator stck_nl = micron::format::find_reverse(dt, stck, "\n") + 1;
    micron::ustr8::iterator stck_ptr = micron::format::find(dt, stck_nl, "-");
    byte *sptr = micron::format::to_pointer_addr<byte, micron::ustr8>(stck_nl, stck_ptr);
    runtime_t rt = { .stack = sptr, .heap = nullptr };
    return rt;
  }
  runtime_t rt = { .stack = nullptr, .heap = nullptr };
  return rt;
}

inline unsigned
cpu_count(void)
{
  return (unsigned)maximum_leaf();
}

inline unsigned
which_cpu(void)
{
  unsigned c = 0;
  ::getcpu(&c, NULL);     // use getcpu instead of sched_getcpu
  return c;
}

inline void
set_priority(int prio, int pid = posix::getpid())
{
  posix::set_priority(PRIO_PROCESS, pid, prio);
  //::setpriority(PRIO_PROCESS, pid, prio);
}

inline unsigned
park_cpu(unsigned c = 0xFFFFFFFF)
{
  if ( c == 0xFFFFFFFF ) {
    ::getcpu(&c, NULL);     // use getcpu instead of sched_getcpu
  }
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(c, &set);
  ::sched_setaffinity(0, sizeof(cpu_set_t), &set);
  return c;
}

inline unsigned
park_cpu_pid(pid_t pid, unsigned c = 0xFFFFFFFF)
{
  if ( c == 0xFFFFFFFF ) {
    ::getcpu(&c, NULL);     // use getcpu instead of sched_getcpu
  }
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(c, &set);
  ::sched_setaffinity(pid, sizeof(cpu_set_t), &set);
  return c;
}
inline void
park_cpu(pid_t pid, cpu_set_t &set)
{
  ::sched_setaffinity(pid, sizeof(cpu_set_t), &set);
}

inline void
enable_all_cores(pid_t pid = 0)
{
  cpu_set_t set;
  CPU_ZERO(&set);
  auto count = cpu_count();
  for ( size_t i = 0; i <= count; i++ )
    CPU_SET(i, &set);
  ::sched_setaffinity(pid, sizeof(cpu_set_t), &set);
}
task_t
this_task(void)
{
  auto rt = load_stack_heap();
  // get current task
  auto this_pid = posix::getpid();
  task_t task;
  task.stack = rt.stack;
  task.heap = rt.heap;
  task.pid = this_pid;
  task.ppid = posix::getppid();
  task.uid = posix::getuid();
  task.euid = posix::geteuid();
  task.gid = posix::getgid();
  task.priority = posix::get_priority(PRIO_PROCESS, this_pid);     //::getpriority(PRIO_PROCESS, this_pid);
  posix::get_affinity(this_pid, task.affinity);
  // sched_getaffinity(this_pid, sizeof(task.affinity), &task.affinity);
  return task;
}

constexpr int cpu_setsize = 1024;
constexpr int cpu_bits = (8 * sizeof(__cpu_mask));
using cpu_mask_t = __cpu_mask;

template <bool D = false> class cpu_t : public scheduler_t
{
  pid_t pid;
  cpu_set_t procs;
  unsigned count;

public:
  cpu_t(void) : scheduler_t(posix::getpid()), pid(posix::getpid()), count(cpu_count())
  {
    CPU_ZERO(&procs);     // for those wondering this is abysmally designed so we have to do it this way
    if constexpr ( D ) {
      for ( size_t i = 0; i <= count; i++ )
        CPU_SET(i, &procs);
    }     // default to all threads
  }
  cpu_mask_t
  operator[](const size_t n) const
  {
    if ( n >= count )
      throw except::library_error("micron cpu_t::operator[] out of range");
    return CPU_ISSET(n, &procs);
  }
  void
  clear(void)
  {
    CPU_ZERO(&procs);
  }
  cpu_mask_t
  at(const size_t n) const
  {
    if ( n >= count )
      throw except::library_error("micron cpu_t::operator[] out of range");
    return CPU_ISSET(n, &procs);
  }

  cpu_mask_t
  set_core(const size_t n)
  {
    if ( n >= count )
      throw except::library_error("micron cpu_t::operator[] out of range");
    return CPU_SET(n, &procs);
  }
  void
  update_cores(void)
  {
    posix::set_affinity(pid, procs);
  }
  void
  update(void)
  {
    enable_all_cores(pid);
    load_scheduler(pid);
    posix::set_affinity(pid, procs);
  }
};

};
