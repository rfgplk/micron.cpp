//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../string/fixed_string.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../linux/process/capabilities.hpp"
#include "../linux/sys/fcntl.hpp"
#include "../linux/sys/limits.hpp"
#include "../linux/sys/mount.hpp"

#include "groups.hpp"
#include "landlock.hpp"
#include "namespaces.hpp"
#include "seccomp.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// type-level policy composition
//
//  using jail_policy = sec::seccomp_policy<
//      sec::allow<sec::groups::baseline, sec::groups::io>,
//      sec::deny <sec::groups::network>>;

namespace micron
{
namespace sec
{

namespace __impl
{

[[nodiscard]] constexpr const char *
__rel_put_old(const char *nr, const char *po) noexcept
{
  if ( nr == nullptr || po == nullptr ) return nullptr;
  usize n = 0;
  while ( nr[n] != '\0' ) ++n;
  while ( n > 1 && nr[n - 1] == '/' ) --n;
  if ( n == 0 ) return nullptr;
  if ( n == 1 && nr[0] == '/' ) return po;
  for ( usize i = 0; i < n; ++i )
    if ( po[i] == '\0' || po[i] != nr[i] ) return nullptr;
  if ( po[n] != '/' ) return nullptr;
  return po + n;
}

// WARNING: MS_REC is ignored by a MS_REMOUNT|MS_BIND remount
[[nodiscard]] inline i32
__remount_attrs(const char *target, bool recursive, unsigned long keep_flags, u64 attr_set, bool rdonly) noexcept
{
  if ( recursive ) {
    posix::mount_attr_t at{};
    at.attr_set = attr_set | (rdonly ? posix::mount_attr_rdonly : 0ull);
    if ( at.attr_set == 0 ) return 0;
    return posix::mount_setattr(posix::at_fdcwd, target, static_cast<u32>(posix::at_recursive), at);
  }
  unsigned long fl = (keep_flags & ~posix::ms_rec) | posix::ms_bind | posix::ms_remount;
  if ( rdonly ) fl |= posix::ms_rdonly;
  if ( attr_set & posix::mount_attr_nosuid ) fl |= posix::ms_nosuid;
  if ( attr_set & posix::mount_attr_nodev ) fl |= posix::ms_nodev;
  if ( attr_set & posix::mount_attr_noexec ) fl |= posix::ms_noexec;
  return static_cast<i32>(posix::mount(nullptr, target, nullptr, fl, nullptr));
}

[[nodiscard]] inline i32
__remount_ro(const char *target, bool recursive, unsigned long keep_flags = 0) noexcept
{
  return __remount_attrs(target, recursive, keep_flags, 0, true);
}

inline constexpr u64 __bind_default_attrs = posix::mount_attr_nosuid | posix::mount_attr_nodev;

};      // namespace __impl

// tags
struct seccomp_policy_tag {
};

struct capability_policy_tag {
};

struct rlimit_policy_tag {
};

struct filesystem_policy_tag {
};

struct namespace_policy_tag {
};

struct landlock_policy_tag {
};

template<typename T>
concept is_policy = requires { typename T::policy_tag; };

template<typename T>
concept is_seccomp_policy = is_policy<T> && micron::is_same_v<typename T::policy_tag, seccomp_policy_tag>;
template<typename T>
concept is_capability_policy = is_policy<T> && micron::is_same_v<typename T::policy_tag, capability_policy_tag>;
template<typename T>
concept is_rlimit_policy = is_policy<T> && micron::is_same_v<typename T::policy_tag, rlimit_policy_tag>;
template<typename T>
concept is_filesystem_policy = is_policy<T> && micron::is_same_v<typename T::policy_tag, filesystem_policy_tag>;
template<typename T>
concept is_namespace_policy = is_policy<T> && micron::is_same_v<typename T::policy_tag, namespace_policy_tag>;
template<typename T>
concept is_landlock_policy = is_policy<T> && micron::is_same_v<typename T::policy_tag, landlock_policy_tag>;

// %%%%%%%%%%%%%%%%%%%%%
// seccomp rules

template<is_syscall_group... Gs> struct allow {
  using policy_tag = syscall_group_tag;

  template<usize M>
  static constexpr void
  emit(seccomp::filter_builder<M> &fb) noexcept
  {
    (
        [&fb]<typename G>(G *) {
          for ( usize i = 0; i < G::count; ++i ) fb.allow(G::calls[i]);
        }(static_cast<Gs *>(nullptr)),
        ...);
  }
};

template<is_syscall_group... Gs> struct deny {
  using policy_tag = syscall_group_tag;

  template<usize M>
  static constexpr void
  emit(seccomp::filter_builder<M> &fb) noexcept
  {
    (
        [&fb]<typename G>(G *) {
          for ( usize i = 0; i < G::count; ++i ) fb.deny_errno(G::calls[i], static_cast<u16>(error::permissions));
        }(static_cast<Gs *>(nullptr)),
        ...);
  }
};

template<i32... Nrs> struct allow_calls {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = { Nrs... };
  static constexpr usize count = sizeof...(Nrs);

  template<usize M>
  static constexpr void
  emit(seccomp::filter_builder<M> &fb) noexcept
  {
    (fb.allow(Nrs), ...);
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// argument predicate rules
// WARNING: micron's thread spawn issues clone3
template<u16 Err = static_cast<u16>(error::permissions)> struct no_new_namespaces {
  using policy_tag = syscall_group_tag;

  static constexpr u64 ns_bits = posix::clone_newns | posix::clone_newuts | posix::clone_newipc | posix::clone_newuser | posix::clone_newpid
                                 | posix::clone_newnet | posix::clone_newcgroup | posix::clone_newtime;

  template<usize M>
  static constexpr void
  emit(seccomp::filter_builder<M> &fb) noexcept
  {
    fb.allow_if(SYS_clone, seccomp::arg_no_bits(0, ns_bits));
    fb.deny_errno(SYS_clone, Err);
    fb.deny_errno(SYS_clone3, Err);
    fb.deny_errno(SYS_unshare, Err);
    fb.deny_errno(SYS_setns, Err);
  }
};

template<u16 Err = static_cast<u16>(error::permissions)> struct no_tty_injection {
  using policy_tag = syscall_group_tag;

  template<usize M>
  static constexpr void
  emit(seccomp::filter_builder<M> &fb) noexcept
  {
    seccomp::deny_tty_injection(fb, Err);
  }
};

template<u16 Err = static_cast<u16>(error::permissions)> struct no_raw_socket_families {
  using policy_tag = syscall_group_tag;

  template<usize M>
  static constexpr void
  emit(seccomp::filter_builder<M> &fb) noexcept
  {
    seccomp::deny_raw_socket_families(fb, Err);
  }
};

template<i32 Nr, u16 Err> struct errno_call {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = { Nr };
  static constexpr usize count = 1;

  template<usize M>
  static constexpr void
  emit(seccomp::filter_builder<M> &fb) noexcept
  {
    fb.deny_errno(Nr, Err);
  }
};

// %%%%%%%%%%%%%%%%%%%%%%
// seccomp policies

template<usize Max, typename... Rules> struct seccomp_policy_n {
  using policy_tag = seccomp_policy_tag;
  static constexpr usize capacity = Max;

  static constexpr seccomp::filter_builder<Max>
  build(u32 default_action) noexcept
  {
    seccomp::filter_builder<Max> fb;
    fb.require_native_arch();
    (Rules::template emit<Max>(fb), ...);
    fb.__seal(default_action);
    return fb;
  }

  // WARNING: TSYNC, see CVE-2017-5206
  static int
  apply(void) noexcept
  {
    auto fb = build(seccomp::act_errno(static_cast<u16>(error::permissions)));
    return seccomp::load_tsync(fb, true);
  }

  static int
  apply_this_thread_only(void) noexcept
  {
    auto fb = build(seccomp::act_errno(static_cast<u16>(error::permissions)));
    return seccomp::load(fb, true);
  }
};

template<typename... Rules> using seccomp_policy = seccomp_policy_n<1024, Rules...>;

template<usize Max, typename... Rules> struct seccomp_strict_policy_n {
  using policy_tag = seccomp_policy_tag;
  static constexpr usize capacity = Max;

  static constexpr seccomp::filter_builder<Max>
  build(void) noexcept
  {
    seccomp::filter_builder<Max> fb;
    fb.require_native_arch();
    (Rules::template emit<Max>(fb), ...);
    fb.default_kill();
    return fb;
  }

  // TSYNC
  static int
  apply(void) noexcept
  {
    auto fb = build();
    return seccomp::load_tsync(fb, true);
  }

  static int
  apply_this_thread_only(void) noexcept
  {
    auto fb = build();
    return seccomp::load(fb, true);
  }
};

template<typename... Rules> using seccomp_strict_policy = seccomp_strict_policy_n<1024, Rules...>;

// %%%%%%%%%%%%%%%%%%%%%%%%
// capabilities

template<micron::cap... Cs> struct capability_policy {
  using policy_tag = capability_policy_tag;

  static int
  apply_pre_uid(void) noexcept
  {
    (void)posix::cap_set_keepcaps(1);
    u64 keep = 0;
    ((keep |= micron::cap_bit(Cs)), ...);
    const u64 drop = ~keep & micron::cap_all_mask;
    int err = 0;
    for ( u32 i = 0; i < static_cast<u32>(micron::cap::__count); ++i ) {
      if ( !(drop & (u64(1) << i)) ) continue;
      const int r = micron::drop_bounding(static_cast<micron::cap>(i));
      if ( r < 0 && r != -error::invalid_arg && err == 0 ) err = r;
    }
    return err;
  }

  static int
  apply_post_uid(void) noexcept
  {
    micron::ucap_set_t cs = micron::ucap_set_t::none();
    (cs.grant(Cs), ...);
    const int r = micron::set_caps(cs);
    if ( r < 0 ) return r;
    ((void)micron::raise_ambient(Cs), ...);
    return 0;
  }

  static int
  apply(void) noexcept
  {
    const int r = apply_pre_uid();
    if ( r < 0 ) return r;
    return apply_post_uid();
  }
};

struct capability_policy_none {
  using policy_tag = capability_policy_tag;

  static int
  apply_pre_uid(void) noexcept
  {
    int err = 0;
    for ( u32 i = 0; i < static_cast<u32>(micron::cap::__count); ++i ) {
      const int r = micron::drop_bounding(static_cast<micron::cap>(i));
      if ( r < 0 && r != -error::invalid_arg && err == 0 ) err = r;      // EINVAL: not a cap this kernel knows
    }
    const int a = micron::clear_ambient();
    return err != 0 ? err : a;
  }

  static int
  apply_post_uid(void) noexcept
  {
    return micron::drop_all_caps();
  }

  static int
  apply(void) noexcept
  {
    const int r = apply_pre_uid();
    if ( r < 0 ) return r;
    return apply_post_uid();
  }
};

struct capability_policy_keep_all {
  using policy_tag = capability_policy_tag;

  static int
  apply_pre_uid(void) noexcept
  {
    return 0;
  }

  static int
  apply_post_uid(void) noexcept
  {
    return 0;
  }

  static int
  apply(void) noexcept
  {
    return 0;
  }
};

// %%%%%%%%%%%%%
// rlimits

template<posix::rlim_t R, u64 Soft, u64 Hard = Soft> struct limit {
  static constexpr posix::rlim_t resource = R;
  static constexpr u64 soft = Soft;
  static constexpr u64 hard = Hard;
};

template<typename... Ls> struct rlimit_policy {
  using policy_tag = rlimit_policy_tag;

  static int
  apply(void) noexcept
  {
    int err = 0;
    (
        [&err]<typename L>(L *) {
          posix::rlimit64_t rl{};
          rl.rlim_cur = L::soft;
          rl.rlim_max = L::hard;
          const int r = static_cast<int>(posix::set_process_limits(0, L::resource, rl));
          if ( r < 0 && err == 0 ) err = r;
        }(static_cast<Ls *>(nullptr)),
        ...);
    return err;
  }
};

struct rlimit_policy_none {
  using policy_tag = rlimit_policy_tag;

  static int
  apply(void) noexcept
  {
    return 0;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%
// filesystem shaping

template<micron::fixed_string P> struct chroot_to {
  static int
  apply(void) noexcept
  {
    const int r = static_cast<int>(posix::chroot(P.c_str()));
    if ( r < 0 ) return r;
    return static_cast<int>(posix::chdir("/"));
  }
};

// pivot_root, then detach the old root
template<micron::fixed_string New, micron::fixed_string Put> struct pivot_to {
  static int
  apply(void) noexcept
  {
    const char *rel = __impl::__rel_put_old(New.c_str(), Put.c_str());
    if ( rel == nullptr ) return -error::invalid_arg;
    int r = static_cast<int>(posix::pivot_root(New.c_str(), Put.c_str()));
    if ( r < 0 ) return r;
    r = static_cast<int>(posix::chdir("/"));
    if ( r < 0 ) return r;
    return static_cast<int>(posix::umount2(rel, posix::mnt_detach));
  }
};

template<micron::fixed_string New, micron::fixed_string Put> struct pivot_to_keep_old {
  static int
  apply(void) noexcept
  {
    const int r = static_cast<int>(posix::pivot_root(New.c_str(), Put.c_str()));
    if ( r < 0 ) return r;
    return static_cast<int>(posix::chdir("/"));
  }
};

template<micron::fixed_string Target, i32 Flags = posix::mnt_detach> struct detach {
  static int
  apply(void) noexcept
  {
    return static_cast<int>(posix::umount2(Target.c_str(), Flags));
  }
};

template<micron::fixed_string Src, micron::fixed_string Dst, bool ReadOnly = false, bool Recursive = true, bool Harden = true> struct bind {
  static int
  apply(void) noexcept
  {
    constexpr unsigned long fl = Recursive ? (posix::ms_bind | posix::ms_rec) : posix::ms_bind;
    const int r = static_cast<int>(posix::mount(Src.c_str(), Dst.c_str(), nullptr, fl, nullptr));
    if ( r < 0 ) return r;
    constexpr u64 attrs = Harden ? __impl::__bind_default_attrs : 0ull;
    if constexpr ( ReadOnly || Harden ) return __impl::__remount_attrs(Dst.c_str(), Recursive, fl, attrs, ReadOnly);
    return r;
  }
};

template<micron::fixed_string Dst, micron::fixed_string Opts = micron::fixed_string("size=64m")> struct tmpfs {
  static int
  apply(void) noexcept
  {
    return static_cast<int>(
        posix::mount("tmpfs", Dst.c_str(), "tmpfs", posix::ms_nosuid | posix::ms_nodev | posix::ms_strictatime, Opts.c_str()));
  }
};

template<micron::fixed_string Dst> struct proc_mount {
  static int
  apply(void) noexcept
  {
    return static_cast<int>(posix::mount("proc", Dst.c_str(), "proc", posix::ms_nosuid | posix::ms_nodev | posix::ms_noexec, nullptr));
  }
};

struct make_private {
  static int
  apply(void) noexcept
  {
    return static_cast<int>(posix::mount("none", "/", nullptr, posix::ms_rec | posix::ms_private, nullptr));
  }
};

// WARNING: STOPS AT THE FIRST FAILURE
template<typename... Ds> struct filesystem_policy {
  using policy_tag = filesystem_policy_tag;

  static int
  apply(void) noexcept
  {
    int err = 0;
    (
        [&err]<typename D>(D *) {
          if ( err < 0 ) return;      // a later directive's preconditions no longer hold
          const int r = D::apply();
          if ( r < 0 ) err = r;
        }(static_cast<Ds *>(nullptr)),
        ...);
    return err;
  }
};

struct filesystem_policy_none {
  using policy_tag = filesystem_policy_tag;

  static int
  apply(void) noexcept
  {
    return 0;
  }
};

// %%%%%%%%%%%%%%%%%%%
// namespaces
// WARNING: THIS CANNOT ENTER A PID NAMESPACE
template<ns::ns_kind... Ks> struct namespace_policy {
  using policy_tag = namespace_policy_tag;
  static constexpr ns::ns_kind kinds = (ns::ns_kind::none | ... | Ks);
  static constexpr bool has_user = ns::any(kinds & ns::ns_kind::user);

  static_assert(!ns::any(kinds & ns::ns_kind::pid), "namespace_policy cannot enter a pid namespace");

  static int
  apply(void) noexcept
  {
    if constexpr ( has_user )
      return ns::unshare_user_as_root(kinds & ~ns::ns_kind::user);
    else
      return ns::unshare(kinds);
  }
};

struct namespace_policy_none {
  using policy_tag = namespace_policy_tag;
  static constexpr ns::ns_kind kinds = ns::ns_kind::none;
  static constexpr bool has_user = false;

  static int
  apply(void) noexcept
  {
    return 0;
  }
};

// %%%%%%%%%%%%%%%%%%
// landlock

template<micron::fixed_string P, landlock::access_fs A> struct beneath {
  static constexpr landlock::access_fs access = A;

  static i32
  add(landlock::ruleset &r) noexcept
  {
    return r.allow(P.c_str(), A);
  }
};

template<typename... Rs> struct landlock_policy {
  using policy_tag = landlock_policy_tag;

  static constexpr landlock::access_fs handled = (landlock::access_fs::none | ... | Rs::access);

  static int
  apply(void) noexcept
  {
    if ( !landlock::available() ) return -error::bad_syscall;

    landlock::ruleset rs = landlock::try_ruleset(handled);
    if ( !rs.valid() ) return rs.error();

    int err = 0;
    (
        [&rs, &err]<typename R>(R *) {
          const i32 r = R::add(rs);
          if ( r < 0 && err == 0 ) err = r;
        }(static_cast<Rs *>(nullptr)),
        ...);
    if ( err < 0 ) return err;
    return rs.restrict_self();
  }
};

struct landlock_policy_none {
  using policy_tag = landlock_policy_tag;
  static constexpr landlock::access_fs handled = landlock::access_fs::none;

  static int
  apply(void) noexcept
  {
    return 0;
  }
};

};      // namespace sec
};      // namespace micron
