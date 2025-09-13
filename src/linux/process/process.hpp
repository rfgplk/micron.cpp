//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// libelves

#include <sched.h> /* Definition of CLONE_* constants */
#include <spawn.h>
//#include <sys/stat.h>
//#include <sys/wait.h>     // waitpid
#include <unistd.h>       // fork, close, daemon

#include "../../thread/signal.hpp"

#include "../sys/fcntl.hpp"
#include "../io.hpp"

#include "wait.hpp"
#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../memory/actions.hpp"
#include "../../memory/mman.hpp"
#include "../../memory/stack.hpp"
#include "../../string/strings.hpp"
#include "../../vector/fvector.hpp"
#include "../../vector/vector.hpp"
#include "../../vector/svector.hpp"

#include "callbacks.hpp"
#include "structs.hpp"

// fork clone daemon execve mmap functionality
// you might be wondering why not roll your own posix calls for process spawning
// the reason is, it's simply too tedious to get it working on a wide range of machines, and there is a LOT of
// boilterplate code, which would make the final result more or less identical to the system version

extern char **environ;
namespace micron
{


struct uelf_t {
  int fd;
  byte* elf;
};

template <is_string S>
uelf_t
create_elf_memory(uelf_t& elf, const S& str, const S& str)
{
  uelf_t elf { memfd_create(str.c_str(), 0),  };
}

template <is_string... S>
posix::process_list_t
create_processes(S... names)
{
  posix::process_list_t p;
  (p.emplace_back(names, names), ...);
  return p;
}

template <typename... S>
posix::process_list_t
create_processes(S... names)
{
  posix::process_list_t p;
  (p.emplace_back(names, names), ...);
  return p;
}

void
run_processes(posix::process_list_t &n)
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
    // if ( t.wait )
    //   ::waitpid(t.pid, &t.status, 0);
  }
}

// implementation of daemon
template <int Stack = default_stack_size, typename F, typename... Args>
  requires micron::is_function_v<F> or micron::is_function_v<micron::remove_pointer_t<F>>
int
daemon(F f, Args &&...args)
{
  int pid = ::fork();     // much nicer to do this
  if ( pid > 0 )          // parent
    _Exit(0);
  if ( setsid() < 0 )
    throw except::runtime_error("micron process daemon failed to create new session");
  // don't change dir
  ::umask(0);
  posix::close(STDIN_FILENO);
  posix::close(STDOUT_FILENO);
  posix::close(STDERR_FILENO);

  micron::open("/dev/null", o_rdwr);
  posix::dup(0);
  posix::dup(0);
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
  int pid
      = ::clone(__default_callback, fstack + Stack, CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD, NULL);
  if ( pid == -1 )
    throw except::system_error("micron process failed to fork()");
  if ( pid == 0 )
    return pid;
  int status = 0;
  micron::waitpid(pid, &status, 0);
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
};
