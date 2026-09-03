//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../src/std.hpp"

#include "../../src/exit.hpp"
#include "../../src/kernel.hpp"
#include "../../src/linux/io/sys.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/process/wait.hpp"
#include "../../src/linux/sys/fcntl.hpp"
#include "../../src/linux/sys/landlock.hpp"
#include "../../src/linux/sys/mount.hpp"
#include "../../src/linux/sys/signal.hpp"

#include "sec_oracle.hpp"

namespace adv
{

namespace mc = micron;

constexpr i32 ok_code = 41;
constexpr i32 bad_code = 42;
constexpr i32 setup_failed = 43;
constexpr i32 unsupported = 44;

enum class grade : i32 {
  ok = 0,
  bad,
  setup,
  unsupported,
  signalled,
  other,
};

struct child_result {
  grade g = grade::other;
  i32 code = -1;
  i32 sig = 0;
  i32 raw = 0;

  [[nodiscard]] constexpr bool
  ok() const noexcept
  {
    return g == grade::ok;
  }

  [[nodiscard]] constexpr bool
  skipped() const noexcept
  {
    return g == grade::unsupported;
  }
};

[[nodiscard]] inline const char *
name_of(grade g) noexcept
{
  switch ( g ) {
  case grade::ok:
    return "ok";
  case grade::bad:
    return "ATTACK SUCCEEDED";
  case grade::setup:
    return "fixture failed";
  case grade::unsupported:
    return "unsupported on this kernel";
  case grade::signalled:
    return "killed by a signal";
  case grade::other:
    return "unrecognised exit";
  }
  return "?";
}

[[nodiscard]] inline child_result
run_child(i32 (*fn)(void)) noexcept
{
  const int pid = mc::try_fork();
  if ( pid < 0 ) return child_result{ grade::setup, -1, 0, 0 };
  if ( pid == 0 ) mc::sys_group_exit(fn());

  int status = 0;
  if ( mc::waitpid(pid, &status, 0) < 0 ) return child_result{ grade::setup, -1, 0, 0 };

  child_result r{};
  r.raw = status;
  if ( mc::wifsignaled(status) ) {
    r.g = grade::signalled;
    r.sig = mc::wtermsig(status);
    return r;
  }
  r.code = mc::wexitstatus(status);
  switch ( r.code ) {
  case ok_code:
    r.g = grade::ok;
    break;
  case bad_code:
    r.g = grade::bad;
    break;
  case setup_failed:
    r.g = grade::setup;
    break;
  case unsupported:
    r.g = grade::unsupported;
    break;
  default:
    r.g = grade::other;
    break;
  }
  return r;
}

[[nodiscard]] inline bool
have_userns(void) noexcept
{
  static i32 cached = -2;
  if ( cached != -2 ) return cached == 1;
  const child_result r = run_child([]() -> i32 { return mc::posix::unshare(mc::posix::clone_newuser) < 0 ? bad_code : ok_code; });
  cached = r.ok() ? 1 : 0;
  return cached == 1;
}

[[nodiscard]] inline i32
landlock_abi(void) noexcept
{
  static i32 cached = 0;
  if ( cached != 0 ) return cached;
  const i32 v = mc::posix::landlock_abi_version();
  cached = v > 0 ? v : -1;
  return cached;
}

[[nodiscard]] inline bool
have_landlock(i32 min_abi = 1) noexcept
{
  const i32 a = landlock_abi();
  return a > 0 && a >= min_abi;
}

[[nodiscard]] inline bool
have_selinux(void) noexcept
{
  const i32 fd = static_cast<i32>(mc::posix::open("/sys/fs/selinux/enforce", mc::posix::o_rdonly | mc::posix::o_cloexec, 0));
  if ( fd < 0 ) return false;
  (void)mc::posix::close(fd);
  return true;
}

[[nodiscard]] inline bool
legacy_tiocsti_enabled(void) noexcept
{
  const i32 fd = static_cast<i32>(mc::posix::open("/proc/sys/dev/tty/legacy_tiocsti", mc::posix::o_rdonly | mc::posix::o_cloexec, 0));
  if ( fd < 0 ) return true;
  char b[8]{};
  const auto n = mc::posix::read(fd, b, sizeof(b) - 1);
  (void)mc::posix::close(fd);
  return n > 0 && b[0] != '0';
}

[[nodiscard]] inline bool
have_kernel(u32 v) noexcept
{
  return mc::kernel::since(v);
}

[[nodiscard]] inline usize
open_fds(u32 ceiling = 4096) noexcept
{
  usize n = 0;
  for ( u32 i = 0; i < ceiling; ++i )
    if ( mc::posix::fcntl(static_cast<i32>(i), mc::posix::f_getfd) >= 0 ) ++n;
  return n;
}

[[nodiscard]] inline bool
fd_is_dir(i32 fd) noexcept
{
  mc::posix::stat_t st{};
  if ( mc::posix::fstat(fd, st) < 0 ) return false;
  return (st.st_mode & 0170000u) == 0040000u;
}

namespace so = sec_oracle;

[[nodiscard]] inline u32
filter_action(const micron::bpf::insn_t *prog, usize n, u32 arch, i32 nr, u64 a0 = 0, u64 a1 = 0, u64 a2 = 0, u64 a3 = 0, u64 a4 = 0,
              u64 a5 = 0) noexcept
{
  so::probe p{};
  p.nr = nr;
  p.arch = arch;
  p.args[0] = a0;
  p.args[1] = a1;
  p.args[2] = a2;
  p.args[3] = a3;
  p.args[4] = a4;
  p.args[5] = a5;
  return so::run(prog, n, p).action;
}

template<usize N>
[[nodiscard]] inline u32
filter_action(micron::sec::seccomp::filter_builder<N> &fb, i32 nr, u64 a0 = 0, u64 a1 = 0, u64 a2 = 0, u64 a3 = 0, u64 a4 = 0,
              u64 a5 = 0) noexcept
{
  auto p = fb.prog();
  return filter_action(p.filter, p.len, static_cast<u32>(micron::sec::seccomp::native_arch), nr, a0, a1, a2, a3, a4, a5);
}

template<typename G>
[[nodiscard]] constexpr bool
group_names(i32 nr) noexcept
{
  for ( usize i = 0; i < G::count; ++i )
    if ( G::calls[i] == nr ) return true;
  return false;
}

constexpr const char *scratch_root = "/var/tmp";

inline void
scratch_path(char *out, usize cap, const char *tag, const char *leaf = nullptr) noexcept
{
  usize i = 0;
  const auto put = [&](const char *s) {
    while ( *s != '\0' && i + 1 < cap ) out[i++] = *s++;
  };
  put(scratch_root);
  put("/");
  put(tag);
  if ( leaf != nullptr ) {
    put("/");
    put(leaf);
  }
  if ( i < cap )
    out[i] = '\0';
  else if ( cap > 0 )
    out[cap - 1] = '\0';
}

[[nodiscard]] inline bool
path_exists(const char *p) noexcept
{
  mc::posix::stat_t st{};
  return mc::posix::stat(p, st) >= 0;
}

[[nodiscard]] inline bool
same_object(const char *a, const char *b) noexcept
{
  mc::posix::stat_t sa{};
  mc::posix::stat_t sb{};
  if ( mc::posix::stat(a, sa) < 0 ) return false;
  if ( mc::posix::stat(b, sb) < 0 ) return false;
  return sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
}

};      // namespace adv
