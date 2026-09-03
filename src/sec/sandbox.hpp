//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../types.hpp"

#include "../exit.hpp"
#include "../linux/io/sys.hpp"
#include "../linux/process/capabilities.hpp"
#include "../linux/process/fork.hpp"
#include "../linux/process/wait.hpp"
#include "../linux/sys/exec.hpp"
#include "../linux/sys/fcntl.hpp"
#include "../linux/sys/limits.hpp"
#include "../linux/sys/mount.hpp"
#include "../linux/sys/prctl.hpp"
#include "../linux/sys/signal.hpp"

#include "bits.hpp"
#include "landlock.hpp"
#include "namespaces.hpp"
#include "policy.hpp"
#include "seccomp.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// sandbox
//
// ORDER OF THE STAGES IS SECURITY CRITICAL
//
//   1  namespaces      unshare, mapping ourselves to root first when a user namespace is involved
//   2  filesystem      make-private, binds, pivot_root/chroot, chdir("/"), DETACH the old root
//   3  rlimits
//   4  stdio           dup2 the caller's descriptors into 0/1/2
//   5  descriptors     close everything the child was not given
//   6  landlock        before seccomp
//   7  caps pre-uid    narrow the bounding set + clear ambient, while CAP_SETPCAP is still held
//   8  seccomp         installs the filter AND sets no_new_privs
//   9  nnp / dumpable
//  10  credentials     setgroups(0) -> setgid -> setuid, then VERIFY all four ids actually changed
//  11  caps post-uid   zero whatever effective/permitted/inheritable survived
//  12  signal mask     execve resets dispositions but NOT the mask
//  13  execve

namespace micron
{
namespace sec
{

#define __micron_sec_sandbox_has_landlock_scope 1
#define __micron_sec_sandbox_has_secure_defaults 1

enum class stage : i32 {
  none = 0,
  fork_failed = 1,
  namespaces = 2,
  filesystem = 3,
  rlimits = 4,
  stdio = 5,
  descriptors = 6,
  landlock = 7,
  seccomp = 8,
  caps_pre = 9,
  no_new_privs = 10,
  credentials = 11,
  caps_post = 12,
  signal_mask = 13,
  exec = 14,
};

[[nodiscard]] constexpr const char *
name_of(stage s) noexcept
{
  switch ( s ) {
  case stage::none:
    return "none";
  case stage::fork_failed:
    return "fork";
  case stage::namespaces:
    return "namespaces";
  case stage::filesystem:
    return "filesystem";
  case stage::rlimits:
    return "rlimits";
  case stage::stdio:
    return "stdio";
  case stage::descriptors:
    return "descriptors";
  case stage::landlock:
    return "landlock";
  case stage::seccomp:
    return "seccomp";
  case stage::caps_pre:
    return "caps_pre_uid";
  case stage::no_new_privs:
    return "no_new_privs";
  case stage::credentials:
    return "credentials";
  case stage::caps_post:
    return "caps_post_uid";
  case stage::signal_mask:
    return "signal_mask";
  case stage::exec:
    return "exec";
  }
  return "?";
}

struct fault_t {
  stage where = stage::none;
  i32 err = 0;

  [[nodiscard]] constexpr explicit
  operator bool(void) const noexcept
  {
    return where != stage::none;
  }

  [[nodiscard]] const char *
  stage_name(void) const noexcept
  {
    return name_of(where);
  }
};

namespace __impl
{

struct wire_fault {
  i32 where;
  i32 err;
};

[[nodiscard]] inline bool
__fd_is_dir(i32 fd) noexcept
{
  if ( fd < 0 ) return false;
  posix::stat_t st{};
  if ( posix::fstat(fd, st) < 0 ) return false;
  return (st.st_mode & 0170000u) == 0040000u;      // S_IFMT / S_IFDIR
}

[[noreturn]] inline void
__child_fail(i32 pipe_fd, stage where, i32 err) noexcept
{
  wire_fault w{ static_cast<i32>(where), err };
  if ( pipe_fd >= 0 ) (void)posix::write(pipe_fd, &w, sizeof(w));
  micron::sys_group_exit(120);
}

};      // namespace __impl

// configuration
struct mount_spec {
  const char *source = nullptr;
  const char *target = nullptr;
  const char *fstype = nullptr;
  const char *options = nullptr;
  unsigned long flags = 0;
  bool read_only = false;
  u64 attrs = 0;

