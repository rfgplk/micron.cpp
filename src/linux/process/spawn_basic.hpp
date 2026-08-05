//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "../io/sys.hpp"
#include "../sys/spawn.hpp"
#include "fork.hpp"
#include "wait.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pain spawn
//
// pipe2(O_CLOEXEC) -> fork -> execve

namespace micron
{

// NOTE: the errno convention is literal, not -errno, do NOT flip it
int
__spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp)
{
  int pipefd[2];
  int pr = micron::pipe2(pipefd, posix::o_cloexec);
  if ( pr < 0 ) return micron::syscall_errno(pr);      // positive errno
  micron::posix::spawn_ctx ctx = { path, argv, envp, nullptr, nullptr, pipefd[1] };
  // NOTE: try_fork, fork exc<>s out
  pid = micron::try_fork();
  if ( pid < 0 ) {
    micron::close(pipefd[0]);
    micron::close(pipefd[1]);
    return micron::syscall_errno(pid);
  }
  if ( pid == 0 ) {
    micron::close(pipefd[0]);
    micron::posix::spawn_process(ctx);
  }

  micron::close(pipefd[1]);

  int err;
  max_t n = micron::read(pipefd[0], &err, sizeof(err));
  micron::close(pipefd[0]);

  // the write only happens when execve failed
  if ( n == sizeof(err) ) {
    micron::wait4(pid, nullptr, 0, nullptr);
    return err;
  }

  return 0;
}

int
spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp)
{
  return __spawn(pid, path, argv, envp);
}

};      // namespace micron
