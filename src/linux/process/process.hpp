//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// libelves

#include "../process/spawn.hpp"
#include "../sys/clone.hpp"
#include "../sys/limits.hpp"

#include "../../pointer.hpp"
#include "../io.hpp"
#include "../sys/fcntl.hpp"

#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../memory/actions.hpp"
#include "../../memory/mman.hpp"
#include "../../memory/stack_constants.hpp"
#include "../../string/strings.hpp"
#include "../../vector/fvector.hpp"
#include "../../vector/svector.hpp"
#include "../../vector/vector.hpp"
#include "../__includes.hpp"
#include "fork.hpp"
#include "wait.hpp"

#include "../../io/filesystem.hpp"

#include "callbacks.hpp"

extern char **environ;

namespace micron
{

/*

   // TODO: add this

   struct uelf_t {
  int fd;
  byte *elf;
};

template <is_string S>
uelf_t
create_elf_memory(uelf_t &elf, const S &str, const S &str_)
{
  //uelf_t elf{
  //  memfd_create(str.c_str(), 0),
  //};
}*/

struct upid_t {
  uid_t uid;
  uid_t euid;
  gid_t gid;
  pid_t pid;
  pid_t ppid;
};

// TODO: extend with mmap regions
struct runtime_t {
  byte *stack;
  byte *heap;
};

inline runtime_t
load_stack_heap(void)
{
  micron::string dt;
  fsys::system<micron::io::rd> sys;
  io::path_t path = "/proc/self/maps";
  sys["/proc/self/maps"] >> dt;
  micron::string::iterator stck = micron::format::find(dt, "[stack]");
  micron::string::iterator heap = micron::format::find(dt, "[heap]");
  // NOTE: the reason this is here is because we might not always have a heap, in which case heap will be nullptr
  if ( !stck and !heap ) {
    runtime_t rt = { .stack = nullptr, .heap = nullptr };
    return rt;
  } else if ( heap ) {
    micron::string::iterator stck_nl = micron::format::find_reverse(dt, stck, "\n") + 1;
    micron::string::iterator heap_nl = micron::format::find_reverse(dt, heap, "\n") + 1;
    micron::string::iterator stck_ptr = micron::format::find(dt, stck_nl, "-");
    micron::string::iterator heap_ptr = micron::format::find(dt, heap_nl, "-");
    byte *sptr = micron::format::to_pointer_addr<byte, micron::string>(stck_nl, stck_ptr);
    byte *hptr = micron::format::to_pointer_addr<byte, micron::string>(heap_nl, heap_ptr);
    runtime_t rt = { .stack = sptr, .heap = hptr };
    return rt;
  } else {
    micron::string::iterator stck_nl = micron::format::find_reverse(dt, stck, "\n") + 1;
    micron::string::iterator stck_ptr = micron::format::find(dt, stck_nl, "-");
    byte *sptr = micron::format::to_pointer_addr<byte, micron::string>(stck_nl, stck_ptr);
    runtime_t rt = { .stack = sptr, .heap = nullptr };
    return rt;
  }
  runtime_t rt = { .stack = nullptr, .heap = nullptr };
  return rt;
}

micron::ptr_arr<char *>
vector_to_argv(const micron::svector<micron::string> &vec)
{
  auto argv = micron::unique_arr<char *>(vec.size() + 1);     // for nullptr
  for ( size_t i = 0; i < vec.size(); ++i ) {
    argv[i] = const_cast<char *>(vec[i].c_str());
  }
  argv[vec.size()] = nullptr;
  return argv;
}

// uses containers rather than PODs, redesigned with spawn_ctx as info
struct uprocess_t {
  upid_t pids;     // pid of process
  micron::sstring<posix::path_max + 1> path;
  micron::svector<micron::string> argv;     // can be variable, up to cca 2 MiB
  micron::svector<micron::string> envp;
  posix::limits_t lims;     // limits of the process, slower to get, but makes handling effortless
  posix::cpu_set_t affinity;
  int status;
  ~uprocess_t() = default;

