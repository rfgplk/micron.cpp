//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
/*
#include <spawn.h>
#include <sys/wait.h>     // waitpid
#include <sys/stat.h>
#include <unistd.h>       // fork, close, daemon
#include <fcntl.h>

#include "../concepts.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../string/strings.h"
#include "../vector/fvector.hpp"
#include "../vector/svector.hpp"

#include "callbacks.hpp"

// fork clone daemon execve mmap functionality
// you might be wondering why not roll your own posix calls for process spawning
// the reason is, it's simply too tedious to get it working on a wide range of machines, and there is a LOT of
// boilterplate code, which would make the final result more or less identical to the system version

extern char **environ;
namespace micron
{

constexpr int default_stack_size = 10 * 1024 * 1024; // 10MB default on Linux
constexpr int auto_thread_stack_size = 2 * 1024 * 1024; // 2MB
constexpr int small_stack_size = 1024 * 1024; // 1MB
constexpr int micro_stack_size = 256 * 1024; // 256KB

// more legible flags
enum class clone_flags : u32 {
  clear_tid = CLONE_CHILD_CLEARTID,
  set_tid = CLONE_CHILD_SETTID,
  detached_obsolete = CLONE_DETACHED,
  new_cgroup = CLONE_NEWCGROUP,
  copy_files = CLONE_FILES,
  same_io = CLONE_IO,
  newcgroup = CLONE_NEWCGROUP,
  newipc = CLONE_NEWIPC,
  newnet = CLONE_NEWNET,
  newns = CLONE_NEWNS,
  newpid = CLONE_NEWPID,
  newuser = CLONE_NEWUSER,
  parent_copy = CLONE_PARENT,
  parent_settid = CLONE_PARENT_SETTID,
  pid_pointer = CLONE_PIDFD,
  ptrace = CLONE_PTRACE,
  enable_tls = CLONE_SETTLS,
  share_signals = CLONE_SIGHAND,
  start_stopped_removed = 0,
  share_semaphores = CLONE_SYSVSEM,
  as_thread = CLONE_THREAD,
  no_trace = CLONE_UNTRACED,
  vfork_obsolete = CLONE_VFORK,
  same_memory = CLONE_VM
};

enum class posix_process_flags {
  sig_mask = POSIX_SPAWN_SETSIGMASK,
  sig_def = POSIX_SPAWN_SETSIGDEF,
  sched_param = POSIX_SPAWN_SETSCHEDPARAM,
  set_scheduler = POSIX_SPAWN_SETSCHEDULER,
  reset_ids = POSIX_SPAWN_RESETIDS,
  set_pgroup = POSIX_SPAWN_SETPGROUP,
  new_session = POSIX_SPAWN_SETSID
};

struct uprocess_t {
  uid_t uid;
  gid_t gid;
  pid_t pid;
  micron::string path;
  micron::svector<micron::string> argv;
  posix_spawnattr_t flags;
  int status;
  //bool wait;
  template <typename... Args>
  uprocess_t(const char *str, Args... args) : pid(0), uid(0), gid(0), path(str), argv({ args... }), status(0)//, wait(true)
  {
    if ( posix_spawnattr_init(&flags) != 0 ) {
      throw except::system_error("micron process failed to init spawnattrs");
    }
  }

  template <typename... Args>
  uprocess_t(micron::string &&o, Args... args) : pid(0), uid(0), gid(0), path(micron::move(o)), argv({ args... }), status(0)//, wait(true)
  {
    if ( posix_spawnattr_init(&flags) != 0 ) {
      throw except::system_error("micron process failed to init spawnattrs");
    }
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
    t.uid = posix::getuid();
    t.gid = posix::getgid();
    if ( ::posix_spawn(&t.pid, t.path.c_str(), NULL, &t.flags, &argv[0], environ) ) {
      throw except::system_error("micron process failed to start posix_spawn");
    }
    //if ( t.wait )
    //  ::waitpid(t.pid, &t.status, 0);
  }
}

void
process(uprocess_t &t)
{
  micron::svector<char *> argv;
  for ( size_t i = 0; i < t.argv.size(); i++ )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  t.uid = posix::getuid();
  t.gid = posix::getgid();
  if ( ::posix_spawn(&t.pid, t.path.c_str(), NULL, &t.flags, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  //if ( t.wait )
  //  ::waitpid(t.pid, &t.status, 0);
}

// fork and run process at path location specified by T
template <bool W = false, is_string T>
void
process(const T &t)
{
  pid_t pid;
  int status = 0;
  posix_spawnattr_t flags;
  if ( posix_spawnattr_init(&flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::svector<char *> argv = { &t[0], nullptr };
  if ( ::posix_spawn(&pid, t.c_str(), NULL, &flags, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    ::waitpid(pid, &status, 0);
}

// fork and run process at path location specified by T
template <bool W = false, is_string T, is_string... R>
void
process(const T &t, const R &...args)
{
  pid_t pid;
  int status = 0;
  posix_spawnattr_t flags;
  if ( posix_spawnattr_init(&flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::svector<char *> argv = { &t[0], nullptr };

  (argv.insert(argv.begin() + 1, &args[0]), ...);

  if ( ::posix_spawn(&pid, t.c_str(), NULL, &flags, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    ::waitpid(pid, &status, 0);
}

template <bool W = false>
void
process(const char *t)
{
  pid_t pid;
  int status = 0;
  posix_spawnattr_t flags;
  if ( posix_spawnattr_init(&flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::svector<char *> argv = { const_cast<char *>(t), nullptr };
  if ( ::posix_spawn(&pid, t, NULL, &flags, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    ::waitpid(pid, &status, 0);
}

template <bool W = false, typename... R>
void
process(const char *t, const R*... args)
{
  pid_t pid;
  int status = 0;
  posix_spawnattr_t flags;
  if ( posix_spawnattr_init(&flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::svector<char *> argv = { const_cast<char *>(t), nullptr };
  
  (argv.insert(argv.begin() + 1, const_cast<char*>(args)), ...);
  
  if ( ::posix_spawn(&pid, t, NULL, &flags, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    ::waitpid(pid, &status, 0);
}

// implementation of daemon
template <int Stack = default_stack_size, typename F, typename... Args>
  requires std::is_function_v<F> or std::is_function_v<std::remove_pointer_t<F>>
int
daemon(F f, Args&&... args)
{
  int pid = ::fork(); // much nicer to do this
  if ( pid > 0 )     // parent
    _Exit(0);
  if ( setsid() < 0 )
    throw except::runtime_error("micron process daemon failed to create new session");
  // don't change dir
  ::umask(0);
  posix::close(STDIN_FILENO);
  posix::close(STDOUT_FILENO);
  posix::close(STDERR_FILENO);

  ::open("/dev/null", O_RDWR);
  ::dup(0);
  ::dup(0);
  f(args...);
  return 0;
}

// implementation of ::fork()
template <int Stack = default_stack_size>
int
dead_fork()
{
  char *fstack = reinterpret_cast<char *>(
      mmap(NULL, Stack, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));
  if ( fstack == MAP_FAILED )
    throw except::system_error("micron process mmap failed to allocate stack");
  int pid = ::clone(__default_callback, fstack + Stack,
                    CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_PARENT | SIGCHLD, NULL);
  if ( pid == -1 )
    throw except::system_error("micron process failed to fork()");
  return pid;
}

// implementation of ::fork()
template <int Stack = default_stack_size>
int
wdead_fork()
{
  char *fstack = reinterpret_cast<char *>(
      mmap(NULL, Stack, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));
  if ( fstack == MAP_FAILED )
    throw except::system_error("micron process mmap failed to allocate stack");
  int pid = ::clone(__default_sleep_callback, fstack + Stack,
                    CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_PARENT | SIGCHLD, NULL);
  if ( pid == -1 )
    throw except::system_error("micron process failed to fork()");
  return pid;
}

// implementation of ::fork()
template <int Stack = default_stack_size>
int
fork()
{

  char *fstack = reinterpret_cast<char *>(
      mmap(NULL, Stack, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));
  if ( fstack == MAP_FAILED )
    throw except::system_error("micron process mmap failed to allocate stack");
  int pid = ::clone(__default_callback, fstack + Stack,
                    CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_PARENT | SIGCHLD, NULL);
  if ( pid == -1 )
    throw except::system_error("micron process failed to fork()");
  return pid;
}

// implementation of ::fork() but automatically wait for child proc to end
template <int Stack = default_stack_size>
int
wfork()
{
  char *fstack = reinterpret_cast<char *>(
      mmap(NULL, Stack, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));
  if ( fstack == MAP_FAILED )
    throw except::system_error("micron process mmap failed to allocate stack");
  int pid = ::clone(__default_callback, fstack + Stack,
                    CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD, NULL);
  if ( pid == -1 )
    throw except::system_error("micron process failed to fork()");
  if ( pid == 0 )
    return pid;
  int status = 0;
  ::waitpid(pid, &status, 0);
  return status;
}
// implementation of ::fork() but automatically wait for child proc to end
template <int Stack = default_stack_size>
int
memfork()
{
  char *fstack = reinterpret_cast<char *>(
      mmap(NULL, Stack, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));
  if ( fstack == MAP_FAILED )
    throw except::system_error("micron process mmap failed to allocate stack");
  int pid = ::clone(__default_callback, fstack + Stack, CLONE_FS | CLONE_FILES | CLONE_PARENT | SIGCHLD, NULL);
  if ( pid == -1 )
    throw except::system_error("micron process failed to fork()");
  return pid;
}
};*/
