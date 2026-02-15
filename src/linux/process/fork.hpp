#pragma once

#include "../../memory/actions.hpp"
#include "../../memory/mman.hpp"
#include "../sys/clone.hpp"

#include "../../memory/stack_constants.hpp"

#include "callbacks.hpp"

namespace micron
{

// Fork implementations
constexpr static const int __fork_flags_std = 0;
constexpr static const int __fork_flags_parent = posix::clone_parent;

// standard fork function, emulating ::fork() behavior, calls specializied fork_clone
int
__base_fork()
{
  int pid = micron::posix::__fork_clone(sig_chld);     // should be sig_chld
  if ( pid == -1 )
    exc<except::system_error>("micron process failed to fork()");
  return pid;
}

// fork and spin off child with dedicated memory stack, run function pointed to
template <auto Fn = __default_callback, int Stack = default_stack_size, typename... Args>
int
__base_fork(Args &&...args)
{
  addr_t *fstack = micron::addrmap(Stack);
  if ( micron::mmap_failed(fstack) )
    exc<except::system_error>("micron process micron::mmap failed to allocate stack");
  int pid = micron::posix::clone<Stack, Fn>(fstack, __fork_flags_std, micron::forward<Args>(args)...);
  if ( pid == -1 )
    exc<except::system_error>("micron process failed to fork()");
  return pid;
}

int
fork()
{
  // invoked base fork
  return __base_fork();
}

// implementation of ::fork() but automatically wait for child proc to end
template <int Stack = default_stack_size>
int
wfork()
{
  int pid = __base_fork();
  if ( pid == 0 )
    return pid;
  if ( pid < 1 ) {
    return pid;
  }
  int status = 0;
  micron::waitpid(pid, &status, 0);
  if ( micron::wifexited(status) )
    return micron::wexitstatus(status);
  return status;
}

template <auto Fn, typename... Args>
int
fork(Args &&...args)
{
  return __base_fork<Fn>(micron::forward<Args>(args)...);
}

template <auto Fn, typename... Args>
int
wfork(Args &&...args)
{
  int pid = __base_fork<Fn>(micron::forward<Args>(args)...);
  if ( pid == 0 )
    return pid;
  if ( pid < 1 ) {
    return pid;
  }
  int status = 0;
  micron::waitpid(pid, &status, 0);
  if ( micron::wifexited(status) )
    return micron::wexitstatus(status);
  return status;
}

};