  [[nodiscard]] constexpr bool
  recursive(void) const noexcept
  {
    return (flags & posix::ms_rec) != 0;
  }

  [[nodiscard]] constexpr bool
  needs_remount(void) const noexcept
  {
    return read_only || attrs != 0;
  }
};

class sandbox
{
public:
  static constexpr usize max_mounts = 12;
  static constexpr usize max_landlock_rules = 12;

private:
  ns::ns_kind __ns_kinds = ns::ns_kind::none;

  const char *__new_root = nullptr;
  const char *__put_old = nullptr;
  bool __use_chroot = false;
  bool __make_private = true;

  mount_spec __mounts[max_mounts]{};
  usize __mount_count = 0;

  landlock::access_fs __ll_handled = landlock::access_fs::none;
  landlock::access_net __ll_handled_net = landlock::access_net::none;
  landlock::scope __ll_scoped = landlock::scope::none;
  bool __ll_handled_explicit = false;

  struct ll_rule {
    const char *path;
    landlock::access_fs access;
  } __ll_rules[max_landlock_rules]{};

  usize __ll_count = 0;

  const bpf::insn_t *__filter = nullptr;
  u16 __filter_len = 0;

  fault_t __cfg_fault{};

  void
  __config_fail(stage where, i32 err) noexcept
  {
    if ( !__cfg_fault ) __cfg_fault = fault_t{ where, err };      // first failure wins
  }

  bool __want_nnp = true;
  bool __want_undumpable = true;
  bool __drop_all_caps = true;

  bool __change_ids = false;
  posix::uid_t __uid = 0;
  posix::gid_t __gid = 0;

  i32 __stdin_fd = -1;
  i32 __stdout_fd = -1;
  i32 __stderr_fd = -1;
  i32 __keep_fds[8]{};
  usize __keep_count = 0;
  bool __close_others = true;

  struct rl_spec {
    posix::rlim_t res;
    u64 soft;
    u64 hard;
  } __rlimits[8]{};

  usize __rlimit_count = 0;

  static constexpr u64 max_fd_sweep = 1u << 20;

  // close every descriptor the child was not given
  [[nodiscard]] i32
  __close_extra(i32 ep, u64 ceiling) const noexcept
  {
    i32 keep[8 + 1];
    usize n = 0;
    for ( usize i = 0; i < __keep_count; ++i )
      if ( __keep_fds[i] >= 3 ) keep[n++] = __keep_fds[i];
    if ( ep >= 3 ) keep[n++] = ep;
    for ( usize i = 1; i < n; ++i ) {
      const i32 v = keep[i];
      usize j = i;
      while ( j > 0 && keep[j - 1] > v ) {
        keep[j] = keep[j - 1];
        --j;
      }
      keep[j] = v;
    }

    bool ranged = micron::kernel::since(micron::kernel::feature::close_range);
    if ( ranged ) {
      unsigned int lo = 3;
      for ( usize i = 0; i < n && ranged; ++i ) {
        const unsigned int k = static_cast<unsigned int>(keep[i]);
        if ( k < lo ) continue;      // a duplicate in the keep set, or below the floor
        if ( k > lo ) {
          const i32 r = posix::close_range(lo, k - 1, 0);
          if ( r == -error::bad_syscall )
            ranged = false;
          else if ( r < 0 )
            return r;
        }
        lo = k + 1;
      }
      if ( ranged ) {
        const i32 r = posix::close_range(lo, ~0u, 0);
        if ( r == -error::bad_syscall )
          ranged = false;
        else if ( r < 0 )
          return r;
        else
          return 0;
      }
    }

    u64 hi = ceiling;
    if ( hi < 1024 ) hi = 1024;
    if ( hi > max_fd_sweep ) [[unlikely]]
      return -error::overflow;      // no close_range, and more fds than a loop can walk
    for ( u64 fd = 3; fd < hi; ++fd ) {
      bool k = false;
      for ( usize i = 0; i < n; ++i )
        if ( static_cast<u64>(keep[i]) == fd ) k = true;
      if ( !k ) (void)posix::close(static_cast<i32>(fd));
    }
    return 0;
  }

