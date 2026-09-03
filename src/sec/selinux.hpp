//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../errno.hpp"
#include "../types.hpp"

#include "../linux/io/sys.hpp"
#include "../linux/sys/fcntl.hpp"
#include "../linux/sys/xattr.hpp"

#include "bits.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^
// SELinux
//
// we parse everything through
//   /sys/fs/selinux/*
//   /proc/<pid>/attr/*
//   security.selinux
namespace micron
{
namespace sec
{
namespace selinux
{

inline constexpr const char *fs_root = "/sys/fs/selinux";

enum class mode : u8 {
  disabled = 0,
  permissive = 1,
  enforcing = 2,
};

struct status_t {
  bool present = false;
  mode state = mode::disabled;
  i32 policyvers = -1;
  bool mls = false;
  bool deny_unknown = false;

  [[nodiscard]] constexpr bool
  enforcing(void) const noexcept
  {
    return state == mode::enforcing;
  }
};

namespace __impl
{

inline max_t
__read_small(const char *path, char *buf, usize cap) noexcept
{
  const i32 fd = static_cast<i32>(posix::open(path, posix::o_rdonly | posix::o_cloexec, 0));
  if ( fd < 0 ) [[unlikely]]
    return fd;
  max_t n = posix::read(fd, buf, cap - 1);
  (void)posix::close(fd);
  if ( n < 0 ) [[unlikely]]
    return n;
  buf[n] = '\0';
  return n;
}

inline i32
__write_one(const char *path, const char *data, usize len) noexcept
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

inline i32
__parse_int(const char *s) noexcept
{
  i32 v = 0;
  bool any = false;
  while ( *s == ' ' || *s == '\t' ) ++s;
  while ( *s >= '0' && *s <= '9' ) {
    v = v * 10 + (*s - '0');
    ++s;
    any = true;
  }
  return any ? v : -1;
}

inline bool
__join(char *out, usize cap, const char *dir, const char *leaf) noexcept
{
  usize i = 0;
  for ( const char *p = dir; *p; ++p ) {
    if ( i + 2 >= cap ) return false;
    out[i++] = *p;
  }
  if ( i + 2 >= cap ) return false;
  out[i++] = '/';
  for ( const char *p = leaf; *p; ++p ) {
    if ( i + 1 >= cap ) return false;
    out[i++] = *p;
  }
  out[i] = '\0';
  return true;
}

inline usize
__len(const char *s) noexcept
{
  usize n = 0;
  while ( s[n] ) ++n;
  return n;
}

// every selinuxfs path goes through fs_root, so moving selinuxfs is one edit and not six
inline bool
__fs_join(char *out, usize cap, const char *leaf) noexcept
{
  return __join(out, cap, fs_root, leaf);
}

inline max_t
__read_fs(const char *leaf, char *buf, usize cap) noexcept
{
  char path[384];
  if ( !__fs_join(path, sizeof(path), leaf) ) [[unlikely]]
    return -error::name_too_long;
  return __read_small(path, buf, cap);
}

};      // namespace __impl

// policy state
[[nodiscard]] inline status_t
status(void) noexcept
{
  status_t st{};
  char buf[32];

  const max_t e = __impl::__read_fs("enforce", buf, sizeof(buf));
  if ( e < 0 ) return st;      // selinuxfs is not mounted -> SELinux is not in play

  st.present = true;
  st.state = (__impl::__parse_int(buf) == 1) ? mode::enforcing : mode::permissive;

  if ( __impl::__read_fs("policyvers", buf, sizeof(buf)) >= 0 ) st.policyvers = __impl::__parse_int(buf);
  if ( __impl::__read_fs("mls", buf, sizeof(buf)) >= 0 ) st.mls = __impl::__parse_int(buf) == 1;
  if ( __impl::__read_fs("deny_unknown", buf, sizeof(buf)) >= 0 ) st.deny_unknown = __impl::__parse_int(buf) == 1;
  return st;
}

[[nodiscard]] inline bool
present(void) noexcept
{
  return status().present;
}

[[nodiscard]] inline bool
enforcing(void) noexcept
{
  return status().enforcing();
}

namespace __impl
{
inline result<bool>
__read_fs_flag(const char *dir, const char *name) noexcept
{
  char sub[320];
  if ( !__join(sub, sizeof(sub), dir, name) ) [[unlikely]]
    return result<bool>{ error_t(-error::name_too_long) };

  char buf[32];
  const max_t n = __read_fs(sub, buf, sizeof(buf));
  if ( n < 0 ) [[unlikely]]
    return result<bool>{ error_t(static_cast<i32>(n)) };
  return result<bool>{ __parse_int(buf) == 1 };
}
};      // namespace __impl

// the file reads "<active> <pending>"; the first is the value in force
[[nodiscard]] inline result<bool>
boolean(const char *name) noexcept
{
  return __impl::__read_fs_flag("booleans", name);
}

[[nodiscard]] inline result<bool>
policy_capability(const char *name) noexcept
{
  return __impl::__read_fs_flag("policy_capabilities", name);
}

// security context
class context
{
public:
  static constexpr usize max_len = 512;

