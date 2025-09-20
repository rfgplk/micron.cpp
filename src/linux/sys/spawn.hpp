#pragma once

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

};
