//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../types.hpp"

#include "../linux/io/sys.hpp"
#include "../linux/sys/fcntl.hpp"
#include "../linux/sys/namespaces.hpp"
#include "../linux/sys/stat.hpp"

#include "bits.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// linux namespaces

namespace micron
{
namespace sec
{
namespace ns
{

// kinds
enum class ns_kind : u64 {
  none = 0,
  mount = posix::clone_newns,
  cgroup = posix::clone_newcgroup,
  uts = posix::clone_newuts,
  ipc = posix::clone_newipc,
  user = posix::clone_newuser,
  pid = posix::clone_newpid,
  net = posix::clone_newnet,
  time = posix::clone_newtime,
};

[[nodiscard]] constexpr ns_kind
operator|(ns_kind a, ns_kind b) noexcept
{
  return static_cast<ns_kind>(static_cast<u64>(a) | static_cast<u64>(b));
}

[[nodiscard]] constexpr ns_kind
operator&(ns_kind a, ns_kind b) noexcept
{
  return static_cast<ns_kind>(static_cast<u64>(a) & static_cast<u64>(b));
}

[[nodiscard]] constexpr ns_kind
operator~(ns_kind a) noexcept
{
  return static_cast<ns_kind>(~static_cast<u64>(a));
}

constexpr ns_kind &
operator|=(ns_kind &a, ns_kind b) noexcept
{
  return a = a | b;
}

constexpr ns_kind &
operator&=(ns_kind &a, ns_kind b) noexcept
{
  return a = a & b;
}

[[nodiscard]] constexpr bool
any(ns_kind k) noexcept
{
  return static_cast<u64>(k) != 0;
}

[[nodiscard]] constexpr u64
bits(ns_kind k) noexcept
{
  return static_cast<u64>(k);
}

[[nodiscard]] constexpr const char *
name_of(ns_kind k) noexcept
{
  switch ( k ) {
  case ns_kind::mount:
    return "mnt";
  case ns_kind::cgroup:
    return "cgroup";
  case ns_kind::uts:
    return "uts";
  case ns_kind::ipc:
    return "ipc";
  case ns_kind::user:
    return "user";
  case ns_kind::pid:
    return "pid";
  case ns_kind::net:
    return "net";
  case ns_kind::time:
    return "time";
  default:
    return nullptr;
  }
}

[[nodiscard]] constexpr u64
host_inode_of(ns_kind k) noexcept
{
  switch ( k ) {
  case ns_kind::mount:
    return posix::mnt_ns_init_ino;
  case ns_kind::cgroup:
    return posix::cgroup_ns_init_ino;
  case ns_kind::uts:
    return posix::uts_ns_init_ino;
  case ns_kind::ipc:
    return posix::ipc_ns_init_ino;
  case ns_kind::user:
    return posix::user_ns_init_ino;
  case ns_kind::pid:
    return posix::pid_ns_init_ino;
  case ns_kind::net:
    return posix::net_ns_init_ino;
  case ns_kind::time:
    return posix::time_ns_init_ino;
  default:
    return 0;
  }
}

inline constexpr ns_kind all_kinds[]
    = { ns_kind::user, ns_kind::mount, ns_kind::pid, ns_kind::net, ns_kind::ipc, ns_kind::uts, ns_kind::cgroup, ns_kind::time };

namespace __impl
{

inline void
__ns_path(char *out, usize cap, i32 pid, const char *name) noexcept
{
  usize i = 0;
  auto put = [&](const char *s) {
    while ( *s && i + 1 < cap ) out[i++] = *s++;
  };
  put("/proc/");
  if ( pid < 0 ) {
    put("self");
  } else {
    char digits[12];
    usize n = 0;
    u32 v = static_cast<u32>(pid);
    if ( v == 0 ) digits[n++] = '0';
    while ( v ) {
      digits[n++] = static_cast<char>('0' + (v % 10));
      v /= 10;
    }
    while ( n && i + 1 < cap ) out[i++] = digits[--n];
  }
  put("/ns/");
  put(name);
  out[i < cap ? i : cap - 1] = '\0';
}

};      // namespace __impl

class ns_handle
{
  i32 __fd = -error::bad_file_number;
  ns_kind __kind = ns_kind::none;

  [[nodiscard]] i32
  __check(void) const noexcept
  {
    if ( __fd < 0 ) [[unlikely]]
      return __fd;
    return 0;
  }

  constexpr ns_handle(i32 fd, ns_kind k) noexcept : __fd(fd), __kind(k) { }

public:
  ~ns_handle() { close(); }