  struct part {
    const char *data = nullptr;
    usize size = 0;

    [[nodiscard]] constexpr bool
    empty(void) const noexcept
    {
      return size == 0;
    }
  };

private:
  char __raw[max_len]{};
  usize __len = 0;
  u16 __sep[3]{};
  u8 __seps = 0;
  bool __trunc = false;

public:
  constexpr context(void) noexcept = default;

  // an over-long label does not become a SHORTER label: it becomes no label, flagged, because a
  // silently clipped MLS range compares equal to things it is not
  [[nodiscard]] static context
  parse(const char *s, usize n) noexcept
  {
    context c{};
    if ( s == nullptr ) return c;
    while ( n > 0 && s[n - 1] == '\0' ) --n;
    while ( n > 0 && s[n - 1] == '\n' ) --n;
    if ( n >= max_len ) {
      c.__trunc = true;
      return c;
    }
    if ( n == 0 ) return c;

    for ( usize i = 0; i < n; ++i ) {
      c.__raw[i] = s[i];
      if ( s[i] == ':' && c.__seps < 3 ) c.__sep[c.__seps++] = static_cast<u16>(i);
    }
    c.__raw[n] = '\0';
    c.__len = n;
    return c;
  }

  [[nodiscard]] static context
  parse(const char *s) noexcept
  {
    return parse(s, s ? __impl::__len(s) : 0);
  }

  [[nodiscard]] constexpr bool
  valid(void) const noexcept
  {
    return __len > 0 && __seps >= 2;
  }

  // the input did not fit in max_len. Never valid(), and never equal to anything
  [[nodiscard]] constexpr bool
  truncated(void) const noexcept
  {
    return __trunc;
  }

  [[nodiscard]] constexpr explicit
  operator bool(void) const noexcept
  {
    return valid();
  }

  [[nodiscard]] constexpr const char *
  str(void) const noexcept
  {
    return __raw;
  }

  [[nodiscard]] constexpr usize
  size(void) const noexcept
  {
    return __len;
  }

  [[nodiscard]] constexpr bool
  has_range(void) const noexcept
  {
    return __seps == 3;
  }

  [[nodiscard]] constexpr part
  user(void) const noexcept
  {
    return __seps >= 1 ? part{ __raw, __sep[0] } : part{};
  }

  [[nodiscard]] constexpr part
  role(void) const noexcept
  {
    return __seps >= 2 ? part{ __raw + __sep[0] + 1, static_cast<usize>(__sep[1] - __sep[0] - 1) } : part{};
  }

  [[nodiscard]] constexpr part
  type(void) const noexcept
  {
    if ( __seps < 2 ) return part{};
    const usize start = __sep[1] + 1;
    const usize end = (__seps == 3) ? __sep[2] : __len;
    return part{ __raw + start, end - start };
  }

  [[nodiscard]] constexpr part
  range(void) const noexcept
  {
    if ( __seps < 3 ) return part{};
    return part{ __raw + __sep[2] + 1, __len - __sep[2] - 1 };
  }

