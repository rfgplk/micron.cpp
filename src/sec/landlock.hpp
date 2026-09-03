//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/intrin.hpp"
#include "../except.hpp"
#include "../kernel.hpp"
#include "../types.hpp"

#include "../kernel.hpp"
#include "../linux/io/sys.hpp"
#include "../linux/sys/fcntl.hpp"
#include "../linux/sys/landlock.hpp"
#include "../linux/sys/prctl.hpp"

#include "bits.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// landlock

namespace micron
{
namespace sec
{
namespace landlock
{

// access rights
enum class access_fs : u64 {
  none = 0,
  execute = posix::landlock_access_fs_execute,
  write_file = posix::landlock_access_fs_write_file,
  read_file = posix::landlock_access_fs_read_file,
  read_dir = posix::landlock_access_fs_read_dir,
  remove_dir = posix::landlock_access_fs_remove_dir,
  remove_file = posix::landlock_access_fs_remove_file,
  make_char = posix::landlock_access_fs_make_char,
  make_dir = posix::landlock_access_fs_make_dir,
  make_reg = posix::landlock_access_fs_make_reg,
  make_sock = posix::landlock_access_fs_make_sock,
  make_fifo = posix::landlock_access_fs_make_fifo,
  make_block = posix::landlock_access_fs_make_block,
  make_sym = posix::landlock_access_fs_make_sym,
  refer = posix::landlock_access_fs_refer,
  truncate = posix::landlock_access_fs_truncate,
  ioctl_dev = posix::landlock_access_fs_ioctl_dev,
  resolve_unix = posix::landlock_access_fs_resolve_unix,
};

enum class access_net : u64 {
  none = 0,
  bind_tcp = posix::landlock_access_net_bind_tcp,
  connect_tcp = posix::landlock_access_net_connect_tcp,
};

enum class scope : u64 {
  none = 0,
  abstract_unix_socket = posix::landlock_scope_abstract_unix_socket,
  signal = posix::landlock_scope_signal,
};

#define __MICRON_SEC_BITOPS(T)                                                                                                             \
  [[nodiscard]] constexpr T operator|(T a, T b) noexcept { return static_cast<T>(static_cast<u64>(a) | static_cast<u64>(b)); }             \
  [[nodiscard]] constexpr T operator&(T a, T b) noexcept { return static_cast<T>(static_cast<u64>(a) & static_cast<u64>(b)); }             \
  [[nodiscard]] constexpr T operator~(T a) noexcept { return static_cast<T>(~static_cast<u64>(a)); }                                       \
  constexpr T &operator|=(T &a, T b) noexcept { return a = a | b; }                                                                        \
  constexpr T &operator&=(T &a, T b) noexcept { return a = a & b; }                                                                        \
  [[nodiscard]] constexpr bool any(T a) noexcept { return static_cast<u64>(a) != 0; }                                                      \
  [[nodiscard]] constexpr u64 bits(T a) noexcept { return static_cast<u64>(a); }

__MICRON_SEC_BITOPS(access_fs)
__MICRON_SEC_BITOPS(access_net)
__MICRON_SEC_BITOPS(scope)

#undef __MICRON_SEC_BITOPS

// composites, for the common shapes
inline constexpr access_fs read_only = access_fs::read_file | access_fs::read_dir;
inline constexpr access_fs read_execute = access_fs::read_file | access_fs::read_dir | access_fs::execute;
inline constexpr access_fs read_write = access_fs::read_file | access_fs::read_dir | access_fs::write_file | access_fs::truncate;
inline constexpr access_fs make_any = access_fs::make_char | access_fs::make_dir | access_fs::make_reg | access_fs::make_sock
                                      | access_fs::make_fifo | access_fs::make_block | access_fs::make_sym;
inline constexpr access_fs full_dir = read_write | make_any | access_fs::remove_dir | access_fs::remove_file | access_fs::refer;

// NOTE: cached
namespace __impl
{
inline i32 __abi_slot = 0;      // 0 = not probed; > 0 = the abi; < 0 = -errno

inline i32
__probe_abi(void) noexcept
{
  const i32 v = posix::landlock_abi_version();
  const i32 s = v != 0 ? v : -error::bad_syscall;
  atom::store(&__abi_slot, s, micron::atomic_relaxed);
  return s;
}
};      // namespace __impl

[[nodiscard]] inline i32
abi_level(void) noexcept
{
  const i32 v = atom::load(&__impl::__abi_slot, micron::atomic_relaxed);
  if ( v != 0 ) [[likely]]
    return v;
  return __impl::__probe_abi();
}

[[nodiscard]] inline bool
available(void) noexcept
{
  return abi_level() > 0;
}

[[nodiscard]] inline access_fs
supported_fs(void) noexcept
{
  const i32 a = abi_level();
  return a > 0 ? static_cast<access_fs>(posix::landlock_fs_mask_for(a)) : access_fs::none;
}

[[nodiscard]] inline access_net
supported_net(void) noexcept
{
  const i32 a = abi_level();
  return a > 0 ? static_cast<access_net>(posix::landlock_net_mask_for(a)) : access_net::none;
}

[[nodiscard]] inline scope
supported_scope(void) noexcept
{
  const i32 a = abi_level();
  return a > 0 ? static_cast<scope>(posix::landlock_scope_mask_for(a)) : scope::none;
}

class ruleset
{
  i32 __fd = -error::bad_file_number;
  i32 __abi = 0;
  access_fs __handled = access_fs::none;
  access_net __handled_net = access_net::none;
  scope __scoped = scope::none;