  constexpr ns_handle() noexcept = default;

  ns_handle(const ns_handle &) = delete;
  ns_handle &operator=(const ns_handle &) = delete;

  ns_handle(ns_handle &&o) noexcept : __fd(o.__fd), __kind(o.__kind) { o.__fd = -error::bad_file_number; }

  ns_handle &
  operator=(ns_handle &&o) noexcept
  {
    if ( this == &o ) return *this;
    close();
    __fd = o.__fd;
    __kind = o.__kind;
    o.__fd = -error::bad_file_number;
    return *this;
  }

  [[nodiscard]] static ns_handle
  __adopt(i32 fd, ns_kind k) noexcept
  {
    return ns_handle(fd, k);
  }

  void
  close(void) noexcept
  {
    if ( __fd >= 0 ) {
      (void)posix::close(__fd);
      __fd = -error::bad_file_number;
    }
  }

  [[nodiscard]] bool
  valid(void) const noexcept
  {
    return __fd >= 0;
  }

  [[nodiscard]] explicit
  operator bool(void) const noexcept
  {
    return valid();
  }

  [[nodiscard]] i32
  fd(void) const noexcept
  {
    return __fd;
  }

  [[nodiscard]] i32
  error(void) const noexcept
  {
    return __fd >= 0 ? 0 : __check();
  }

  [[nodiscard]] ns_kind
  kind(void) const noexcept
  {
    return __kind;
  }

  [[nodiscard]] i32
  type(void) const noexcept
  {
    if ( i32 e = __check(); e ) [[unlikely]]
      return e;
    return posix::ns_type_of(__fd);
  }

  [[nodiscard]] max_t
  inode(void) const noexcept
  {
    if ( i32 e = __check(); e ) [[unlikely]]
      return e;
    posix::stat_t st{};
    if ( const i32 r = static_cast<i32>(posix::fstat(posix::fd_t{ __fd }, st)); r != 0 ) [[unlikely]]
      return r;
    return static_cast<max_t>(st.st_ino);
  }

  [[nodiscard]] i32
  owner_uid(posix::uid_t &out) const noexcept
  {
    if ( i32 e = __check(); e ) [[unlikely]]
      return e;
    return posix::ns_owner_uid_of(__fd, out);
  }

  [[nodiscard]] ns_handle
  owning_userns(void) const noexcept
  {
    if ( i32 e = __check(); e ) [[unlikely]]
      return ns_handle(e, ns_kind::user);
    return ns_handle(posix::ns_userns_of(__fd), ns_kind::user);
  }

  [[nodiscard]] ns_handle
  parent(void) const noexcept
  {
    if ( i32 e = __check(); e ) [[unlikely]]
      return ns_handle(e, __kind);
    return ns_handle(posix::ns_parent_of(__fd), __kind);
  }

  [[nodiscard]] i32
  enter(void) const noexcept
  {
    if ( i32 e = __check(); e ) [[unlikely]]
      return e;
    return posix::setns(__fd, static_cast<i32>(bits(__kind)));
  }