  // WARNING: two contexts that did not parse are NOT equal. This is an access predicate before it is
  // a value comparison, and "both unreadable" must never read as "the same label"
  [[nodiscard]] constexpr bool
  operator==(const context &o) const noexcept
  {
    if ( !valid() || !o.valid() ) return false;
    if ( __len != o.__len ) return false;
    for ( usize i = 0; i < __len; ++i )
      if ( __raw[i] != o.__raw[i] ) return false;
    return true;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%
// process contexts

enum class attr : u8 {
  current,
  exec,
  fscreate,
  keycreate,
  sockcreate,
  prev,
};

[[nodiscard]] constexpr const char *
name_of(attr a) noexcept
{
  switch ( a ) {
  case attr::current:
    return "current";
  case attr::exec:
    return "exec";
  case attr::fscreate:
    return "fscreate";
  case attr::keycreate:
    return "keycreate";
  case attr::sockcreate:
    return "sockcreate";
  case attr::prev:
    return "prev";
  }
  return "current";
}

namespace __impl
{
// a context that did not fit is an error, not a value -- otherwise is_first() reports success on a
// label nobody ever read in full
inline result<context>
__wrap(const context &c) noexcept
{
  if ( c.truncated() ) [[unlikely]]
    return result<context>{ error_t(-error::name_too_long) };
  return result<context>{ c };
}
};      // namespace __impl

[[nodiscard]] inline result<context>
get_attr(attr a) noexcept
{
  char path[64];
  if ( !__impl::__join(path, sizeof(path), "/proc/self/attr", name_of(a)) ) [[unlikely]]
    return result<context>{ error_t(-error::name_too_long) };

  // one byte MORE than a context can hold, so an over-long attr reaches parse() as over-long
  // instead of arriving pre-clipped and looking well-formed
  char buf[context::max_len + 1];
  const max_t n = __impl::__read_small(path, buf, sizeof(buf));
  if ( n < 0 ) [[unlikely]]
    return result<context>{ error_t(static_cast<i32>(n)) };
  return __impl::__wrap(context::parse(buf, static_cast<usize>(n)));
}

[[nodiscard]] inline result<context>
self(void) noexcept
{
  return get_attr(attr::current);
}

[[nodiscard]] inline i32
set_attr(attr a, const context &c) noexcept
{
  if ( !c.valid() ) [[unlikely]]
    return -error::invalid_arg;
  char path[64];
  if ( !__impl::__join(path, sizeof(path), "/proc/self/attr", name_of(a)) ) [[unlikely]]
    return -error::name_too_long;
  return __impl::__write_one(path, c.str(), c.size());
}

[[nodiscard]] inline i32
clear_attr(attr a) noexcept
{
  char path[64];
  if ( !__impl::__join(path, sizeof(path), "/proc/self/attr", name_of(a)) ) [[unlikely]]
    return -error::name_too_long;
  return __impl::__write_one(path, "", 0);
}

[[nodiscard]] inline i32
set_exec(const context &c) noexcept
{
  return set_attr(attr::exec, c);
}

[[nodiscard]] inline i32
set_fscreate(const context &c) noexcept
{
  return set_attr(attr::fscreate, c);
}

// %%%%%%%%%%%%%%%%%%%
// file labels

namespace __impl
{
inline result<context>
__label_from(max_t n, const char *buf) noexcept
{
  if ( n < 0 ) [[unlikely]]
    return result<context>{ error_t(static_cast<i32>(n)) };
  return __wrap(context::parse(buf, static_cast<usize>(n)));
}
};      // namespace __impl

[[nodiscard]] inline result<context>
label_of(const char *path) noexcept
{
  char buf[context::max_len];
  return __impl::__label_from(posix::getxattr(path, posix::xattr_name_selinux, buf, sizeof(buf)), buf);
}

// does not follow a terminal symlink
[[nodiscard]] inline result<context>
label_of_link(const char *path) noexcept
{
  char buf[context::max_len];
  return __impl::__label_from(posix::lgetxattr(path, posix::xattr_name_selinux, buf, sizeof(buf)), buf);
}

[[nodiscard]] inline result<context>
label_of_fd(i32 fd) noexcept
{
  char buf[context::max_len];
  return __impl::__label_from(posix::fgetxattr(fd, posix::xattr_name_selinux, buf, sizeof(buf)), buf);
}

// NOTE: the label is stored with its terminating NUL
[[nodiscard]] inline i32
set_label(const char *path, const context &c) noexcept
{
  if ( !c.valid() ) [[unlikely]]
    return -error::invalid_arg;
  return posix::setxattr(path, posix::xattr_name_selinux, c.str(), c.size() + 1, 0);
}

[[nodiscard]] inline i32
set_label_link(const char *path, const context &c) noexcept
{
  if ( !c.valid() ) [[unlikely]]
    return -error::invalid_arg;
  return posix::lsetxattr(path, posix::xattr_name_selinux, c.str(), c.size() + 1, 0);
}

[[nodiscard]] inline i32
set_label_fd(i32 fd, const context &c) noexcept
{
  if ( !c.valid() ) [[unlikely]]
    return -error::invalid_arg;
  return posix::fsetxattr(fd, posix::xattr_name_selinux, c.str(), c.size() + 1, 0);
}

[[nodiscard]] inline i32
remove_label(const char *path) noexcept
{
  return posix::removexattr(path, posix::xattr_name_selinux);
}

};      // namespace selinux
};      // namespace sec
};      // namespace micron
