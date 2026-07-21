//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"
#include "types.hpp"

namespace micron
{
namespace posix
{

// kernel key-management API
using key_serial_t = i32;

// special keyring ids
constexpr key_serial_t key_spec_thread_keyring = -1;
constexpr key_serial_t key_spec_process_keyring = -2;
constexpr key_serial_t key_spec_session_keyring = -3;
constexpr key_serial_t key_spec_user_keyring = -4;
constexpr key_serial_t key_spec_user_session_keyring = -5;
constexpr key_serial_t key_spec_group_keyring = -6;
constexpr key_serial_t key_spec_reqkey_auth_key = -7;
constexpr key_serial_t key_spec_requestor_keyring = -8;

// default-keyring selectors
constexpr i32 key_reqkey_defl_no_change = -1;
constexpr i32 key_reqkey_defl_default = 0;
constexpr i32 key_reqkey_defl_thread_keyring = 1;
constexpr i32 key_reqkey_defl_process_keyring = 2;
constexpr i32 key_reqkey_defl_session_keyring = 3;
constexpr i32 key_reqkey_defl_user_keyring = 4;
constexpr i32 key_reqkey_defl_user_session_keyring = 5;
constexpr i32 key_reqkey_defl_group_keyring = 6;
constexpr i32 key_reqkey_defl_requestor_keyring = 7;

constexpr i32 keyctl_get_keyring_id = 0;
constexpr i32 keyctl_join_session_keyring = 1;
constexpr i32 keyctl_update = 2;
constexpr i32 keyctl_revoke = 3;
constexpr i32 keyctl_chown = 4;
constexpr i32 keyctl_setperm = 5;
constexpr i32 keyctl_describe = 6;
constexpr i32 keyctl_clear = 7;
constexpr i32 keyctl_link = 8;
constexpr i32 keyctl_unlink = 9;
constexpr i32 keyctl_search = 10;
constexpr i32 keyctl_read = 11;
constexpr i32 keyctl_instantiate = 12;
constexpr i32 keyctl_negate = 13;
constexpr i32 keyctl_set_reqkey_keyring = 14;
constexpr i32 keyctl_set_timeout = 15;
constexpr i32 keyctl_assume_authority = 16;
constexpr i32 keyctl_get_security = 17;
constexpr i32 keyctl_session_to_parent = 18;
constexpr i32 keyctl_reject = 19;
constexpr i32 keyctl_instantiate_iov = 20;
constexpr i32 keyctl_invalidate = 21;
constexpr i32 keyctl_get_persistent = 22;
constexpr i32 keyctl_dh_compute = 23;
constexpr i32 keyctl_pkey_query = 24;
constexpr i32 keyctl_pkey_encrypt = 25;
constexpr i32 keyctl_pkey_decrypt = 26;
constexpr i32 keyctl_pkey_sign = 27;
constexpr i32 keyctl_pkey_verify = 28;
constexpr i32 keyctl_restrict_keyring = 29;
constexpr i32 keyctl_move = 30;
constexpr i32 keyctl_capabilities = 31;
constexpr i32 keyctl_watch_key = 32;

// key permission bits
constexpr u32 key_pos_view = 0x01000000;
constexpr u32 key_pos_read = 0x02000000;
constexpr u32 key_pos_write = 0x04000000;
constexpr u32 key_pos_search = 0x08000000;
constexpr u32 key_pos_link = 0x10000000;
constexpr u32 key_pos_setattr = 0x20000000;
constexpr u32 key_pos_all = 0x3f000000;
constexpr u32 key_usr_view = 0x00010000;
constexpr u32 key_usr_read = 0x00020000;
constexpr u32 key_usr_write = 0x00040000;
constexpr u32 key_usr_search = 0x00080000;
constexpr u32 key_usr_link = 0x00100000;
constexpr u32 key_usr_setattr = 0x00200000;
constexpr u32 key_usr_all = 0x003f0000;
constexpr u32 key_grp_view = 0x00000100;
constexpr u32 key_grp_read = 0x00000200;
constexpr u32 key_grp_write = 0x00000400;
constexpr u32 key_grp_search = 0x00000800;
constexpr u32 key_grp_link = 0x00001000;
constexpr u32 key_grp_setattr = 0x00002000;
constexpr u32 key_grp_all = 0x00003f00;
constexpr u32 key_oth_view = 0x00000001;
constexpr u32 key_oth_read = 0x00000002;
constexpr u32 key_oth_write = 0x00000004;
constexpr u32 key_oth_search = 0x00000008;
constexpr u32 key_oth_link = 0x00000010;
constexpr u32 key_oth_setattr = 0x00000020;
constexpr u32 key_oth_all = 0x0000003f;

constexpr u32 keyctl_move_excl = 0x00000001;

inline auto
add_key(const char *type, const char *description, const void *payload, usize plen, key_serial_t keyring) -> key_serial_t
{
  return static_cast<key_serial_t>(micron::syscall(SYS_add_key, type, description, payload, plen, keyring));
}

inline auto
request_key(const char *type, const char *description, const char *callout_info, key_serial_t dest_keyring) -> key_serial_t
{
  return static_cast<key_serial_t>(micron::syscall(SYS_request_key, type, description, callout_info, dest_keyring));
}

inline auto
keyctl(i32 cmd, unsigned long a2 = 0, unsigned long a3 = 0, unsigned long a4 = 0, unsigned long a5 = 0) -> long
{
  return micron::syscall(SYS_keyctl, cmd, a2, a3, a4, a5);
}

inline auto
keyctl_get_keyring_id_for(key_serial_t id, i32 create) -> key_serial_t
{
  return static_cast<key_serial_t>(keyctl(keyctl_get_keyring_id, static_cast<unsigned long>(id), static_cast<unsigned long>(create)));
}

inline auto
keyctl_join_session(const char *name) -> key_serial_t
{
  return static_cast<key_serial_t>(keyctl(keyctl_join_session_keyring, reinterpret_cast<unsigned long>(name)));
}

inline auto
keyctl_update_key(key_serial_t key, const void *payload, usize plen) -> i32
{
  return static_cast<i32>(keyctl(keyctl_update, static_cast<unsigned long>(key), reinterpret_cast<unsigned long>(payload), plen));
}

inline auto
keyctl_revoke_key(key_serial_t key) -> i32
{
  return static_cast<i32>(keyctl(keyctl_revoke, static_cast<unsigned long>(key)));
}

inline auto
keyctl_chown_key(key_serial_t key, uid_t uid, gid_t gid) -> i32
{
  return static_cast<i32>(
      keyctl(keyctl_chown, static_cast<unsigned long>(key), static_cast<unsigned long>(uid), static_cast<unsigned long>(gid)));
}

inline auto
keyctl_setperm_key(key_serial_t key, u32 perm) -> i32
{
  return static_cast<i32>(keyctl(keyctl_setperm, static_cast<unsigned long>(key), perm));
}

inline auto
keyctl_describe_key(key_serial_t key, char *buf, usize buflen) -> i32
{
  return static_cast<i32>(keyctl(keyctl_describe, static_cast<unsigned long>(key), reinterpret_cast<unsigned long>(buf), buflen));
}

inline auto
keyctl_clear_keyring(key_serial_t keyring) -> i32
{
  return static_cast<i32>(keyctl(keyctl_clear, static_cast<unsigned long>(keyring)));
}

inline auto
keyctl_link_key(key_serial_t key, key_serial_t keyring) -> i32
{
  return static_cast<i32>(keyctl(keyctl_link, static_cast<unsigned long>(key), static_cast<unsigned long>(keyring)));
}

inline auto
keyctl_unlink_key(key_serial_t key, key_serial_t keyring) -> i32
{
  return static_cast<i32>(keyctl(keyctl_unlink, static_cast<unsigned long>(key), static_cast<unsigned long>(keyring)));
}

inline auto
keyctl_search_keyring(key_serial_t keyring, const char *type, const char *description, key_serial_t dest) -> key_serial_t
{
  return static_cast<key_serial_t>(keyctl(keyctl_search, static_cast<unsigned long>(keyring), reinterpret_cast<unsigned long>(type),
                                          reinterpret_cast<unsigned long>(description), static_cast<unsigned long>(dest)));
}

inline auto
keyctl_read_key(key_serial_t key, void *buf, usize buflen) -> i32
{
  return static_cast<i32>(keyctl(keyctl_read, static_cast<unsigned long>(key), reinterpret_cast<unsigned long>(buf), buflen));
}

inline auto
keyctl_set_timeout_key(key_serial_t key, u32 seconds) -> i32
{
  return static_cast<i32>(keyctl(keyctl_set_timeout, static_cast<unsigned long>(key), seconds));
}

inline auto
keyctl_invalidate_key(key_serial_t key) -> i32
{
  return static_cast<i32>(keyctl(keyctl_invalidate, static_cast<unsigned long>(key)));
}

inline auto
keyctl_set_reqkey_keyring_to(i32 reqkey_defl) -> i32
{
  return static_cast<i32>(keyctl(keyctl_set_reqkey_keyring, static_cast<unsigned long>(reqkey_defl)));
}

inline auto
keyctl_get_security_of(key_serial_t key, char *buf, usize buflen) -> i32
{
  return static_cast<i32>(keyctl(keyctl_get_security, static_cast<unsigned long>(key), reinterpret_cast<unsigned long>(buf), buflen));
}

inline auto
keyctl_restrict(key_serial_t keyring, const char *type, const char *restriction) -> i32
{
  return static_cast<i32>(keyctl(keyctl_restrict_keyring, static_cast<unsigned long>(keyring), reinterpret_cast<unsigned long>(type),
                                 reinterpret_cast<unsigned long>(restriction)));
}

// >=5.10
inline auto
keyctl_move_key(key_serial_t key, key_serial_t from, key_serial_t to, u32 flags) -> i32
{
  return static_cast<i32>(
      keyctl(keyctl_move, static_cast<unsigned long>(key), static_cast<unsigned long>(from), static_cast<unsigned long>(to), flags));
}

};      // namespace posix
};      // namespace micron
