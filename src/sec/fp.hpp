//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

#include "fn.hpp"
#include "groups.hpp"
#include "landlock.hpp"
#include "namespaces.hpp"
#include "sandbox.hpp"
#include "seccomp.hpp"
#include "selinux.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// functional porcelain
//
//  auto r = sec::seccomp::policy<256>()
//         | sec::seccomp::arch_native()
//         | sec::seccomp::allow_group<sec::groups::baseline, sec::groups::io>()
//         | sec::seccomp::errno_on(SYS_openat, EPERM)
//         | sec::seccomp::deny_all()
//         | sec::seccomp::install();

namespace micron
{
namespace sec
{
namespace seccomp
{

using micron::sec::operator|;

// sources
template<usize N = 1024>
[[nodiscard]] constexpr filter_builder<N>
policy(void) noexcept
{
  return filter_builder<N>{};
}

// adaptors
struct __arch_fn {
  bool raw;

  template<typename B>
  constexpr B &&
  operator()(B &&b) const noexcept
  {
    if ( raw )
      b.require_native_arch_raw();
    else
      b.require_native_arch();
    return static_cast<B &&>(b);
  }
};

[[nodiscard]] constexpr auto
arch_native(void) noexcept
{
  return __arch_fn{ false };
}

[[nodiscard]] constexpr auto
arch_native_raw(void) noexcept
{
  return __arch_fn{ true };
}

template<usize K> struct __allow_fn {
  i32 nrs[K];

  template<typename B>
  constexpr B &&
  operator()(B &&b) const noexcept
  {
    for ( usize i = 0; i < K; ++i ) b.allow(nrs[i]);
    return static_cast<B &&>(b);
  }
};

template<typename... Ns>
[[nodiscard]] constexpr auto
allow(Ns... nrs) noexcept
{
  return __allow_fn<sizeof...(Ns)>{ { static_cast<i32>(nrs)... } };
}

template<is_syscall_group... Gs> struct __allow_group_fn {
  template<typename B>
  constexpr B &&
  operator()(B &&b) const noexcept
  {
    (
        [&b]<typename G>(G *) {
          for ( usize i = 0; i < G::count; ++i ) b.allow(G::calls[i]);
        }(static_cast<Gs *>(nullptr)),
        ...);
    return static_cast<B &&>(b);
  }
};

template<is_syscall_group... Gs>
[[nodiscard]] constexpr auto
allow_group(void) noexcept
{
  return __allow_group_fn<Gs...>{};
}

template<is_syscall_group... Gs> struct __deny_group_fn {
  u16 err;

  template<typename B>
  constexpr B &&
  operator()(B &&b) const noexcept
  {
    (
        [this, &b]<typename G>(G *) {
          for ( usize i = 0; i < G::count; ++i ) b.deny_errno(G::calls[i], err);
        }(static_cast<Gs *>(nullptr)),
        ...);
    return static_cast<B &&>(b);
  }
};

template<is_syscall_group... Gs>
[[nodiscard]] constexpr auto
deny_group(u16 err = static_cast<u16>(error::permissions)) noexcept
{
  return __deny_group_fn<Gs...>{ err };
}

struct __errno_on_fn {
  i32 nr;
  u16 err;

  template<typename B>
  constexpr B &&
  operator()(B &&b) const noexcept
  {
    b.deny_errno(nr, err);
    return static_cast<B &&>(b);
  }
};

[[nodiscard]] constexpr auto
errno_on(i32 nr, u16 err) noexcept
{
  return __errno_on_fn{ nr, err };
}

struct __trap_on_fn {
  i32 nr;

  template<typename B>
  constexpr B &&
  operator()(B &&b) const noexcept
  {
    b.trap_syscall(nr);
    return static_cast<B &&>(b);
  }
};

[[nodiscard]] constexpr auto
trap_on(i32 nr) noexcept
{
  return __trap_on_fn{ nr };
}

struct __when_fn {
  i32 nr;
  arg_cmp_t cmp;
  u32 action;

  template<typename B>
  constexpr B &&
  operator()(B &&b) const noexcept
  {
    b.action_if(nr, cmp, action);
    return static_cast<B &&>(b);
  }
};

[[nodiscard]] constexpr auto
allow_when(i32 nr, const arg_cmp_t &c) noexcept
{
  return __when_fn{ nr, c, act_allow() };
}

[[nodiscard]] constexpr auto
deny_when(i32 nr, const arg_cmp_t &c, u16 err = static_cast<u16>(error::permissions)) noexcept
{
  return __when_fn{ nr, c, act_errno(err) };
}

struct __seal_fn {
  u32 action;

  template<typename B>
  constexpr B &&
  operator()(B &&b) const noexcept
  {
    b.__seal(action);
    return static_cast<B &&>(b);
  }
};

[[nodiscard]] constexpr auto
deny_all(u16 err = static_cast<u16>(error::permissions)) noexcept
{
  return __seal_fn{ act_errno(err) };
}

[[nodiscard]] constexpr auto
kill_all(void) noexcept
{
  return __seal_fn{ posix::seccomp_ret_kill_process };
}

[[nodiscard]] constexpr auto
allow_all(void) noexcept
{
  return __seal_fn{ act_allow() };
}

// terminals
struct __install_fn {
  bool tsync;

