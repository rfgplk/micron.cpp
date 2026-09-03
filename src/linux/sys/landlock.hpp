//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// linux landlock

namespace micron
{
namespace posix
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// filesystem access rights

constexpr static const u64 landlock_access_fs_execute = (1uLL << 0);
constexpr static const u64 landlock_access_fs_write_file = (1uLL << 1);
constexpr static const u64 landlock_access_fs_read_file = (1uLL << 2);
constexpr static const u64 landlock_access_fs_read_dir = (1uLL << 3);
constexpr static const u64 landlock_access_fs_remove_dir = (1uLL << 4);
constexpr static const u64 landlock_access_fs_remove_file = (1uLL << 5);
constexpr static const u64 landlock_access_fs_make_char = (1uLL << 6);
constexpr static const u64 landlock_access_fs_make_dir = (1uLL << 7);
constexpr static const u64 landlock_access_fs_make_reg = (1uLL << 8);
constexpr static const u64 landlock_access_fs_make_sock = (1uLL << 9);
constexpr static const u64 landlock_access_fs_make_fifo = (1uLL << 10);
constexpr static const u64 landlock_access_fs_make_block = (1uLL << 11);
constexpr static const u64 landlock_access_fs_make_sym = (1uLL << 12);
constexpr static const u64 landlock_access_fs_refer = (1uLL << 13);             // abi 2
constexpr static const u64 landlock_access_fs_truncate = (1uLL << 14);          // abi 3
constexpr static const u64 landlock_access_fs_ioctl_dev = (1uLL << 15);         // abi 5
constexpr static const u64 landlock_access_fs_resolve_unix = (1uLL << 16);      // abi 9

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// network access rights (abi 4)

constexpr static const u64 landlock_access_net_bind_tcp = (1uLL << 0);
constexpr static const u64 landlock_access_net_connect_tcp = (1uLL << 1);

// %%%%%%%%%%%%%%%%%%%%%%%
// ipc scoping (abi 6)

constexpr static const u64 landlock_scope_abstract_unix_socket = (1uLL << 0);
constexpr static const u64 landlock_scope_signal = (1uLL << 1);

// fs masks
constexpr static const u64 landlock_access_fs_abi1 = 0x1fffuLL;
constexpr static const u64 landlock_access_fs_abi2 = 0x3fffuLL;
constexpr static const u64 landlock_access_fs_abi3 = 0x7fffuLL;
constexpr static const u64 landlock_access_fs_abi5 = 0xffffuLL;
constexpr static const u64 landlock_access_fs_abi9 = 0x1ffffuLL;

[[nodiscard]] constexpr u64
landlock_fs_mask_for(i32 abi) noexcept
{
  if ( abi <= 1 ) return landlock_access_fs_abi1;
  if ( abi == 2 ) return landlock_access_fs_abi2;
  if ( abi == 3 or abi == 4 ) return landlock_access_fs_abi3;
  if ( abi >= 5 and abi <= 8 ) return landlock_access_fs_abi5;
  return landlock_access_fs_abi9;
}

[[nodiscard]] constexpr u64
landlock_net_mask_for(i32 abi) noexcept
{
  return abi >= 4 ? (landlock_access_net_bind_tcp | landlock_access_net_connect_tcp) : 0uLL;
}

[[nodiscard]] constexpr u64
landlock_scope_mask_for(i32 abi) noexcept
{
  return abi >= 6 ? (landlock_scope_abstract_unix_socket | landlock_scope_signal) : 0uLL;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%
// rule types + call flags

constexpr static const u32 landlock_rule_path_beneath = 1u;
constexpr static const u32 landlock_rule_net_port = 2u;

constexpr static const u32 landlock_create_ruleset_version = (1u << 0);
constexpr static const u32 landlock_create_ruleset_errata = (1u << 1);      // abi 5

constexpr static const u32 landlock_restrict_self_log_same_exec_off = (1u << 0);       // abi 7
constexpr static const u32 landlock_restrict_self_log_new_exec_on = (1u << 1);         // abi 7
constexpr static const u32 landlock_restrict_self_log_subdomains_off = (1u << 2);      // abi 7
constexpr static const u32 landlock_restrict_self_tsync = (1u << 3);                   // abi 8

// %%%%%%%%%%%%%%%%%%%%%
// records

struct landlock_ruleset_attr_t {
  u64 handled_access_fs;
  u64 handled_access_net;      // abi 4
  u64 scoped;                  // abi 6
};

constexpr static const usize landlock_ruleset_size_ver0 = 8;       // abi 1-3
constexpr static const usize landlock_ruleset_size_ver1 = 16;      // abi 4-5
constexpr static const usize landlock_ruleset_size_ver2 = 24;      // abi 6+

[[nodiscard]] constexpr usize
landlock_ruleset_size_for(i32 abi) noexcept
{
  if ( abi <= 3 ) return landlock_ruleset_size_ver0;
  if ( abi <= 5 ) return landlock_ruleset_size_ver1;
  return landlock_ruleset_size_ver2;
}

// must be packed
struct __attribute__((packed)) landlock_path_beneath_attr_t {
  u64 allowed_access;
  i32 parent_fd;      // preferably opened O_PATH
};

struct landlock_net_port_attr_t {
  u64 allowed_access;
  u64 port;      // host endianness, NOT network order
};

static_assert(sizeof(landlock_ruleset_attr_t) == 24, "landlock_ruleset_attr ABI");
static_assert(sizeof(landlock_path_beneath_attr_t) == 12, "landlock_path_beneath_attr ABI (must be packed)");
static_assert(sizeof(landlock_net_port_attr_t) == 16, "landlock_net_port_attr ABI");
static_assert(__builtin_offsetof(landlock_ruleset_attr_t, scoped) == 16, "landlock_ruleset_attr.scoped ABI");
static_assert(__builtin_offsetof(landlock_path_beneath_attr_t, parent_fd) == 8, "landlock_path_beneath_attr.parent_fd ABI");

// %%%%%%%%%%%%%%
// syscalls
inline i32
landlock_create_ruleset(const landlock_ruleset_attr_t *attr, usize size, u32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_landlock_create_ruleset, attr, size, flags));
}

inline i32
landlock_add_rule(i32 ruleset_fd, u32 rule_type, const void *rule_attr, u32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_landlock_add_rule, ruleset_fd, rule_type, rule_attr, flags));
}

inline i32
landlock_restrict_self(i32 ruleset_fd, u32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_landlock_restrict_self, ruleset_fd, flags));
}

inline i32
landlock_abi_version(void)
{
  return landlock_create_ruleset(nullptr, 0, landlock_create_ruleset_version);
}

inline i32
landlock_errata(void)
{
  return landlock_create_ruleset(nullptr, 0, landlock_create_ruleset_errata);
}

inline i32
landlock_add_path_beneath(i32 ruleset_fd, i32 parent_fd, u64 allowed)
{
  landlock_path_beneath_attr_t a{ allowed, parent_fd };
  return landlock_add_rule(ruleset_fd, landlock_rule_path_beneath, &a, 0);
}

inline i32
landlock_add_net_port(i32 ruleset_fd, u64 port, u64 allowed)
{
  landlock_net_port_attr_t a{ allowed, port };
  return landlock_add_rule(ruleset_fd, landlock_rule_net_port, &a, 0);
}

};      // namespace posix
};      // namespace micron