  // child
  [[noreturn]] void
  __child(i32 ep, const char *path, char *const *argv, char *const *envp, i32 (*body)(void)) const noexcept
  {
    i32 r = 0;

    // namespaces
    if ( ns::any(__ns_kinds) ) {
      if ( ns::any(__ns_kinds & ns::ns_kind::user) )
        r = ns::unshare_user_as_root(__ns_kinds & ~ns::ns_kind::user);
      else
        r = ns::unshare(__ns_kinds);
      if ( r < 0 ) __impl::__child_fail(ep, stage::namespaces, r);

      // WARNING: unshare(CLONE_NEWPID) does ___NOT___ move the caller
      if ( ns::any(__ns_kinds & ns::ns_kind::pid) ) {
        const int inner = micron::try_fork();
        if ( inner < 0 ) __impl::__child_fail(ep, stage::namespaces, inner);
        if ( inner > 0 ) {
          if ( ep >= 0 ) (void)posix::close(ep);
          int st = 0;
          const posix::pid_t got = micron::waitpid(inner, &st, 0);
          if ( got < 0 ) micron::sys_group_exit(125);
          if ( micron::wifsignaled(st) ) micron::sys_group_exit(128 + micron::wtermsig(st));
          micron::sys_group_exit(micron::wifexited(st) ? micron::wexitstatus(st) : 125);
        }
      }
    }

    // filesystem
    if ( ns::any(__ns_kinds & ns::ns_kind::mount) && __make_private ) {
      r = static_cast<i32>(posix::mount("none", "/", nullptr, posix::ms_rec | posix::ms_private, nullptr));
      if ( r < 0 ) __impl::__child_fail(ep, stage::filesystem, r);
    }
    for ( usize i = 0; i < __mount_count; ++i ) {
      const mount_spec &m = __mounts[i];
      r = static_cast<i32>(posix::mount(m.source, m.target, m.fstype, m.flags, m.options));
      if ( r < 0 ) __impl::__child_fail(ep, stage::filesystem, r);
      if ( m.needs_remount() ) {
        r = __impl::__remount_attrs(m.target, m.recursive(), m.flags, m.attrs, m.read_only);
        if ( r < 0 ) __impl::__child_fail(ep, stage::filesystem, r);
      }
    }
    if ( __new_root != nullptr ) {
      if ( __use_chroot ) {
        r = static_cast<i32>(posix::chroot(__new_root));
        if ( r < 0 ) __impl::__child_fail(ep, stage::filesystem, r);
        r = static_cast<i32>(posix::chdir("/"));
        if ( r < 0 ) __impl::__child_fail(ep, stage::filesystem, r);
      } else {
        const char *old_here = __impl::__rel_put_old(__new_root, __put_old);
        if ( old_here == nullptr ) __impl::__child_fail(ep, stage::filesystem, -error::invalid_arg);

        r = static_cast<i32>(posix::pivot_root(__new_root, __put_old));
        if ( r < 0 ) __impl::__child_fail(ep, stage::filesystem, r);
        r = static_cast<i32>(posix::chdir("/"));
        if ( r < 0 ) __impl::__child_fail(ep, stage::filesystem, r);

        r = static_cast<i32>(posix::umount2(old_here, posix::mnt_detach));
        if ( r < 0 ) __impl::__child_fail(ep, stage::filesystem, r);
      }
    }

    u64 fd_ceiling = 0;
    if ( __close_others ) {
      posix::rlimit64_t rl{};
      if ( posix::get_process_limits(0, posix::rlimit_nofile, rl) == 0 ) fd_ceiling = rl.rlim_max > rl.rlim_cur ? rl.rlim_max : rl.rlim_cur;
    }

    // rlimits
    for ( usize i = 0; i < __rlimit_count; ++i ) {
      posix::rlimit64_t rl{};
      rl.rlim_cur = __rlimits[i].soft;
      rl.rlim_max = __rlimits[i].hard;
      r = static_cast<i32>(posix::set_process_limits(0, __rlimits[i].res, rl));
      if ( r < 0 ) __impl::__child_fail(ep, stage::rlimits, r);
    }

    // stdio
    // WARNING: dup2 onto 0/1/2 can clobber a source that is 0/1/2
    {
      i32 src[3] = { __stdin_fd, __stdout_fd, __stderr_fd };
      i32 tmp[3] = { -1, -1, -1 };
      for ( usize i = 0; i < 3; ++i ) {
        if ( src[i] < 0 || src[i] >= 3 ) continue;
        const i32 hi = posix::fcntl(src[i], posix::f_dupfd, 3);
        if ( hi < 0 ) __impl::__child_fail(ep, stage::stdio, hi);
        tmp[i] = hi;
        src[i] = hi;
      }
      for ( usize i = 0; i < 3; ++i )
        if ( src[i] >= 0 && posix::dup2(src[i], static_cast<i32>(i)) < 0 ) __impl::__child_fail(ep, stage::stdio, -error::bad_file_number);
      for ( usize i = 0; i < 3; ++i )
        if ( tmp[i] >= 0 ) (void)posix::close(tmp[i]);
    }

    // descriptors, must be before the seccomp stage
    if ( __close_others ) {
      r = __close_extra(ep, fd_ceiling);
      if ( r < 0 ) __impl::__child_fail(ep, stage::descriptors, r);
    }

    // landlock, also before seccomp
    if ( landlock::any(__ll_handled) || landlock::any(__ll_handled_net) || landlock::any(__ll_scoped) || __ll_count > 0 ) {
      landlock::ruleset rs = landlock::try_ruleset(__ll_handled, __ll_handled_net, __ll_scoped);
      if ( !rs.valid() ) __impl::__child_fail(ep, stage::landlock, rs.error());
      for ( usize i = 0; i < __ll_count; ++i ) {
        const i32 a = rs.allow(__ll_rules[i].path, __ll_rules[i].access);
        if ( a < 0 ) __impl::__child_fail(ep, stage::landlock, a);
      }
      const i32 e = rs.restrict_self();
      if ( e < 0 ) __impl::__child_fail(ep, stage::landlock, e);
    }

    // capabilities; needs CAP_SETPCAP, and must precede seccomp
    bool has_caps_to_drop = false;
    if ( __drop_all_caps ) {
      //   holds CAP_SETPCAP        -> drop, falat failure
      //   no CAP_SETPCAP, NNP on   -> skip
      //   no CAP_SETPCAP, NNP OFF  -> FATAL
      const bool can_drop_bounding = micron::has_cap(micron::cap::setpcap);
      if ( !can_drop_bounding && !__want_nnp ) __impl::__child_fail(ep, stage::caps_pre, -error::permissions);
      const micron::ucap_set_t __live = micron::this_caps();
      // EFFECTIVE and PERMITTED only
      has_caps_to_drop = static_cast<u64>(__live.effective) != 0 || static_cast<u64>(__live.permitted) != 0;
      if ( can_drop_bounding ) {
        for ( u32 i = 0; i < static_cast<u32>(micron::cap::__count); ++i ) {
          const i32 d = micron::drop_bounding(static_cast<micron::cap>(i));
          if ( d < 0 && d != -error::invalid_arg ) __impl::__child_fail(ep, stage::caps_pre, d);
        }
      }

      // ambient caps need no privilege to clear, so this is unconditional
      r = micron::clear_ambient();
      if ( r < 0 ) __impl::__child_fail(ep, stage::caps_pre, r);
      constexpr i32 want
          = posix::secbit_noroot | posix::secbit_noroot_locked | posix::secbit_no_setuid_fixup | posix::secbit_no_setuid_fixup_locked;
      if ( can_drop_bounding ) {
        const i32 sb = posix::cap_set_securebits(want);
        if ( sb < 0 ) __impl::__child_fail(ep, stage::caps_pre, sb);
      }
    }

    // seccomp, must precede the cred drop
    if ( __filter != nullptr && __filter_len > 0 ) {
      bpf::fprog_t prog{ __filter_len, const_cast<bpf::insn_t *>(__filter) };
      r = seccomp::load_raw(prog, true, 0);
      if ( r < 0 ) __impl::__child_fail(ep, stage::seccomp, r);
    }

    // nnp / dumpable
    if ( __want_nnp ) {
      r = micron::prctl(PR_SET_NO_NEW_PRIVS, 1UL);
      if ( r < 0 ) __impl::__child_fail(ep, stage::no_new_privs, r);
    }
    if ( __want_undumpable ) {
      r = micron::prctl(PR_SET_DUMPABLE, 0UL);
      if ( r < 0 ) __impl::__child_fail(ep, stage::no_new_privs, r);
    }

    // credentials
    if ( __change_ids ) {
      if ( !ns::any(__ns_kinds & ns::ns_kind::user) ) {
        if ( micron::syscall(SYS_setgroups, 0, 0) < 0 ) __impl::__child_fail(ep, stage::credentials, -error::permissions);
      }
      if ( posix::setgid(__gid) < 0 ) __impl::__child_fail(ep, stage::credentials, -error::permissions);
      if ( posix::setuid(__uid) < 0 ) __impl::__child_fail(ep, stage::credentials, -error::permissions);
      if ( posix::getuid() != __uid || posix::geteuid() != __uid || posix::getgid() != __gid || posix::getegid() != __gid )
        __impl::__child_fail(ep, stage::credentials, -error::permissions);
    }

    // capabilities
    if ( __drop_all_caps && has_caps_to_drop ) {
      r = micron::drop_all_caps();
      if ( r < 0 ) __impl::__child_fail(ep, stage::caps_post, r);
    }

    // signal mask
    {
      posix::sigset_t empty{};
      posix::sigemptyset(empty);
      if ( posix::sigprocmask(posix::sig_setmask, empty, nullptr) < 0 ) __impl::__child_fail(ep, stage::signal_mask, -error::invalid_arg);
    }

    // exec.
    // WARNING: the fault pipe is closed on execve by O_CLOEXEC
    if ( body != nullptr ) {
      if ( ep >= 0 ) (void)posix::close(ep);
      micron::sys_group_exit(body());
    }

    (void)posix::execve(path, argv, envp);
    __impl::__child_fail(ep, stage::exec, -error::no_entry);
  }

public:
  constexpr sandbox(void) noexcept = default;