  uprocess_t(void)
      : pids{ posix::getuid(), posix::geteuid(), posix::getgid(), posix::getpid(), posix::getppid() }, path(), argv{}, envp{}, lims(0),
        affinity(), status(0)
  {
    // programatically get argv and environ
    micron::string str;
    fsys::system<micron::io::rd> sys;
    sys["/proc/self/cmdline"] >> str;
    umax_t k = 0;
    umax_t t = 0;
    // stored as null term strings, so we're iterating according to what we read
    for ( umax_t i = 0; i < str.size(); ++i ) {
      if ( str[i] == 0x0 ) {
        t = i;
        argv.emplace_back(str.begin() + k, str.begin() + t);
        k = t + 1;
      }
    }
    // argv[0] is always path, not absolute but works without reparsing
    path = argv[0];
    str.clear();
    k = 0;
    t = 0;
    sys["/proc/self/environ"] >> str;
    for ( umax_t i = 0; i < str.size(); ++i ) {
      if ( str[i] == 0x0 ) {
        t = i;
        envp.emplace_back(str.begin() + k, str.begin() + t);
        // our vector can only hold 64 elements, and as it turns out environ could easily exceed that
        if ( envp.full_or_overflowed() )
          break;
        k = t + 1;
      }
    }
    posix::get_affinity(pids.pid, affinity);
  }

  template <typename... Args>
  uprocess_t(const char *str, Args &&...args)
      : pids{}, path(str), argv{ micron::forward<Args>(args)... }, envp{}, lims(0), affinity(), status(0)
  {
  }

  template <typename... Args>
  uprocess_t(micron::sstring<posix::path_max + 1> &&o, Args &&...args)
      : pids{}, path(micron::move(o)), argv{ micron::forward<Args>(args)... }, envp{}, lims(0), affinity(), status(0)
  {
  }

  uprocess_t(const uprocess_t &o)
      : pids(o.pids), path(o.path), argv(o.argv), envp(o.envp), lims(o.lims), affinity(o.affinity), status(o.status)
  {
  }

  uprocess_t(uprocess_t &&o)
      : pids(micron::move(o.pids)), path(micron::move(o.path)), argv(micron::move(o.argv)), envp(micron::move(o.envp)),
        lims(micron::move(o.lims)), affinity(micron::move(o.affinity)), status(o.status)
  {
    o.status = 0;
  }

  template <typename... Args>
  uprocess_t &
  operator=(uprocess_t &&o)
  {

    pids = micron::move(o.pids);
    path = micron::move(o.path);
    argv = micron::move(o.argv);
    envp = micron::move(o.envp);
    lims = micron::move(o.lims);
    affinity = micron::move(o.affinity);
    status = o.status;
    o.status = 0;
    return *this;
  }

  uprocess_t &
  operator=(const uprocess_t &o)
  {
    pids = o.pids;
    path = o.path;
    argv = o.argv;
    envp = o.envp;
    lims = o.lims;
    affinity = o.affinity;
    status = o.status;
    return *this;
  }
};

typedef micron::fvector<uprocess_t> process_list_t;

template <is_string... S>
process_list_t
create_processes(S... names)
{
  process_list_t p;
  (p.emplace_back(names, names), ...);
  return p;
}

template <typename... S>
process_list_t
create_processes(S... names)
{
  process_list_t p;
  (p.emplace_back(names, names), ...);
  return p;
}

void
run_processes(process_list_t &n)
{
  for ( auto &t : n ) {
    micron::svector<char *> argv;
    for ( size_t i = 0; i < t.argv.size(); i++ )
      argv.push_back(&t.argv[i][0]);
    argv.push_back(nullptr);
    t.pids.uid = posix::getuid();
    t.pids.gid = posix::getgid();
    if ( micron::spawn(t.pids.pid, t.path.c_str(), &argv[0], environ) ) {
      exc<except::system_error>("micron process failed to start spawn");
    }
  }
}

// implementation of daemon
template <int Stack = default_stack_size, typename F, typename... Args>
  requires micron::is_function_v<F> or micron::is_function_v<micron::remove_pointer_t<F>>
int
daemon(F f, Args &&...args)
{
  int pid = micron::fork();     // much nicer to do this
  if ( pid > 0 )                // parent
    _Exit(0);
  if ( posix::setsid() < 0 )
    exc<except::runtime_error>("micron process daemon failed to create new session");
  // don't change dir
  micron::umask(0);
  micron::close(stdin_fileno);
  micron::close(stdout_fileno);
  micron::close(stderr_fileno);

  micron::open("/dev/null", o_rdwr);
  micron::dup(0);
  micron::dup(0);
  f(args...);
  return 0;
}

// ex this_task
uprocess_t
this_process(void)
{
  return uprocess_t();
};

};
