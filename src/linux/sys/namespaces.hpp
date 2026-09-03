//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"

#include "ioctl.hpp"
#include "mount.hpp"
#include "sched.hpp"
#include "system.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// linux namespaces
//
// NOTE: setns() is in system.hpp and unshare()/pivot_root() are in mount.hpp

namespace micron
{
namespace posix
{

// nsfs records
struct mnt_ns_info_t {
  u32 size;
  u32 nr_mounts;
  u64 mnt_ns_id;
};

constexpr static const u32 mnt_ns_info_size_ver0 = 16;

static_assert(sizeof(mnt_ns_info_t) == 16, "mnt_ns_info ABI");

// nsfs ioctls (NSIO == 0xb7)
constexpr static const u64 __nsio = 0xb7;

constexpr static const u64 ns_get_userns = io_default_command(__nsio, 0x1);
constexpr static const u64 ns_get_parent = io_default_command(__nsio, 0x2);
constexpr static const u64 ns_get_nstype = io_default_command(__nsio, 0x3);      // returns the clone_new* value
constexpr static const u64 ns_get_owner_uid = io_default_command(__nsio, 0x4);
constexpr static const u64 ns_get_mntns_id = io_read_command<u64>(__nsio, 5);
constexpr static const u64 ns_get_pid_from_pidns = io_read_command<i32>(__nsio, 0x6);
constexpr static const u64 ns_get_tgid_from_pidns = io_read_command<i32>(__nsio, 0x7);
constexpr static const u64 ns_get_pid_in_pidns = io_read_command<i32>(__nsio, 0x8);
constexpr static const u64 ns_get_tgid_in_pidns = io_read_command<i32>(__nsio, 0x9);
constexpr static const u64 ns_mnt_get_info = io_read_command<mnt_ns_info_t>(__nsio, 10);
constexpr static const u64 ns_mnt_get_next = io_read_command<mnt_ns_info_t>(__nsio, 11);
constexpr static const u64 ns_mnt_get_prev = io_read_command<mnt_ns_info_t>(__nsio, 12);
constexpr static const u64 ns_get_id = io_read_command<u64>(__nsio, 13);

constexpr static const u64 ipc_ns_init_ino = 0xEFFFFFFFuLL;
constexpr static const u64 uts_ns_init_ino = 0xEFFFFFFEuLL;
constexpr static const u64 user_ns_init_ino = 0xEFFFFFFDuLL;
constexpr static const u64 pid_ns_init_ino = 0xEFFFFFFCuLL;
constexpr static const u64 cgroup_ns_init_ino = 0xEFFFFFFBuLL;
constexpr static const u64 time_ns_init_ino = 0xEFFFFFFAuLL;
constexpr static const u64 net_ns_init_ino = 0xEFFFFFF9uLL;
constexpr static const u64 mnt_ns_init_ino = 0xEFFFFFF8uLL;

inline i32
ns_type_of(i32 fd)
{
  return static_cast<i32>(micron::posix::ioctl(fd, ns_get_nstype));
}

inline i32
ns_parent_of(i32 fd)
{
  return static_cast<i32>(micron::posix::ioctl(fd, ns_get_parent));
}

inline i32
ns_userns_of(i32 fd)
{
  return static_cast<i32>(micron::posix::ioctl(fd, ns_get_userns));
}

inline i32
ns_owner_uid_of(i32 fd, uid_t &out)
{
  return static_cast<i32>(micron::posix::ioctl(fd, ns_get_owner_uid, &out));
}

inline i32
ns_id_of(i32 fd, u64 &out)
{
  return static_cast<i32>(micron::posix::ioctl(fd, ns_get_id, &out));
}

inline i32
ns_mntns_id_of(i32 fd, u64 &out)
{
  return static_cast<i32>(micron::posix::ioctl(fd, ns_get_mntns_id, &out));
}

};      // namespace posix
};      // namespace micron