  // %%%%%%%%%%%%%%%%%
  // namespaces
  sandbox &
  user(void) noexcept
  {
    return __ns_kinds |= ns::ns_kind::user, *this;
  }

  sandbox &
  mount_ns(void) noexcept
  {
    return __ns_kinds |= ns::ns_kind::mount, *this;
  }

  sandbox &
  pid_ns(void) noexcept
  {
    return __ns_kinds |= ns::ns_kind::pid, *this;
  }

  sandbox &
  net(void) noexcept
  {
    return __ns_kinds |= ns::ns_kind::net, *this;
  }

  sandbox &
  uts(void) noexcept
  {
    return __ns_kinds |= ns::ns_kind::uts, *this;
  }

  sandbox &
  ipc(void) noexcept
  {
    return __ns_kinds |= ns::ns_kind::ipc, *this;
  }

  sandbox &
  cgroup(void) noexcept
  {
    return __ns_kinds |= ns::ns_kind::cgroup, *this;
  }

  sandbox &
  namespaces(ns::ns_kind k) noexcept
  {
    return __ns_kinds |= k, *this;
  }

  // %%%%%%%%%%%%%%%%%
  // filesystem

  // pivot_root
  sandbox &
  root(const char *new_root, const char *put_old) noexcept
  {
    __new_root = new_root;
    __put_old = put_old;
    __use_chroot = false;
    return *this;
  }

