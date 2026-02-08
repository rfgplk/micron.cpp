//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../sys/capabilities.hpp"

#include "../../array/arrays.hpp"
#include "../../linux/process/system.hpp"
#include "../../types.hpp"
#include "../../vector/svector.hpp"

namespace micron
{

enum class capabilities : i32 {
  chown = cap_chown,
  dac_override = cap_dac_override,
  dac_read_search = cap_dac_read_search,
  fowner = cap_fowner,
  fsetid = cap_fsetid,
  kill = cap_kill,
  setgid = cap_setgid,
  setuid = cap_setuid,
  setpcap = cap_setpcap,
  linux_immutable = cap_linux_immutable,
  net_bind_service = cap_net_bind_service,
  net_broadcast = cap_net_broadcast,
  net_admin = cap_net_admin,
  net_raw = cap_net_raw,
  ipc_lock = cap_ipc_lock,
  ipc_owner = cap_ipc_owner,
  sys_module = cap_sys_module,
  sys_rawio = cap_sys_rawio,
  sys_chroot = cap_sys_chroot,
  sys_ptrace = cap_sys_ptrace,
  sys_pacct = cap_sys_pacct,
  sys_admin = cap_sys_admin,
  sys_boot = cap_sys_boot,
  sys_nice = cap_sys_nice,
  sys_resource = cap_sys_resource,
  sys_time = cap_sys_time,
  sys_tty_config = cap_sys_tty_config,
  mknod = cap_mknod,
  lease = cap_lease,
  audit_write = cap_audit_write,
  audit_control = cap_audit_control,
  setfcapm = cap_setfcap,
  mac_override = cap_mac_override,
  mac_admin = cap_mac_admin,
  syslog = cap_syslog,
  wake_alarm = cap_wake_alarm,
  block_suspend = cap_block_suspend,
  audit_read = cap_audit_read,
  perfmon = cap_perfmon,
  bpf = cap_bpf,
  checkpoint_restore = cap_checkpoint_restore,
  __end
};

enum cap_flags { effective = 0, permitted = 1, inheritable = 2, __end = 3 };

enum cap_modes : u32 { clear = 0, set = 1 };

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
