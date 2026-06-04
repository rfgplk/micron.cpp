//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../sys/keyctl.hpp"

#include "../../types.hpp"

namespace micron
{

enum class keyring_spec : posix::key_serial_t {
  thread = posix::key_spec_thread_keyring,
  process = posix::key_spec_process_keyring,
  session = posix::key_spec_session_keyring,
  user = posix::key_spec_user_keyring,
  user_session = posix::key_spec_user_session_keyring,
  group = posix::key_spec_group_keyring,
  reqkey_auth = posix::key_spec_reqkey_auth_key,
  requestor = posix::key_spec_requestor_keyring,
};

enum class key_perm : u32 {
  pos_view = posix::key_pos_view,
  pos_read = posix::key_pos_read,
  pos_write = posix::key_pos_write,
  pos_search = posix::key_pos_search,
  pos_link = posix::key_pos_link,
  pos_setattr = posix::key_pos_setattr,
  pos_all = posix::key_pos_all,
  usr_view = posix::key_usr_view,
  usr_read = posix::key_usr_read,
  usr_write = posix::key_usr_write,
  usr_search = posix::key_usr_search,
  usr_link = posix::key_usr_link,
  usr_setattr = posix::key_usr_setattr,
  usr_all = posix::key_usr_all,
  grp_all = posix::key_grp_all,
  oth_view = posix::key_oth_view,
  oth_read = posix::key_oth_read,
  oth_all = posix::key_oth_all,
};

constexpr key_perm
operator|(key_perm a, key_perm b) noexcept
{
  return static_cast<key_perm>(static_cast<u32>(a) | static_cast<u32>(b));
}

class key
{
  posix::key_serial_t __id;

public:
  constexpr key() noexcept : __id(0) { }

  constexpr explicit key(posix::key_serial_t id) noexcept : __id(id) { }

  constexpr key(keyring_spec s) noexcept : __id(static_cast<posix::key_serial_t>(s)) { }

  constexpr posix::key_serial_t
  id() const noexcept
  {
    return __id;
  }

  constexpr bool
  valid() const noexcept
  {
    return __id != 0;
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return valid();
  }

  static key
  add_key(const char *type, const char *description, const void *payload, usize plen, key keyring)
  {
    posix::key_serial_t s = posix::add_key(type, description, payload, plen, keyring.id());
    return micron::syscall_failed(s) ? key{} : key{ s };
  }

  static key
  request(const char *type, const char *description, const char *callout_info, key dest_keyring)
  {
    posix::key_serial_t s = posix::request_key(type, description, callout_info, dest_keyring.id());
    return micron::syscall_failed(s) ? key{} : key{ s };
  }

  i32
  update(const void *payload, usize plen) const noexcept
  {
    return posix::keyctl_update_key(__id, payload, plen);
  }

  i32
  revoke() const noexcept
  {
    return posix::keyctl_revoke_key(__id);
  }

  i32
  invalidate() const noexcept
  {
    return posix::keyctl_invalidate_key(__id);
  }

  i32
  chown(uid_t uid, gid_t gid) const noexcept
  {
    return posix::keyctl_chown_key(__id, uid, gid);
  }

  i32
  set_perm(key_perm p) const noexcept
  {
    return posix::keyctl_setperm_key(__id, static_cast<u32>(p));
  }

  i32
  set_timeout(u32 seconds) const noexcept
  {
    return posix::keyctl_set_timeout_key(__id, seconds);
  }

  // NOTE: read/describe/get_security return the kernel's TRUE size, which may EXCEED buflen
  i32
  read(void *buf, usize buflen) const noexcept
  {
    return posix::keyctl_read_key(__id, buf, buflen);
  }

  i32
  describe(char *buf, usize buflen) const noexcept
  {
    return posix::keyctl_describe_key(__id, buf, buflen);
  }

  i32
  get_security(char *buf, usize buflen) const noexcept
  {
    return posix::keyctl_get_security_of(__id, buf, buflen);
  }

  i32
  link_into(key keyring) const noexcept
  {
    return posix::keyctl_link_key(__id, keyring.id());
  }

  i32
  unlink_from(key keyring) const noexcept
  {
    return posix::keyctl_unlink_key(__id, keyring.id());
  }

  i32
  clear() const noexcept
  {
    return posix::keyctl_clear_keyring(__id);
  }

  i32
  link(key k) const noexcept
  {
    return posix::keyctl_link_key(k.id(), __id);
  }

  i32
  restrict_to(const char *type, const char *restriction) const noexcept
  {
    return posix::keyctl_restrict(__id, type, restriction);
  }

  key
  search(const char *type, const char *description, key dest = key{}) const
  {
    posix::key_serial_t s = posix::keyctl_search_keyring(__id, type, description, dest.id());
    return micron::syscall_failed(s) ? key{} : key{ s };
  }
};

inline key
thread_keyring() noexcept
{
  return key{ keyring_spec::thread };
}

inline key
process_keyring() noexcept
{
  return key{ keyring_spec::process };
}

inline key
session_keyring() noexcept
{
  return key{ keyring_spec::session };
}

inline key
user_keyring() noexcept
{
  return key{ keyring_spec::user };
}

inline key
user_session_keyring() noexcept
{
  return key{ keyring_spec::user_session };
}

inline key
join_session_keyring(const char *name = nullptr)
{
  posix::key_serial_t s = posix::keyctl_join_session(name);
  return micron::syscall_failed(s) ? key{} : key{ s };
}

};      // namespace micron
