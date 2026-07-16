//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../bits/__arch.hpp"
#include "../../types.hpp"
#include "../sys/types.hpp"

#include "../io/io_structs.hpp"

#include "../../memory/cmemory/memcmp.hpp"

#include "fcntl.hpp"
#include "stat_structs.hpp"

namespace micron
{
namespace posix
{
// stat dispatchx:
//   amd64     -> SYS_stat / SYS_lstat / SYS_fstat / SYS_newfstatat    -> 64-bit native stat_t
//   arm64     -> SYS_fstat / SYS_newfstatat (no stat/lstat in asm-gen)-> 64-bit stat_t
//   arm32/x86 -> statx (arch-uniform) translated into stat64_t
#if defined(__micron_arch_arm32) || defined(__micron_arch_x86)
constexpr dev_t
__statx_makedev(u32 maj, u32 mnr)
{
  return static_cast<dev_t>((static_cast<u64>(mnr) & 0xffu) | ((static_cast<u64>(maj) & 0xfffu) << 8)
                            | ((static_cast<u64>(mnr) & ~0xffull) << 12) | ((static_cast<u64>(maj) & ~0xfffull) << 32));
}

inline void
__statx_to_stat64(const statx_t &sx, stat64_t &st)
{
  st.st_dev = __statx_makedev(sx.stx_dev_major, sx.stx_dev_minor);
  st.__pad1 = 0;
  st.__st_ino = static_cast<ino_t>(sx.stx_ino);
  st.st_mode = static_cast<mode_t>(sx.stx_mode);
  st.st_nlink = static_cast<nlink_t>(sx.stx_nlink);
  st.st_uid = static_cast<uid_t>(sx.stx_uid);
  st.st_gid = static_cast<gid_t>(sx.stx_gid);
  st.st_rdev = __statx_makedev(sx.stx_rdev_major, sx.stx_rdev_minor);
  st.__pad2 = 0;
  st.st_size = static_cast<off64_t>(sx.stx_size);
  st.st_blksize = static_cast<blksize_t>(sx.stx_blksize);
  st.st_blocks = static_cast<__blkcnt64_t_type>(sx.stx_blocks);
  st.st_atim.tv_sec = sx.stx_atime.tv_sec;
  st.st_atim.tv_nsec = sx.stx_atime.tv_nsec;
  st.st_mtim.tv_sec = sx.stx_mtime.tv_sec;
  st.st_mtim.tv_nsec = sx.stx_mtime.tv_nsec;
  st.st_ctim.tv_sec = sx.stx_ctime.tv_sec;
  st.st_ctim.tv_nsec = sx.stx_ctime.tv_nsec;
  st.st_ino = static_cast<ino64_t>(sx.stx_ino);
}

inline i32
__stat64_via_statx(int dirfd, const char *path, int flags, stat64_t &buf)
{
  statx_t sx{};
  i32 r = static_cast<i32>(micron::syscall(SYS_statx, dirfd, path, flags, statx_basic_stats, &sx));
  if ( r == 0 ) __statx_to_stat64(sx, buf);
  return r;
}
#endif

#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
auto
fstatat(posix::fd_t dirfd, const char *__restrict name, stat_t &__restrict buf, int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_newfstatat, dirfd.fd, name, &buf, flags));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
fstatat(posix::fd_t dirfd, const char *__restrict name, stat64_t &__restrict buf, int flags) -> i32
{
  return __stat64_via_statx(dirfd.fd, name, flags, buf);
}
#endif

#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
auto
fstatat(int dirfd, const char *__restrict name, stat_t &__restrict buf, int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_newfstatat, dirfd, name, &buf, flags));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
fstatat(int dirfd, const char *__restrict name, stat64_t &__restrict buf, int flags) -> i32
{
  return __stat64_via_statx(dirfd, name, flags, buf);
}
#endif

#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
auto
fstat(posix::fd_t fd, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstat, fd.fd, &buf));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
fstat(posix::fd_t fd, stat64_t &buf) -> i32
{
  return __stat64_via_statx(fd.fd, "", at_empty_path, buf);
}
#endif

#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
auto
fstat(int fd, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstat, fd, &buf));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
fstat(int fd, stat64_t &buf) -> i32
{
  return __stat64_via_statx(fd, "", at_empty_path, buf);
}
#endif

#if defined(__micron_arch_amd64)
auto
lstat(const char *path, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_lstat, path, &buf));
}
#elif defined(__micron_arch_arm64)
auto
lstat(const char *path, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_newfstatat, at_fdcwd, path, &buf, at_symlink_nofollow));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
lstat(const char *path, stat64_t &buf) -> i32
{
  return __stat64_via_statx(at_fdcwd, path, at_symlink_nofollow, buf);
}
#endif

#if defined(__micron_arch_amd64)
auto
stat(const char *path, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_stat, path, &buf));
}
#elif defined(__micron_arch_arm64)
auto
stat(const char *path, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_newfstatat, at_fdcwd, path, &buf, 0));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
stat(const char *path, stat64_t &buf) -> i32
{
  return __stat64_via_statx(at_fdcwd, path, at_statx_sync_as_stat, buf);
}
#endif

auto
statx(int dirfd, const char *__restrict path, int flags, unsigned int req_mask, statx_t &__restrict buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_statx, dirfd, path, flags, req_mask, &buf));
}

auto
statx(posix::fd_t dirfd, const char *__restrict path, int flags, unsigned int req_mask, statx_t &__restrict buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_statx, dirfd.fd, path, flags, req_mask, &buf));
}
};      // namespace posix
};      // namespace micron