  void
  __alive(void) const
  {
    if ( __fd < 0 ) [[unlikely]]
      exc<except::system_error>("micron::sec::landlock::ruleset, no open ruleset fd");
  }

  [[nodiscard]] i32
  __check(void) const noexcept
  {
    if ( __fd < 0 ) [[unlikely]]
      return __fd;
    return 0;
  }

  constexpr ruleset(i32 fd, i32 ab, access_fs h, access_net hn, scope sc) noexcept
      : __fd(fd), __abi(ab), __handled(h), __handled_net(hn), __scoped(sc)
  {
  }

  void
  __create(access_fs handled, access_net handled_net, scope scoped)
  {
    __abi = abi_level();
    if ( __abi <= 0 ) [[unlikely]]
      exc<except::system_error>("micron::sec::landlock::ruleset, landlock unavailable on this kernel");

    __handled = handled & static_cast<access_fs>(posix::landlock_fs_mask_for(__abi));
    __handled_net = handled_net & static_cast<access_net>(posix::landlock_net_mask_for(__abi));
    __scoped = scoped & static_cast<scope>(posix::landlock_scope_mask_for(__abi));

    if ( !any(__handled) && !any(__handled_net) && !any(__scoped) ) [[unlikely]]
      exc<except::invalid_argument>("micron::sec::landlock::ruleset, nothing left to handle after ABI narrowing");

    posix::landlock_ruleset_attr_t attr{ bits(__handled), bits(__handled_net), bits(__scoped) };
    __fd = posix::landlock_create_ruleset(&attr, posix::landlock_ruleset_size_for(__abi), 0);
    if ( __fd < 0 ) [[unlikely]]
      exc<except::system_error>("micron::sec::landlock::ruleset, landlock_create_ruleset failed");
  }

public:
  ~ruleset() { close(); }

  explicit ruleset(access_fs handled, access_net handled_net = access_net::none, scope scoped = scope::none)
  {
    __create(handled, handled_net, scoped);
  }

  ruleset(const ruleset &) = delete;
  ruleset &operator=(const ruleset &) = delete;

  ruleset(ruleset &&o) noexcept : __fd(o.__fd), __abi(o.__abi), __handled(o.__handled), __handled_net(o.__handled_net), __scoped(o.__scoped)
  {
    o.__fd = -error::bad_file_number;
  }

  ruleset &
  operator=(ruleset &&o) noexcept
  {
    if ( this == &o ) return *this;
    close();
    __fd = o.__fd;
    __abi = o.__abi;
    __handled = o.__handled;
    __handled_net = o.__handled_net;
    __scoped = o.__scoped;
    o.__fd = -error::bad_file_number;
    return *this;
  }

  void
  close(void) noexcept
  {
    if ( __fd >= 0 ) {
      (void)posix::close(__fd);
      __fd = -error::bad_file_number;
    }
  }

  [[nodiscard]] static constexpr ruleset
  __failed(i32 neg_errno) noexcept
  {
    return ruleset(neg_errno < 0 ? neg_errno : -error::invalid_arg, 0, access_fs::none, access_net::none, scope::none);
  }

  [[nodiscard]] static constexpr ruleset
  __adopt(i32 fd, i32 ab, access_fs h, access_net hn, scope sc) noexcept
  {
    return ruleset(fd, ab, h, hn, sc);
  }

  [[nodiscard]] bool
  valid(void) const noexcept
  {
    return __fd >= 0;
  }