  [[nodiscard]] bool
  is_host(void) const noexcept
  {
    const max_t ino = inode();
    return ino >= 0 && static_cast<u64>(ino) == host_inode_of(__kind);
  }
};

// opening
[[nodiscard]] inline ns_handle
open_of(i32 pid, ns_kind k) noexcept
{
  const char *nm = name_of(k);
  if ( nm == nullptr ) [[unlikely]]
    return ns_handle::__adopt(-error::invalid_arg, k);

  char path[64];
  __impl::__ns_path(path, sizeof(path), pid, nm);
  const i32 fd = static_cast<i32>(posix::open(path, posix::o_rdonly | posix::o_cloexec, 0));
  return ns_handle::__adopt(fd, k);
}

[[nodiscard]] inline ns_handle
open_self(ns_kind k) noexcept
{
  return open_of(-1, k);
}

// creating
[[nodiscard]] inline i32
unshare(ns_kind kinds) noexcept
{
  if ( !any(kinds) ) return 0;
  return posix::unshare(static_cast<i32>(bits(kinds)));
}

// id maps
namespace __impl
{
inline i32
__write_proc(const char *path, const char *data, usize len) noexcept
{
  const i32 fd = static_cast<i32>(posix::open(path, posix::o_wronly | posix::o_cloexec, 0));
  if ( fd < 0 ) [[unlikely]]
    return fd;
  const max_t w = posix::write(fd, data, len);
  (void)posix::close(fd);
  if ( w < 0 ) [[unlikely]]
    return static_cast<i32>(w);
  return w == static_cast<max_t>(len) ? 0 : -error::invalid_arg;
}

// "<inner> <outer> <count>\n" with no formatter
inline usize
__map_line(char *out, usize cap, u32 inner, u32 outer, u32 count) noexcept
{
  auto put_u32 = [&](usize i, u32 v) -> usize {
    char d[12];
    usize n = 0;
    if ( v == 0 ) d[n++] = '0';
    while ( v ) {
      d[n++] = static_cast<char>('0' + (v % 10));
      v /= 10;
    }
    while ( n && i + 1 < cap ) out[i++] = d[--n];
    return i;
  };
  usize i = put_u32(0, inner);
  if ( i + 1 < cap ) out[i++] = ' ';
  i = put_u32(i, outer);
  if ( i + 1 < cap ) out[i++] = ' ';
  i = put_u32(i, count);
  if ( i + 1 < cap ) out[i++] = '\n';
  out[i] = '\0';
  return i;
}
};      // namespace __impl

[[nodiscard]] inline i32
deny_setgroups(void) noexcept
{
  return __impl::__write_proc("/proc/self/setgroups", "deny", 4);
}

[[nodiscard]] inline i32
write_uid_map(u32 inner, u32 outer, u32 count = 1) noexcept
{
  char line[48];
  const usize n = __impl::__map_line(line, sizeof(line), inner, outer, count);
  return __impl::__write_proc("/proc/self/uid_map", line, n);
}

[[nodiscard]] inline i32
write_gid_map(u32 inner, u32 outer, u32 count = 1) noexcept
{
  char line[48];
  const usize n = __impl::__map_line(line, sizeof(line), inner, outer, count);
  return __impl::__write_proc("/proc/self/gid_map", line, n);
}

[[nodiscard]] inline i32
map_to_root(u32 outer_uid, u32 outer_gid) noexcept
{
  if ( const i32 r = deny_setgroups(); r < 0 ) [[unlikely]]
    return r;
  if ( const i32 r = write_uid_map(0, outer_uid, 1); r < 0 ) [[unlikely]]
    return r;
  return write_gid_map(0, outer_gid, 1);
}

[[nodiscard]] inline i32
unshare_user_as_root(ns_kind extra = ns_kind::none) noexcept
{
  const u32 uid = static_cast<u32>(posix::getuid());
  const u32 gid = static_cast<u32>(posix::getgid());
  if ( const i32 r = unshare(ns_kind::user); r < 0 ) [[unlikely]]
    return r;
  if ( const i32 r = map_to_root(uid, gid); r < 0 ) [[unlikely]]
    return r;
  return any(extra) ? unshare(extra) : 0;
}

// scopes
namespace __impl
{
inline thread_local i32 __restore_slot = 0;
};      // namespace __impl

[[nodiscard]] inline i32
last_restore_error(void) noexcept
{
  return __impl::__restore_slot;
}

inline void
clear_restore_error(void) noexcept
{
  __impl::__restore_slot = 0;
}

class enter_scope
{
  ns_handle __saved;
  i32 __entered = -error::bad_file_number;
  bool __restored = false;

  i32
  __restore(void) noexcept
  {
    if ( __restored || __entered < 0 ) return 0;
    if ( !__saved.valid() ) return __saved.error();
    const i32 r = __saved.enter();
    if ( r >= 0 ) __restored = true;
    return r;
  }

public:
  enter_scope(const enter_scope &) = delete;
  enter_scope &operator=(const enter_scope &) = delete;

  explicit enter_scope(const ns_handle &target) noexcept : __saved(open_self(target.kind()))
  {
    __entered = target.enter();
    if ( __entered < 0 ) __saved.close();
  }

  ~enter_scope()
  {
    const i32 r = __restore();
    if ( r < 0 ) __impl::__restore_slot = r;
  }

  [[nodiscard]] i32
  restore(void) noexcept
  {
    return __restore();
  }

  [[nodiscard]] bool
  ok(void) const noexcept
  {
    return __entered >= 0;
  }

  [[nodiscard]] i32
  error(void) const noexcept
  {
    return __entered;
  }

  [[nodiscard]] bool
  restorable(void) const noexcept
  {
    return __saved.valid();
  }

  [[nodiscard]] bool
  restored(void) const noexcept
  {
    return __restored;
  }
};

};      // namespace ns
};      // namespace sec
};      // namespace micron