  // WARNING: chroot leaves the old root reachable through any fd or cwd that outlived it
  sandbox &
  chroot_to(const char *new_root) noexcept
  {
    __new_root = new_root;
    __use_chroot = true;
    return *this;
  }

  sandbox &
  keep_propagation(void) noexcept
  {
    return __make_private = false, *this;
  }

  sandbox &
  bind(const char *src, const char *dst, bool read_only = false, bool recursive = true, bool harden = true) noexcept
  {
    if ( __mount_count >= max_mounts ) return __config_fail(stage::filesystem, -error::no_space), *this;
    const unsigned long fl = recursive ? (posix::ms_bind | posix::ms_rec) : posix::ms_bind;
    const u64 attrs = harden ? __impl::__bind_default_attrs : 0ull;
    __mounts[__mount_count++] = mount_spec{ src, dst, nullptr, nullptr, fl, read_only, attrs };
    return *this;
  }

  sandbox &
  tmpfs(const char *dst, const char *options = "size=64m") noexcept
  {
    if ( __mount_count >= max_mounts ) return __config_fail(stage::filesystem, -error::no_space), *this;
    __mounts[__mount_count++]
        = mount_spec{ "tmpfs", dst, "tmpfs", options, posix::ms_nosuid | posix::ms_nodev | posix::ms_strictatime, false, 0 };
    return *this;
  }

