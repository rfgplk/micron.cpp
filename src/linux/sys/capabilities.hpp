//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"
#include "prctl.hpp"

namespace micron
{
namespace posix
{

constexpr u32 linux_capability_version_3 = 0x20080522u;

struct cap_hdr_t {
  u32 version;
  i32 pid;
};

struct cap_data_t {
  u32 effective;
  u32 permitted;
  u32 inheritable;
};

constexpr u32 cap_chown = 0;
constexpr u32 cap_dac_override = 1;
constexpr u32 cap_dac_read_search = 2;
constexpr u32 cap_fowner = 3;
constexpr u32 cap_fsetid = 4;
constexpr u32 cap_kill = 5;
constexpr u32 cap_setgid = 6;
constexpr u32 cap_setuid = 7;
constexpr u32 cap_setpcap = 8;
constexpr u32 cap_linux_immutable = 9;
constexpr u32 cap_net_bind_service = 10;
constexpr u32 cap_net_broadcast = 11;
constexpr u32 cap_net_admin = 12;
constexpr u32 cap_net_raw = 13;
constexpr u32 cap_ipc_lock = 14;
constexpr u32 cap_ipc_owner = 15;
constexpr u32 cap_sys_module = 16;
constexpr u32 cap_sys_rawio = 17;
constexpr u32 cap_sys_chroot = 18;
constexpr u32 cap_sys_ptrace = 19;
constexpr u32 cap_sys_pacct = 20;
constexpr u32 cap_sys_admin = 21;
constexpr u32 cap_sys_boot = 22;
constexpr u32 cap_sys_nice = 23;
constexpr u32 cap_sys_resource = 24;
constexpr u32 cap_sys_time = 25;
constexpr u32 cap_sys_tty_config = 26;
constexpr u32 cap_mknod = 27;
constexpr u32 cap_lease = 28;
constexpr u32 cap_audit_write = 29;
constexpr u32 cap_audit_control = 30;
constexpr u32 cap_setfcap = 31;
constexpr u32 cap_mac_override = 32;
constexpr u32 cap_mac_admin = 33;
constexpr u32 cap_syslog = 34;
constexpr u32 cap_wake_alarm = 35;
constexpr u32 cap_block_suspend = 36;
constexpr u32 cap_audit_read = 37;
constexpr u32 cap_perfmon = 38;
constexpr u32 cap_bpf = 39;
constexpr u32 cap_checkpoint_restore = 40;
constexpr u32 cap_last_cap = cap_checkpoint_restore;

inline i32
capget(cap_hdr_t &hdr, cap_data_t data[2])
{
  return static_cast<i32>(micron::syscall(SYS_capget, &hdr, data));
}

inline i32
capset(cap_hdr_t &hdr, const cap_data_t data[2])
{
  return static_cast<i32>(micron::syscall(SYS_capset, &hdr, data));
}

inline i32
capget_pid(pid_t pid, cap_data_t data[2])
{
  cap_hdr_t hdr{ linux_capability_version_3, pid };
  return capget(hdr, data);
}

inline i32
capset_self(const cap_data_t data[2])
{
  cap_hdr_t hdr{ linux_capability_version_3, 0 };
  return capset(hdr, data);
}

inline i32
cap_bset_read(u32 c)
{
  return micron::prctl(PR_CAPBSET_READ, static_cast<unsigned long>(c));
}

inline i32
cap_bset_drop(u32 c)
{
  return micron::prctl(PR_CAPBSET_DROP, static_cast<unsigned long>(c));
}

inline i32
cap_ambient_is_set(u32 c)
{
  return micron::prctl(PR_CAP_AMBIENT, static_cast<unsigned long>(PR_CAP_AMBIENT_IS_SET), static_cast<unsigned long>(c));
}

inline i32
cap_ambient_raise(u32 c)
{
  return micron::prctl(PR_CAP_AMBIENT, static_cast<unsigned long>(PR_CAP_AMBIENT_RAISE), static_cast<unsigned long>(c));
}

inline i32
cap_ambient_lower(u32 c)
{
  return micron::prctl(PR_CAP_AMBIENT, static_cast<unsigned long>(PR_CAP_AMBIENT_LOWER), static_cast<unsigned long>(c));
}

inline i32
cap_ambient_clear_all()
{
  return micron::prctl(PR_CAP_AMBIENT, static_cast<unsigned long>(PR_CAP_AMBIENT_CLEAR_ALL));
}

inline i32
cap_set_keepcaps(i32 keep)
{
  return micron::prctl(PR_SET_KEEPCAPS, static_cast<unsigned long>(keep));
}

inline i32
cap_get_keepcaps()
{
  return micron::prctl(PR_GET_KEEPCAPS);
}

inline i32
cap_no_new_privs()
{
  return micron::prctl(PR_SET_NO_NEW_PRIVS, 1UL);
}

inline i32
cap_get_no_new_privs()
{
  return micron::prctl(PR_GET_NO_NEW_PRIVS);
}

// securebits
inline i32
cap_get_securebits()
{
  return micron::prctl(PR_GET_SECUREBITS);
}

inline i32
cap_set_securebits(unsigned long bits)
{
  return micron::prctl(PR_SET_SECUREBITS, bits);
}

inline u64
cap_bset_read_all()
{
  u64 mask = 0;
  for ( u32 i = 0; i <= cap_last_cap; ++i ) {
    if ( cap_bset_read(i) > 0 )
      mask |= u64(1) << i;
  }
  return mask;
}

inline u64
cap_ambient_read_all()
{
  u64 mask = 0;
  for ( u32 i = 0; i <= cap_last_cap; ++i ) {
    if ( cap_ambient_is_set(i) > 0 )
      mask |= u64(1) << i;
  }
  return mask;
}

inline void
caps_to_data(u64 eff, u64 prm, u64 inh, cap_data_t data[2])
{
  data[0].effective = static_cast<u32>(eff & 0xFFFFFFFFu);
  data[0].permitted = static_cast<u32>(prm & 0xFFFFFFFFu);
  data[0].inheritable = static_cast<u32>(inh & 0xFFFFFFFFu);
  data[1].effective = static_cast<u32>((eff >> 32) & 0xFFFFFFFFu);
  data[1].permitted = static_cast<u32>((prm >> 32) & 0xFFFFFFFFu);
  data[1].inheritable = static_cast<u32>((inh >> 32) & 0xFFFFFFFFu);
}

inline void
data_to_caps(const cap_data_t data[2], u64 &eff, u64 &prm, u64 &inh)
{
  eff = u64(data[0].effective) | (u64(data[1].effective) << 32);
  prm = u64(data[0].permitted) | (u64(data[1].permitted) << 32);
  inh = u64(data[0].inheritable) | (u64(data[1].inheritable) << 32);
}

};     // namespace posix
};     // namespace micron
