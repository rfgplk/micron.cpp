//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "atomic/intrin.hpp"
#include "syscall.hpp"
#include "types.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// runtime linux kernel version detection & dispatch
// our kernel baseline is 5.0 (5.8 effectively for freestanding threads)

namespace micron
{
namespace kernel
{

constexpr u32
ver(u32 maj, u32 min = 0, u32 patch = 0) noexcept
{
  if ( maj > 255 ) maj = 255;
  if ( min > 255 ) min = 255;
  if ( patch > 255 ) patch = 255;
  return (maj << 16) | (min << 8) | patch;
}

// NOTE: i thought about extending this fully for all linux kernel versions (and for legacy systems), but it practically doesn't make sense
// and looking through all archives is tedious

// minimum supported kernel; everything older is treated as exactly this
inline constexpr u32 baseline = ver(5, 0);

#if defined(MICRON_MIN_KERNEL_MAJOR)
#if !defined(MICRON_MIN_KERNEL_MINOR)
#define MICRON_MIN_KERNEL_MINOR 0
#endif
#if !defined(MICRON_MIN_KERNEL_PATCH)
#define MICRON_MIN_KERNEL_PATCH 0
#endif
inline constexpr u32 floor_version = ver(MICRON_MIN_KERNEL_MAJOR, MICRON_MIN_KERNEL_MINOR, MICRON_MIN_KERNEL_PATCH);
#else
inline constexpr u32 floor_version = baseline;
#endif
static_assert(floor_version >= baseline, "MICRON_MIN_KERNEL_* below the 5.0 baseline is meaningless");

namespace __impl
{

// NOTE: redeclared here on purpose so kernel.hpp doesn't pull in sys/system.hpp
struct __utsname {
  char sysname[65];
  char nodename[65];
  char release[65];
  char version[65];
  char machine[65];
  char domainname[65];
};

constexpr u32
__parse_release(const char *s) noexcept
{
  u32 part[3]{ 0, 0, 0 };
  usize i = 0;
  for ( usize p = 0; p < 3; p++ ) {
    if ( s[i] < '0' || s[i] > '9' ) break;
    u32 acc = 0;
    for ( ; s[i] >= '0' && s[i] <= '9'; i++ ) {
      acc = acc * 10 + static_cast<u32>(s[i] - '0');
      if ( acc > 255 ) acc = 255;
    }
    part[p] = acc;
    if ( s[i] != '.' ) break;
    i++;
  }
  if ( part[0] == 0 ) return 0;
  return ver(part[0], part[1], part[2]);
}

// pre-5.0 and parse failures
constexpr u32
__bucket(u32 v) noexcept
{
  return v < floor_version ? floor_version : v;
}

inline u32 __version_slot = 0;

[[gnu::cold]] [[gnu::noinline]] inline u32
__detect(void) noexcept
{
  __utsname u{};
  long r = micron::syscall(SYS_uname, &u);
  u32 v = __bucket(micron::syscall_failed(r) ? 0 : __parse_release(u.release));
  atom::store(&__version_slot, v, micron::atomic_relaxed);
  return v;
}
};      // namespace __impl

[[gnu::always_inline]] inline u32
version(void) noexcept
{
  u32 v = atom::load(&__impl::__version_slot, micron::atomic_relaxed);
  if ( v != 0 ) [[likely]]
    return v;
  return __impl::__detect();
}

[[gnu::always_inline]] inline bool
since(u32 v) noexcept
{
  if ( v <= floor_version ) return true;
  return version() >= v;
}

[[gnu::always_inline]] inline bool
since(u32 maj, u32 min, u32 patch = 0) noexcept
{
  return since(ver(maj, min, patch));
}

// version gated feature tags
namespace feature
{
inline constexpr u32 io_uring = ver(5, 1);
inline constexpr u32 pidfd_send_signal = ver(5, 1);
inline constexpr u32 clone3 = ver(5, 3);
inline constexpr u32 pidfd_open = ver(5, 3);
inline constexpr u32 copy_file_range_cross_fs = ver(5, 3);
inline constexpr u32 madv_cold_pageout = ver(5, 4);      // MADV_COLD / MADV_PAGEOUT
inline constexpr u32 openat2 = ver(5, 6);
inline constexpr u32 uring_op_read = ver(5, 6);         // IORING_OP_READ / IORING_OP_WRITE / OPENAT / CLOSE / STATX
inline constexpr u32 mremap_dontunmap = ver(5, 7);      // MREMAP_DONTUNMAP
inline constexpr u32 faccessat2 = ver(5, 8);
inline constexpr u32 close_range = ver(5, 9);
inline constexpr u32 keyctl_move = ver(5, 10);      // KEYCTL_MOVE (keyctl op 30)
inline constexpr u32 epoll_pwait2 = ver(5, 11);
inline constexpr u32 uring_renameat = ver(5, 11);      // IORING_OP_RENAMEAT / UNLINKAT
inline constexpr u32 memfd_secret = ver(5, 14);
inline constexpr u32 madv_populate = ver(5, 14);      // MADV_POPULATE_READ / MADV_POPULATE_WRITE
inline constexpr u32 uring_mkdirat = ver(5, 15);      // IORING_OP_MKDIRAT / SYMLINKAT / LINKAT; sqe file_index
inline constexpr u32 uring_cqe_skip = ver(5, 17);
inline constexpr u32 uring_msg_ring = ver(5, 18);
inline constexpr u32 uring_reg_ring_fd = ver(5, 18);         // IORING_REGISTER_RING_FDS
inline constexpr u32 madv_dontneed_locked = ver(5, 18);      // MADV_DONTNEED_LOCKED
inline constexpr u32 uring_coop_taskrun = ver(5, 19);
inline constexpr u32 uring_files_sparse = ver(5, 19);      // sparse fixed-file table + buf rings
inline constexpr u32 uring_single_issuer = ver(6, 0);
inline constexpr u32 uring_defer_taskrun = ver(6, 1);
inline constexpr u32 statx_dio_align = ver(6, 1);
inline constexpr u32 madv_collapse = ver(6, 1);      // MADV_COLLAPSE
inline constexpr u32 uring_futex = ver(6, 7);
inline constexpr u32 uring_ftruncate = ver(6, 9);          // IORING_OP_FTRUNCATE
inline constexpr u32 map_droppable = ver(6, 11);           // MAP_DROPPABLE
inline constexpr u32 uring_resize_rings = ver(6, 13);      // IORING_REGISTER_RESIZE_RINGS (defer_taskrun rings only)
inline constexpr u32 uring_sqe_mixed = ver(7, 1);          // IORING_SETUP_SQE_MIXED: 64B and 128B sqes on one ring
};      // namespace feature

[[gnu::always_inline]] inline bool
has(u32 feature_ver) noexcept
{
  return since(feature_ver);
}

// NOTE: a kernel can report >= X yet still refuse the syscall (seccomp/docker filters, qemu-user, CONFIG_ off, vendor kernels).
// WARNING: demote on ENOSYS only; EPERM can be a legitimate result and must never demote
struct probe_gate {
  u32 __demoted = 0;

  [[gnu::always_inline]] bool
  open(u32 feature_ver) noexcept
  {
    return since(feature_ver) && atom::load(&__demoted, micron::atomic_relaxed) == 0;
  }

  [[gnu::always_inline]] void
  demote(void) noexcept
  {
    atom::store(&__demoted, 1u, micron::atomic_relaxed);
  }

  void
  restore(void) noexcept
  {
    atom::store(&__demoted, 0u, micron::atomic_relaxed);
  }
};

inline void
__set_version_for_testing(u32 v) noexcept
{
  atom::store(&__impl::__version_slot, v, micron::atomic_relaxed);
}

};      // namespace kernel
};      // namespace micron