  template<typename B>
  result<unit_t>
  operator()(B &&b) const noexcept
  {
    if ( !b.valid() ) [[unlikely]]
      return result<unit_t>{ error_t(-error::invalid_arg) };
    const int r = tsync ? load_tsync(b, true) : load(b, true);
    return to_unit(r);
  }
};

[[nodiscard]] constexpr auto
install(void) noexcept
{
  return __install_fn{ false };
}

[[nodiscard]] constexpr auto
install_tsync(void) noexcept
{
  return __install_fn{ true };
}

struct __install_notif_fn {
  template<typename B>
  result<i32>
  operator()(B &&b) const noexcept
  {
    if ( !b.valid() ) [[unlikely]]
      return result<i32>{ error_t(-error::invalid_arg) };
    const int r = load_notif(b, true);
    if ( r < 0 ) [[unlikely]]
      return result<i32>{ error_t(r) };
    return result<i32>{ r };
  }
};

[[nodiscard]] constexpr auto
install_notif(void) noexcept
{
  return __install_notif_fn{};
}

struct __build_fn {
  template<typename B>
  constexpr micron::remove_cvref_t<B>
  operator()(B &&b) const noexcept
  {
    return static_cast<micron::remove_cvref_t<B> &&>(b);
  }
};

[[nodiscard]] constexpr auto
build(void) noexcept
{
  return __build_fn{};
}

};      // namespace seccomp

// %%%%%%%%%%%%%%%%%%%%%
// landlock

namespace landlock
{

using micron::sec::operator|;

[[nodiscard]] inline ruleset
confine(access_fs handled, access_net handled_net = access_net::none, scope scoped = scope::none) noexcept
{
  return try_ruleset(handled, handled_net, scoped);
}

struct __beneath_fn {
  const char *path;
  access_fs access;

  ruleset &&
  operator()(ruleset &&r) const noexcept
  {
    if ( r.valid() ) {
      const i32 e = r.allow(path, access);
      if ( e < 0 ) r = ruleset::__failed(e);
    }
    return static_cast<ruleset &&>(r);
  }
};

[[nodiscard]] inline auto
beneath(const char *path, access_fs access) noexcept
{
  return __beneath_fn{ path, access };
}

struct __port_fn {
  u64 port;
  access_net access;

  ruleset &&
  operator()(ruleset &&r) const noexcept
  {
    if ( r.valid() ) {
      const i32 e = r.allow_port(port, access);
      if ( e < 0 ) r = ruleset::__failed(e);
    }
    return static_cast<ruleset &&>(r);
  }
};

[[nodiscard]] inline auto
port(u64 p, access_net a) noexcept
{
  return __port_fn{ p, a };
}

struct __enforce_fn {
  u32 flags;

  result<unit_t>
  operator()(ruleset &&r) const noexcept
  {
    if ( !r.valid() ) [[unlikely]]
      return result<unit_t>{ error_t(r.error()) };
    return to_unit(r.restrict_self(flags));
  }
};

[[nodiscard]] inline auto
enforce(u32 flags = 0) noexcept
{
  return __enforce_fn{ flags };
}

};      // namespace landlock

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bracket combinators

template<typename Fn>
  requires(micron::invocable<Fn, landlock::ruleset &> && __resultable<micron::invoke_result_t<Fn, landlock::ruleset &>>)
auto
with_ruleset(landlock::access_fs handled, Fn &&fn) -> result<__unit_if_void_t<micron::invoke_result_t<Fn, landlock::ruleset &>>>
{
  using R = result<__unit_if_void_t<micron::invoke_result_t<Fn, landlock::ruleset &>>>;
  landlock::ruleset rs = landlock::try_ruleset(handled);
  if ( !rs.valid() ) [[unlikely]]
    return R{ error_t(rs.error()) };
  if constexpr ( micron::is_void_v<micron::invoke_result_t<Fn, landlock::ruleset &>> ) {
    micron::forward<Fn>(fn)(rs);
    return R{ unit_t{} };
  } else {
    return R{ micron::forward<Fn>(fn)(rs) };
  }
}

template<typename Fn>
  requires micron::invocable<Fn>
[[nodiscard]] inline result<sandbox::exit_status>
in_namespace(ns::ns_kind kinds, Fn &&fn) noexcept
{
  sandbox box;
  box.namespaces(kinds);
  return box.run_to_completion(static_cast<i32 (*)(void)>(fn));
}

// WARNING: only seccomp
template<usize N>
[[nodiscard]] inline result<sandbox::exit_status>
confined(const seccomp::filter_builder<N> &fb, i32 (*body)(void)) noexcept
{
  using R = result<sandbox::exit_status>;
  if ( !fb.valid() ) [[unlikely]]
    return R{ error_t(-error::invalid_arg) };
  sandbox box;
  box.seccomp(fb);
  if ( !box.configured() ) [[unlikely]]
    return R{ error_t(box.config_fault().err) };
  return box.run_to_completion(body);
}

template<usize N> result<sandbox::exit_status> confined(const seccomp::filter_builder<N> &&, i32 (*)(void)) = delete;

namespace selinux
{

using micron::sec::operator|;

// path | sec::selinux::relabel_c(ctx)
[[nodiscard]] inline auto
relabel_c(const context &c) noexcept
{
  return [c](const char *path) { return selinux::set_label(path, c); };
}

// ctx | sec::selinux::set_exec_c()
[[nodiscard]] inline auto
set_exec_c(void) noexcept
{
  return [](const context &c) { return selinux::set_attr(attr::exec, c); };
}

// path | sec::selinux::label_of_c()
[[nodiscard]] inline auto
label_of_c(void) noexcept
{
  return [](const char *path) { return selinux::label_of(path); };
}

};      // namespace selinux

};      // namespace sec
};      // namespace micron
