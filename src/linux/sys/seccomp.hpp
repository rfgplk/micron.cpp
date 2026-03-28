//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "prctl.hpp"

#include "bpf.hpp"

namespace micron
{
namespace posix
{
constexpr static const u32 seccomp_set_mode_strict = 0u;
constexpr static const u32 seccomp_set_mode_filter = 1u;
constexpr static const u32 seccomp_get_action_avail = 2u;
constexpr static const u32 seccomp_get_notif_sizes = 3u;

constexpr static const u32 seccomp_filter_flag_tsync = (1u << 0);
constexpr static const u32 seccomp_filter_flag_log = (1u << 1);
constexpr static const u32 seccomp_filter_flag_spec_allow = (1u << 2);
constexpr static const u32 seccomp_filter_flag_new_listener = (1u << 3);
constexpr static const u32 seccomp_filter_flag_tsync_esrch = (1u << 4);
constexpr static const u32 seccomp_filter_flag_wait_killable_recv = (1u << 5);

constexpr static const u32 seccomp_ret_kill_process = 0x80000000u;
constexpr static const u32 seccomp_ret_kill_thread = 0x00000000u;
constexpr static const u32 seccomp_ret_kill = seccomp_ret_kill_thread;
constexpr static const u32 seccomp_ret_trap = 0x00030000u;
constexpr static const u32 seccomp_ret_errno = 0x00050000u;
constexpr static const u32 seccomp_ret_user_notif = 0x7fc00000u;
constexpr static const u32 seccomp_ret_trace = 0x7ff00000u;
constexpr static const u32 seccomp_ret_log = 0x7ffc0000u;
constexpr static const u32 seccomp_ret_allow = 0x7fff0000u;
constexpr static const u32 seccomp_ret_action_full = 0xffff0000u;
constexpr static const u32 seccomp_ret_data = 0x0000ffffu;

struct seccomp_data_t {
  i32 nr;     // syscall number
  u32 arch;
  u64 instruction_pointer;     // RIP at syscall entry
  u64 args[6];                 // syscall arguments
};

constexpr static const u32 seccomp_data_nr_off = 0u;
constexpr static const u32 seccomp_data_arch_off = 4u;
constexpr static const u32 seccomp_data_ip_lo_off = 8u;
constexpr static const u32 seccomp_data_ip_hi_off = 12u;

[[nodiscard]] constexpr u32
seccomp_data_arg_lo_off(u32 n) noexcept
{
  return 16u + n * 8u;
}

[[nodiscard]] constexpr u32
seccomp_data_arg_hi_off(u32 n) noexcept
{
  return 16u + n * 8u + 4u;
}

// NOTE: needs kernel >5.0
struct __attribute__((packed)) seccomp_notif_t {
  u64 id;
  u32 pid;
  u32 flags;
  seccomp_data_t data;
};

struct seccomp_notif_resp_t {
  u64 id;
  i64 val;
  i32 error;
  u32 flags;
};

struct seccomp_notif_sizes_t {
  u16 seccomp_notif;
  u16 seccomp_notif_resp;
  u16 seccomp_data;
};

constexpr static const u32 seccomp_user_notif_flag_continue = (1u << 0);

inline long
seccomp(u32 op, u32 flags, const void *args)
{
  return micron::syscall(SYS_seccomp, op, flags, args);
}

inline int
seccomp_strict(void)
{
  return static_cast<int>(seccomp(seccomp_set_mode_strict, 0, nullptr));
}

inline int
seccomp_load_filter(const bpf::fprog_t &prog, u32 flags = 0)
{
  return static_cast<int>(seccomp(seccomp_set_mode_filter, flags, &prog));
}

inline int
seccomp_action_avail(u32 action)
{
  return static_cast<int>(seccomp(seccomp_get_action_avail, 0, &action));
}

inline int
seccomp_get_notif_sizes(seccomp_notif_sizes_t &out)
{
  return static_cast<int>(seccomp(seccomp_get_notif_sizes, 0, &out));
}

inline int
seccomp_load_filter_notif(const bpf::fprog_t &prog, u32 extra_flags = 0)
{
  return static_cast<int>(seccomp(seccomp_set_mode_filter, seccomp_filter_flag_new_listener | extra_flags, &prog));
}

// NOTE: needs kernel >5.0

constexpr static const u32 seccomp_ioctl_notif_recv = 0xc0502100u;
constexpr static const u32 seccomp_ioctl_notif_send = 0xc0182101u;
constexpr static const u32 seccomp_ioctl_notif_id_valid = 0x40082102u;
constexpr static const u32 seccomp_ioctl_notif_addfd = 0xc0182103u;

inline int
seccomp_notify_receive(int fd, seccomp_notif_t &req)
{
  return static_cast<int>(micron::ioctl(fd, seccomp_ioctl_notif_recv, &req));
}

inline int
seccomp_notify_respond(int fd, seccomp_notif_resp_t &resp)
{
  return static_cast<int>(micron::ioctl(fd, seccomp_ioctl_notif_send, &resp));
}

inline int
seccomp_notify_id_valid(int fd, u64 id)
{
  return static_cast<int>(micron::ioctl(fd, seccomp_ioctl_notif_id_valid, &id));
}

};     // namespace posix
};     // namespace micron
