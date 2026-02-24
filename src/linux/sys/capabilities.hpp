//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../types.hpp"
#include "types.hpp"

namespace micron
{
namespace posix
{

struct ucap_header {
  u32 version;
  i32 pid;
};

using ucap_header_t = ucap_header_t *;

struct ucap_data_t {
  u32 effective;
  u32 permitted;
  u32 inheritable;
};

using cap_user_data_t = user_cap_data *;

constexpr static const i32 vfs_cap_revision_mask = 0xFF000000;
constexpr static const i32 vfs_cap_revision_shift = 24;
constexpr static const i32 vfs_cap_flags_mask = ~vfs_cap_revision_mask;
constexpr static const i32 vfs_cap_flags_effective = 0x000001;

constexpr static const i32 vfs_cap_revision_1 = 0x01000000;
constexpr static const i32 vfs_cap_u32_1 = 1;
constexpr static const i32 xattr_caps_sz_1 = sizeof(u32) * (1 + 2 * vfs_cap_u32_1);

constexpr static const i32 vfs_cap_revision_2 = 0x02000000;
constexpr static const i32 vfs_cap_u32_2 = 2;
constexpr static const i32 xattr_caps_sz_2 = sizeof(u32) * (1 + 2 * vfs_cap_u32_2);

constexpr static const i32 vfs_cap_revision_3 = 0x03000000;
constexpr static const i32 vfs_cap_u32_3 = 2;
constexpr static const i32 xattr_caps_sz_3 = sizeof(u32) * (2 + 2 * vfs_cap_u32_3);

constexpr static const i32 xattr_caps_sz = xattr_caps_sz_3;
constexpr static const i32 vfs_cap_u32 = vfs_cap_u32_3;
constexpr static const i32 vfs_cap_revision = vfs_cap_revision_3;

struct vfs_cap_data {
  u32 magic_etc;

  struct {
    u32 permitted;
    u32 inheritable;
  } data[vfs_cap_u32];
};

struct vfs_ns_cap_data {
  u32 magic_etc;

  struct {
    u32 permitted;
    u32 inheritable;
  } data[vfs_cap_u32];

  u32 rootid;
};

constexpr static const i32 _linux_capability_version = 0x19980330;
constexpr static const i32 _linux_capability_u32s = 1;

constexpr static const i32 cap_chown = 0;
constexpr static const i32 cap_dac_override = 1;
constexpr static const i32 cap_dac_read_search = 2;
constexpr static const i32 cap_fowner = 3;
constexpr static const i32 cap_fsetid = 4;
constexpr static const i32 cap_kill = 5;
constexpr static const i32 cap_setgid = 6;
constexpr static const i32 cap_setuid = 7;
constexpr static const i32 cap_setpcap = 8;
constexpr static const i32 cap_linux_immutable = 9;
constexpr static const i32 cap_net_bind_service = 10;
constexpr static const i32 cap_net_broadcast = 11;
constexpr static const i32 cap_net_admin = 12;
constexpr static const i32 cap_net_raw = 13;
constexpr static const i32 cap_ipc_lock = 14;
constexpr static const i32 cap_ipc_owner = 15;
constexpr static const i32 cap_sys_module = 16;
constexpr static const i32 cap_sys_rawio = 17;
constexpr static const i32 cap_sys_chroot = 18;
constexpr static const i32 cap_sys_ptrace = 19;
constexpr static const i32 cap_sys_pacct = 20;
constexpr static const i32 cap_sys_admin = 21;
constexpr static const i32 cap_sys_boot = 22;
constexpr static const i32 cap_sys_nice = 23;
constexpr static const i32 cap_sys_resource = 24;
constexpr static const i32 cap_sys_time = 25;
constexpr static const i32 cap_sys_tty_config = 26;
constexpr static const i32 cap_mknod = 27;
constexpr static const i32 cap_lease = 28;
constexpr static const i32 cap_audit_write = 29;
constexpr static const i32 cap_audit_control = 30;
constexpr static const i32 cap_setfcap = 31;
constexpr static const i32 cap_mac_override = 32;
constexpr static const i32 cap_mac_admin = 33;
constexpr static const i32 cap_syslog = 34;
constexpr static const i32 cap_wake_alarm = 35;
constexpr static const i32 cap_block_suspend = 36;
constexpr static const i32 cap_audit_read = 37;
constexpr static const i32 cap_perfmon = 38;
constexpr static const i32 cap_bpf = 39;
constexpr static const i32 cap_checkpoi32_restore = 40;

constexpr static const i32 cap_last_cap = cap_checkpoi32_restore;

constexpr static inline bool
cap_valid(i32 x) noexcept
{
  return x >= 0 && x <= cap_last_cap;
}

constexpr static inline i32
cap_to_index(i32 x) noexcept
{
  return x >> 5;
}

constexpr static inline u32
cap_to_mask(i32 x) noexcept
{
  return 1U << (x & 31);
}

};     // namespace posix

};     // namespace micron
