#pragma once

#include "../../memory/actions.hpp"
#include "../../memory/mman.hpp"
#include "../sys/clone.hpp"

#include "../../memory/stack_constants.hpp"

#include "../../thread/callbacks.hpp"
#include "wait.hpp"

namespace micron
{

// Fork implementations
constexpr static const int __fork_flags_std = 0;
constexpr static const int __fork_flags_parent = posix::clone_parent;

// standard fork function, emulating ::fork() behavior, calls specializied fork_clone
int
__base_fork()
{
  int pid = micron::posix::__fork_clone(posix::sig_chld);
  if ( pid < 0 ) exc<except::system_error>("micron process failed to fork()");
  return pid;
}

// fork and run a function in the child
// NOTE: flags=0 fork is copy-on-write: the kernel does NOT switch the stack pointer,
// so the child resumes in *this* frame on a COW copy of the parent stack
template<auto Fn = __default_callback, int Stack = default_stack_size, typename... Args>
int
__base_fork(Args &&...args)
{
  int pid = micron::posix::__fork_clone(posix::sig_chld);
  if ( pid < 0 ) exc<except::system_error>("micron process failed to fork()");
  if ( pid == 0 ) {
    // child: COW address space, same stack pointer -> forwarded args are live here
    using ret_t = decltype(Fn(micron::forward<Args>(args)...));
    if constexpr ( micron::is_same_v<ret_t, int> ) {
      int rc = Fn(micron::forward<Args>(args)...);
      micron::syscall(SYS_exit_group, rc);
    } else {
      Fn(micron::forward<Args>(args)...);
      micron::syscall(SYS_exit_group, 0);
    }
    __builtin_unreachable();
  }
  return pid;      // parent
}

int
fork()
{
  // invoked base fork
  return __base_fork();
}

// implementation of ::fork() but automatically wait for child proc to end
template<int Stack = default_stack_size>
int
wfork()
{
  int pid = __base_fork();
  if ( pid == 0 ) return pid;
  if ( pid < 1 ) {
    return pid;
  }
  int status = 0;
  micron::waitpid(pid, &status, 0);
  if ( micron::wifexited(status) ) return micron::wexitstatus(status);
  return status;
}

template<auto Fn, typename... Args>
int
fork(Args &&...args)
{
  return __base_fork<Fn>(micron::forward<Args>(args)...);
}

template<auto Fn, typename... Args>
int
wfork(Args &&...args)
{
  int pid = __base_fork<Fn>(micron::forward<Args>(args)...);
  if ( pid == 0 ) return pid;
  if ( pid < 1 ) {
    return pid;
  }
  int status = 0;
  micron::waitpid(pid, &status, 0);
  if ( micron::wifexited(status) ) return micron::wexitstatus(status);
  return status;
}

};      // namespace micron