  sandbox &
  procfs(const char *dst) noexcept
  {
    if ( __mount_count >= max_mounts ) return __config_fail(stage::filesystem, -error::no_space), *this;
    __mounts[__mount_count++] = mount_spec{ "proc", dst, "proc", nullptr, posix::ms_nosuid | posix::ms_nodev | posix::ms_noexec, false, 0 };
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%
  // confinement
  sandbox &
  landlock(const char *path, landlock::access_fs access) noexcept
  {
    if ( __ll_count >= max_landlock_rules ) return __config_fail(stage::landlock, -error::no_space), *this;
    __ll_rules[__ll_count++] = ll_rule{ path, access };
    // WARNING: the default is everything handled, not the union of what was granted
    if ( !__ll_handled_explicit ) __ll_handled = landlock::supported_fs();
    return *this;
  }

  // Landlock's IPC scoping (ABI 6+): a scoped domain may not signal, or connect to an abstract unix
  // socket of, a process outside it
  //
  // WARNING: see CVE-2026-72183
  sandbox &
  landlock_scope(landlock::scope scoped) noexcept
  {
    __ll_scoped = scoped;
    return *this;
  }

  sandbox &
  landlock_net(landlock::access_net handled_net) noexcept
  {
    __ll_handled_net = handled_net;
    return *this;
  }

  sandbox &
  landlock_handled(landlock::access_fs handled) noexcept
  {
    __ll_handled = handled;
    __ll_handled_explicit = true;
    return *this;
  }

  sandbox &
  landlock_handled_all(void) noexcept
  {
    return landlock_handled(landlock::supported_fs());
  }

  template<usize N>
  sandbox &
  seccomp(const seccomp::filter_builder<N> &fb) noexcept
  {
    if ( !fb.valid() ) return __config_fail(stage::seccomp, -error::invalid_arg), *this;
    __filter = fb.insns;
    __filter_len = static_cast<u16>(fb.count);
    return *this;
  }

  template<usize N> sandbox &seccomp(const seccomp::filter_builder<N> &&) = delete;

  sandbox &
  no_new_privs(bool on = true) noexcept
  {
    return __want_nnp = on, *this;
  }

  sandbox &
  undumpable(bool on = true) noexcept
  {
    return __want_undumpable = on, *this;
  }

  sandbox &
  drop_capabilities(bool on = true) noexcept
  {
    return __drop_all_caps = on, *this;
  }

  sandbox &
  as_user(posix::uid_t uid, posix::gid_t gid) noexcept
  {
    __uid = uid;
    __gid = gid;
    __change_ids = true;
    return *this;
  }

  sandbox &
  rlimit(posix::rlim_t res, u64 soft, u64 hard) noexcept
  {
    if ( __rlimit_count >= 8 ) return __config_fail(stage::rlimits, -error::no_space), *this;
    __rlimits[__rlimit_count++] = rl_spec{ res, soft, hard };
    return *this;
  }

  sandbox &
  stdio(i32 in, i32 out, i32 err) noexcept
  {
    __stdin_fd = in;
    __stdout_fd = out;
    __stderr_fd = err;
    return *this;
  }

  // WARNING: DIRECTORY DESCRIPTORS ARE REFUSED; SEE CVE-2024-21626
  sandbox &
  keep_fd(i32 fd) noexcept
  {
    if ( __impl::__fd_is_dir(fd) ) return __config_fail(stage::descriptors, -error::is_a_dir), *this;
    return keep_dir_fd(fd);
  }

  sandbox &
  keep_dir_fd(i32 fd) noexcept
  {
    __close_others = true;
    if ( __keep_count >= 8 ) return __config_fail(stage::descriptors, -error::no_space), *this;
    __keep_fds[__keep_count++] = fd;
    return *this;
  }

  sandbox &
  close_extra_fds(bool on = true) noexcept
  {
    return __close_others = on, *this;
  }

  [[nodiscard]] ns::ns_kind
  kinds(void) const noexcept
  {
    return __ns_kinds;
  }

  [[nodiscard]] usize
  mount_count(void) const noexcept
  {
    return __mount_count;
  }

  [[nodiscard]] usize
  landlock_rule_count(void) const noexcept
  {
    return __ll_count;
  }

  [[nodiscard]] bool
  has_seccomp(void) const noexcept
  {
    return __filter != nullptr && __filter_len > 0;
  }

  [[nodiscard]] landlock::access_fs
  landlock_handled(void) const noexcept
  {
    return __ll_handled;
  }

  [[nodiscard]] landlock::scope
  landlock_scoped(void) const noexcept
  {
    return __ll_scoped;
  }

  [[nodiscard]] landlock::access_net
  landlock_handled_net(void) const noexcept
  {
    return __ll_handled_net;
  }

  // observable, so a caller -- and a test -- can ask what the builder recorded rather than inferring
  // it from behaviour
  [[nodiscard]] bool
  closes_extra_fds(void) const noexcept
  {
    return __close_others;
  }

  [[nodiscard]] bool
  drops_capabilities(void) const noexcept
  {
    return __drop_all_caps;
  }

  // what the builder could not record. Non-empty means spawn()/run() will refuse before forking
  [[nodiscard]] fault_t
  config_fault(void) const noexcept
  {
    return __cfg_fault;
  }

  [[nodiscard]] bool
  configured(void) const noexcept
  {
    return !__cfg_fault;
  }

  // %%%%%%%%%%%%%%%%%%%%
  // launching

  struct child {
    posix::pid_t pid = -1;
    fault_t fault{};

    [[nodiscard]] constexpr bool
    ok(void) const noexcept
    {
      return pid > 0 && !fault;
    }
  };

  // how a child ended
  struct exit_status {
    int raw_status = 0;

    [[nodiscard]] constexpr bool
    exited(void) const noexcept
    {
      return micron::wifexited(raw_status);
    }

    [[nodiscard]] constexpr bool
    signaled(void) const noexcept
    {
      return micron::wifsignaled(raw_status);
    }

    // meaningful only when exited()
    [[nodiscard]] constexpr i32
    code(void) const noexcept
    {
      return micron::wexitstatus(raw_status);
    }

    // meaningful only when signaled()
    [[nodiscard]] constexpr i32
    signal(void) const noexcept
    {
      return micron::wtermsig(raw_status);
    }

    [[nodiscard]] constexpr int
    raw(void) const noexcept
    {
      return raw_status;
    }
  };

  // fork, apply every stage, then execve
  [[nodiscard]] child
  spawn(const char *path, char *const *argv, char *const *envp) const noexcept
  {
    return __launch(path, argv, envp, nullptr);
  }

  [[nodiscard]] child
  run(i32 (*body)(void)) const noexcept
  {
    return __launch(nullptr, nullptr, nullptr, body);
  }

  [[nodiscard]] result<exit_status>
  run_to_completion(i32 (*body)(void)) const noexcept
  {
    child c = run(body);
    if ( !c.ok() ) return result<exit_status>{ error_t(c.fault.err ? c.fault.err : -error::permissions) };
    int status = 0;
    if ( micron::waitpid(c.pid, &status, 0) < 0 ) [[unlikely]]
      return result<exit_status>{ error_t(-error::no_child_process) };      // nothing was reaped; status is not an answer
    return result<exit_status>{ exit_status{ status } };
  }

private:
  [[nodiscard]] child
  __launch(const char *path, char *const *argv, char *const *envp, i32 (*body)(void)) const noexcept
  {
    child out{};

    // must refuse before the fork
    if ( __cfg_fault ) {
      out.fault = __cfg_fault;
      return out;
    }

    i32 pfd[2] = { -1, -1 };
    if ( posix::pipe2(pfd, posix::o_cloexec) < 0 ) {
      out.fault = fault_t{ stage::fork_failed, -error::too_many_files };
      return out;
    }

    const int pid = micron::try_fork();
    if ( pid < 0 ) {
      (void)posix::close(pfd[0]);
      (void)posix::close(pfd[1]);
      out.fault = fault_t{ stage::fork_failed, pid };
      return out;
    }

    if ( pid == 0 ) {
      (void)posix::close(pfd[0]);
      __child(pfd[1], path, argv, envp, body);
    }

    (void)posix::close(pfd[1]);

    __impl::wire_fault w{ 0, 0 };
    usize got = 0;
    max_t n = 0;
    for ( ;; ) {
      n = posix::read(pfd[0], reinterpret_cast<char *>(&w) + got, sizeof(w) - got);
      if ( n > 0 ) {
        got += static_cast<usize>(n);
        if ( got == sizeof(w) ) break;
        continue;
      }
      if ( n == -error::interrupted ) continue;
      break;      // a clean eof, or a read error
    }
    (void)posix::close(pfd[0]);

    out.pid = pid;
    if ( got == sizeof(w) )
      out.fault = fault_t{ static_cast<stage>(w.where), w.err };
    else if ( got != 0 || n < 0 )
      out.fault = fault_t{ stage::fork_failed, got != 0 ? -error::io_error : static_cast<i32>(n) };

    if ( out.fault ) {
      // the child either failed a stage and exited, or is in a state we cannot vouch for
      if ( got != sizeof(w) ) (void)posix::kill(pid, posix::sig_kill);
      int status = 0;
      (void)micron::waitpid(pid, &status, 0);
      out.pid = -1;
    }
    return out;
  }
};

};      // namespace sec
};      // namespace micron
