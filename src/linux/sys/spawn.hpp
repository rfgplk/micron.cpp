
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "system.hpp"

#include "../io.hpp"

#include "exec.hpp"

#include "signal.hpp"

namespace micron
{
constexpr static const int posix_spawn_resetids = 0x01;
constexpr static const int posix_spawn_setpgroup = 0x02;
constexpr static const int posix_spawn_setsigdef = 0x04;
constexpr static const int posix_spawn_setsigmask = 0x08;
constexpr static const int posix_spawn_setschedparam = 0x10;
constexpr static const int posix_spawn_setscheduler = 0x20;
constexpr static const int posix_spawn_usevfork = 0x40;
constexpr static const int posix_spawn_setsid = 0x80;
constexpr static const int posix_spawn_setcgroup = 0x100;

namespace posix
{

enum spawn_action_type { SPAWN_ACTION_OPEN, SPAWN_ACTION_CLOSE, SPAWN_ACTION_DUP2 };

struct spawn_action {
  int type;
  int fd;
  int newfd;
  const char *path;
  int oflag;
  mode_t mode;
};

struct spawnattr_t {
  short int __flags;
  posix::pid_t pgrp;
  sigset_t sd;
  sigset_t ss;
  struct sched_param __sp;
  int policy;
  int cgroup;
  int pad[15];
};

struct spawn_file_actions_t {
  int allocated;
  int used;
  struct spawn_action *__actions;
  int pad[16];
};

struct spawn_ctx {
  const char *path;
  char *const *argv;
  char *const *envp;
  const spawn_file_actions_t *fa;
  const spawnattr_t *attr;
  int errfd;
};

int
spawnattr_init(spawnattr_t &attr)
{
  attr.__flags = 0;
  attr.pgrp = 0;

  micron::sigemptyset(attr.sd);
  micron::sigemptyset(attr.ss);

  attr.__sp.sched_priority = 0;
  attr.policy = sched_other;

  attr.cgroup = -1;

  for ( int i = 0; i < 15; ++i )
    attr.pad[i] = 0;

  return 0;
}

int
__apply_file_actions(const spawn_file_actions_t &fa)
{
  for ( int i = 0; i < fa.used; ++i ) {
    const spawn_action &a = fa.__actions[i];

    switch ( a.type ) {
    case SPAWN_ACTION_OPEN : {
      auto fd = micron::openat(at_fdcwd, a.path, a.oflag, a.mode);
      if ( fd < 0 )
        return -errno;
      if ( fd != a.fd ) {
        micron::dup3(fd, a.fd, 0);
        micron::close(fd);
      }
      break;
    }
    case SPAWN_ACTION_CLOSE :
      micron::close(a.fd);
      break;

    case SPAWN_ACTION_DUP2 :
      micron::dup3(a.fd, a.newfd, 0);
      break;
    }
  }
  return 0;
}

int
__apply_spawnattr(const spawnattr_t &attr)
{
  // TODO: push as separate file
  if ( attr.__flags & posix_spawn_setpgroup )
    if ( micron::syscall(SYS_setpgid, 0, attr.pgrp) < 0 )
      return -errno;

  if ( attr.__flags & posix_spawn_setsigmask )
    if ( micron::syscall(SYS_rt_sigprocmask, sig_setmask, &attr.ss, nullptr, sizeof(sigset_t)) < 0 )
      return -errno;

  if ( attr.__flags & posix_spawn_setsigdef )
    if ( micron::syscall(SYS_rt_sigprocmask, sig_unblock, &attr.sd, nullptr, sizeof(sigset_t)) < 0 )
      return -errno;

  if ( attr.__flags & posix_spawn_setscheduler )
    if ( micron::syscall(SYS_sched_setscheduler, 0, attr.policy, &attr.__sp) < 0 )
      return -errno;

  if ( attr.__flags & posix_spawn_setschedparam )
    if ( micron::syscall(SYS_sched_setparam, 0, &attr.__sp) < 0 )
      return -errno;

  // if ( attr.cgroup >= 0 )
  //   if ( micron::syscall(SYS_setns, attr.cgroup, CLONE_NEWCGROUP) < 0 )
  //     return -errno;

  return 0;
}
__attribute__((noreturn)) int
spawn_process(spawn_ctx *ctx)
{

  if ( ctx->fa ) {
    int r = __apply_file_actions(*ctx->fa);
    if ( r < 0 )
      goto fail;
  }

  if ( ctx->attr ) {
    int r = __apply_spawnattr(*ctx->attr);
    if ( r < 0 )
      goto fail;
  }
  posix::execve(ctx->path, ctx->argv, ctx->envp);

fail:
  int err = errno;
  micron::write(ctx->errfd, &err, sizeof(err));
  micron::posix::exit(127);
  __builtin_unreachable();
}
__attribute__((noreturn)) int
spawn_process(spawn_ctx &ctx)
{

  if ( ctx.fa ) {
    int r = __apply_file_actions(*ctx.fa);
    if ( r < 0 )
      goto fail;
  }

  if ( ctx.attr ) {
    int r = __apply_spawnattr(*ctx.attr);
    if ( r < 0 )
      goto fail;
  }
  posix::execve(ctx.path, ctx.argv, ctx.envp);

fail:
  int err = errno;
  micron::write(ctx.errfd, &err, sizeof(err));
  micron::posix::exit(127);
  __builtin_unreachable();
}
};
};
