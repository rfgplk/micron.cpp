//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <linux/capability.h>
#include <sys/capability.h>

#include "../../array/arrays.hpp"
#include "../../vector/svector.hpp"
#include "../../linux/process/system.hpp"
#include "../../types.hpp"

namespace micron
{
enum class capabilities : u64 {
  audit_control,
  audit_read,
  audit_write,
  block_suspend,
  bpf,
  checkpoint_restore,
  chown,
  dac_override,
  dac_read_search,
  fowner,
  fsetid,
  ipc_lock,
  ipc_owner,
  kill,
  lease,
  linux_immutable,
  mac_admin,
  mac_override,
  mknod,
  net_admin,
  net_bind_service,
  net_broadcast,
  net_raw,
  perfmon,
  setgid,
  setfcapm,
  setpcap,
  stuid,
  sys_admin,
  sys_boot,
  sys_chroot,
  sys_module,
  sys_nice,
  sys_pacct,
  sys_ptrace,
  sys_rawio,
  sys_resource,
  sys_time,
  sys_tty_config,
  syslog,
  wake_alarm,
  __end
};

enum cap_flags { effective = 0, permitted = 1, inheritable = 2, __end = 3 };

enum cap_modes : u32 { clear = 0, set = 1 };

struct ucap_header_t {
  u32 version;
  int pid;
};

struct ucap_single_t {
  capabilities cap;
  cap_modes effective;     // yay or nay
  cap_modes permitted;
  cap_modes inheritable;
};     // cap_t equiv.

using ucap_t = micron::array<ucap_single_t, static_cast<u64>(capabilities::__end)>;

ucap_t
load_caps(pid_t p = 0)
{
  // cap_t cap = cap_get_proc();
  cap_t cap = cap_get_pid(p);
  if ( !cap ) {
  }     // err
  ucap_t ucaps;
  ucap_single_t ucap;
  cap_flag_value_t flag;
  cap_value_t val;

  u64 r = static_cast<u64>(capabilities::__end);
  for ( u64 i = 0; i < r; i++ ) {
    ucap.cap = static_cast<capabilities>(i);
    val = i;
    if ( cap_get_flag(caps, val, cap_flags::effective, &flag) == -1 ) {
      break;
      // cap_free(caps);
    }
    ucap.effective = flag;

    if ( cap_get_flag(caps, val, cap_flags::permitted, &flag) == -1 ) {
      break;
      // cap_free(caps);
    }
    ucap.permitted = flag;

    if ( cap_get_flag(caps, val, cap_flags::inheritable, &flag) == -1 ) {
      break;
      // cap_free(caps);
    }
    ucap.inheritable = flag;
    ucaps[i] = ucap;
  }
  cap_free(caps);
  return ucaps;
}

template <typename... T>
concept all_pid = (micron::same_as<T, pid_t> && ...);

template <all_pid... Args>
micron::svector<cap_t>
load_caps(Args... args)
{
  micron::svector<ucap_t> caps;
  (caps.push_back(load_caps(args)), ...);
  return caps;
}

void
clear_caps(ucap_t &cap)
{
  cap.effective = cap_modes::clear;
  cap.permitted = cap_modes::clear;
  cap.inheritable = cap_modes::clear;
}

void
apply_caps(cap_t _caps, ucap_t &cap)
{
  // cap_t _caps = cap_get_proc();
  if ( !_caps )
    return;
  if ( cap_set_flag(_caps, cap_flags::effective, 1, static_cast<cap_value_t>(cap.cap), cap.effective)
       or cap_set_flag(_caps, cap_flags::permitted, 1, static_cast<cap_value_t>(cap.cap), cap.permitted)
       or cap_set_flag(_caps, cap_flags::inheritable, 1, static_cast<cap_value_t>(cap.cap), cap.inheritable) ) {
    cap_free(_caps);
    return;
  }
  if ( cap_set_proc(_caps) == -1 ) {
    cap_free(_caps);
    return;
  }
}

void
apply_caps(cap_t _caps, micron::svector<ucap_t> &caps)
{
  for ( auto &n : caps ) {
    apply_caps(_caps, n);
  }
}
};     // namespace micron