  [[nodiscard]] i32
  error(void) const noexcept
  {
    return __fd >= 0 ? 0 : __check();
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
  abi(void) const noexcept
  {
    return __abi;
  }

  [[nodiscard]] access_fs
  handled(void) const noexcept
  {
    return __handled;
  }

  [[nodiscard]] access_net
  handled_net(void) const noexcept
  {
    return __handled_net;
  }

  [[nodiscard]] scope
  scoped(void) const noexcept
  {
    return __scoped;
  }

  [[nodiscard]] i32
  allow_fd(i32 dir_fd, access_fs allowed) noexcept
  {
    if ( i32 e = __check(); e ) [[unlikely]]
      return e;
    if ( dir_fd < 0 ) [[unlikely]]
      return -error::bad_file_number;

    const access_fs granted = allowed & __handled;
    if ( !any(granted) ) [[unlikely]]
      return -error::invalid_arg;
    return posix::landlock_add_path_beneath(__fd, dir_fd, bits(granted));
  }

  [[nodiscard]] i32
  allow(const char *path, access_fs allowed) noexcept
  {
    return __add_path(path, allowed, false);
  }

  [[nodiscard]] i32
  allow_following(const char *path, access_fs allowed) noexcept
  {
    return __add_path(path, allowed, true);
  }

private:
  [[nodiscard]] i32
  __open_rule_dir(const char *path, bool follow) noexcept
  {
    constexpr i32 base = posix::o_path | posix::o_cloexec | posix::o_directory;
    if ( follow ) return static_cast<i32>(posix::open(path, base, 0));

    if ( micron::kernel::since(micron::kernel::feature::openat2) ) {
      posix::open_how how{};
      how.flags = static_cast<u64>(base);
      how.mode = 0;
      how.resolve = posix::resolve_no_symlinks | posix::resolve_no_magiclinks;
      const i32 fd = posix::openat2(posix::at_fdcwd, path, how);
      if ( fd >= 0 ) return fd;
      if ( fd != -error::bad_syscall ) return fd;
    }
    return static_cast<i32>(posix::open(path, base | posix::o_nofollow, 0));
  }

  [[nodiscard]] i32
  __add_path(const char *path, access_fs allowed, bool follow) noexcept
  {
    if ( i32 e = __check(); e ) [[unlikely]]
      return e;
    if ( path == nullptr ) [[unlikely]]
      return -error::invalid_arg;

    const i32 pf = __open_rule_dir(path, follow);
    if ( pf < 0 ) [[unlikely]]
      return pf;
    const i32 r = allow_fd(pf, allowed);
    (void)posix::close(pf);
    return r;
  }

public:
  [[nodiscard]] i32
  allow_port(u64 port, access_net allowed) noexcept
  {
    if ( i32 e = __check(); e ) [[unlikely]]
      return e;
    const access_net granted = allowed & __handled_net;
    if ( !any(granted) ) [[unlikely]]
      return -error::invalid_arg;
    return posix::landlock_add_net_port(__fd, port, bits(granted));
  }

  [[nodiscard]] i32
  restrict_self(u32 flags = 0) noexcept
  {
    if ( i32 e = __check(); e ) [[unlikely]]
      return e;
    if ( i32 nnp = micron::prctl(PR_SET_NO_NEW_PRIVS, 1UL); nnp < 0 ) [[unlikely]]
      return nnp;
    return posix::landlock_restrict_self(__fd, flags);
  }
};

enum class abi_policy : u8 {
  narrow = 0,
  strict,
};

[[nodiscard]] inline ruleset
try_ruleset(access_fs handled, access_net handled_net = access_net::none, scope scoped = scope::none,
            abi_policy pol = abi_policy::narrow) noexcept
{
  const i32 a = abi_level();
  if ( a <= 0 ) [[unlikely]]
    return ruleset::__failed(a < 0 ? a : -error::bad_syscall);      // abi_level() never answers 0 or -1

  const access_fs h = handled & static_cast<access_fs>(posix::landlock_fs_mask_for(a));
  const access_net hn = handled_net & static_cast<access_net>(posix::landlock_net_mask_for(a));
  const scope sc = scoped & static_cast<scope>(posix::landlock_scope_mask_for(a));

  if ( pol == abi_policy::strict ) {
    if ( any(handled & ~h) || any(handled_net & ~hn) || any(scoped & ~sc) ) [[unlikely]]
      return ruleset::__failed(-error::op_not_supported);
  }

  if ( !any(h) && !any(hn) && !any(sc) ) [[unlikely]]
    return ruleset::__failed(-error::invalid_arg);

  posix::landlock_ruleset_attr_t attr{ bits(h), bits(hn), bits(sc) };
  const i32 fd = posix::landlock_create_ruleset(&attr, posix::landlock_ruleset_size_for(a), 0);
  if ( fd < 0 ) [[unlikely]]
    return ruleset::__failed(fd);
  return ruleset::__adopt(fd, a, h, hn, sc);
}

[[nodiscard]] inline access_fs
dropped_fs(access_fs requested, const ruleset &rs) noexcept
{
  return requested & ~rs.handled();
}

[[nodiscard]] inline scope
dropped_scope(scope requested, const ruleset &rs) noexcept
{
  return requested & ~rs.scoped();
}

[[nodiscard]] inline access_net
dropped_net(access_net requested, const ruleset &rs) noexcept
{
  return requested & ~rs.handled_net();
}

};      // namespace landlock
};      // namespace sec
};      // namespace micron
