//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
// more legible flags

namespace posix
{

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

// holds ind. pid of process running
struct upid_t {
  uid_t uid;
  gid_t gid;
  pid_t pid;
};

struct uprocess_t : public upid_t {
  micron::string path;
  micron::svector<micron::string> argv;
  posix_spawnattr_t flags;
  int status;
  // bool wait;
  template <typename... Args>
  uprocess_t(const char *str, Args... args)
      : upid_t{ 0, 0, 0 }, path(str), argv({ args... }), status(0)     //, wait(true)
  {
    if ( posix_spawnattr_init(&flags) != 0 ) {
      throw except::system_error("micron process failed to init spawnattrs");
    }
  }

  template <typename... Args>
  uprocess_t(micron::string &&o, Args... args)
      : upid_t{ 0, 0, 0 }, path(micron::move(o)), argv({ args... }), status(0)     //, wait(true)
  {
    if ( posix_spawnattr_init(&flags) != 0 ) {
      throw except::system_error("micron process failed to init spawnattrs");
    }
  }
};

typedef micron::fvector<uprocess_t> process_list_t;
};
};
