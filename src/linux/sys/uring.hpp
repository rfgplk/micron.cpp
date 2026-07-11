//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

#include "../../atomic/intrin.hpp"
#include "../../memory/mman.hpp"
#include "../../syscall.hpp"
#include "time.hpp"

// NOTE: partially ported over from libjkr, canonical implementation lives there, stripped of the reactor + associated code, just enough so
// we can get fibers working without pulling in the rest (see plumbing/linux/net/uring.hpp && plumbing/conn/uring.hpp)

// TODO: potentially port libjkr over to this impl for cleanliness

namespace micron
{
namespace uring
{

// opcodes (IORING_OP_*)
inline constexpr u8 op_nop = 0;
inline constexpr u8 op_readv = 1;
inline constexpr u8 op_writev = 2;
inline constexpr u8 op_fsync = 3;
inline constexpr u8 op_poll_add = 6;
inline constexpr u8 op_poll_remove = 7;
inline constexpr u8 op_timeout = 11;
inline constexpr u8 op_timeout_remove = 12;
inline constexpr u8 op_accept = 13;
inline constexpr u8 op_async_cancel = 14;
inline constexpr u8 op_link_timeout = 15;
inline constexpr u8 op_connect = 16;
inline constexpr u8 op_openat = 18;
inline constexpr u8 op_close = 19;
inline constexpr u8 op_read = 22;       // >=5.6
inline constexpr u8 op_write = 23;      // >=5.6
inline constexpr u8 op_send = 26;
inline constexpr u8 op_recv = 27;
inline constexpr u8 op_shutdown = 34;
inline constexpr u8 op_msg_ring = 40;         // >=5.18
inline constexpr u8 op_futex_wait = 51;       // >=6.7
inline constexpr u8 op_futex_wake = 52;       // >=6.7
inline constexpr u8 op_futex_waitv = 53;      // >=6.7

// io_uring_setup flags (IORING_SETUP_*)
inline constexpr u32 setup_iopoll = 1u << 0;
inline constexpr u32 setup_sqpoll = 1u << 1;
inline constexpr u32 setup_cqsize = 1u << 3;
inline constexpr u32 setup_clamp = 1u << 4;
inline constexpr u32 setup_submit_all = 1u << 7;
inline constexpr u32 setup_coop_taskrun = 1u << 8;        // >=5.19
inline constexpr u32 setup_single_issuer = 1u << 12;      // >=6.0
inline constexpr u32 setup_defer_taskrun = 1u << 13;      // >=6.1

// io_uring_enter flags (IORING_ENTER_*)
inline constexpr u32 enter_getevents = 1u << 0;
inline constexpr u32 enter_sq_wakeup = 1u << 1;
inline constexpr u32 enter_sq_wait = 1u << 2;
inline constexpr u32 enter_ext_arg = 1u << 3;
inline constexpr u32 enter_registered_ring = 1u << 4;

// feature bits reported in params.features (IORING_FEAT_*)
inline constexpr u32 feat_single_mmap = 1u << 0;
inline constexpr u32 feat_nodrop = 1u << 1;
inline constexpr u32 feat_submit_stable = 1u << 2;
inline constexpr u32 feat_fast_poll = 1u << 5;
inline constexpr u32 feat_ext_arg = 1u << 8;
inline constexpr u32 feat_cqe_skip = 1u << 11;

// per-sqe flags (IOSQE_*)
inline constexpr u8 sqe_fixed_file = 1u << 0;
inline constexpr u8 sqe_io_drain = 1u << 1;
inline constexpr u8 sqe_io_link = 1u << 2;
inline constexpr u8 sqe_io_hardlink = 1u << 3;
inline constexpr u8 sqe_async = 1u << 4;
inline constexpr u8 sqe_buffer_select = 1u << 5;
inline constexpr u8 sqe_cqe_skip_success = 1u << 6;      // needs feat_cqe_skip (>=5.17)

// mmap offsets (IORING_OFF_*)
inline constexpr u64 off_sq_ring = 0ull;
inline constexpr u64 off_cq_ring = 0x8000000ull;
inline constexpr u64 off_sqes = 0x10000000ull;

// op-specific flags
inline constexpr u32 timeout_abs = 1u << 0;      // op_timeout: deadline is absolute (CLOCK_MONOTONIC)
inline constexpr u64 msg_data = 0ull;            // op_msg_ring cmd: pass user_data/res to the target CQ

// futex2 flags for the futex ops (sqe.fd carries them; FUTEX2_*)
inline constexpr u32 futex2_size_u32 = 0x02;
inline constexpr u32 futex2_private = 128;
inline constexpr u64 futex_match_any = 0xffffffffull;      // FUTEX_BITSET_MATCH_ANY

struct ktimespec {
  i64 tv_sec;
  i64 tv_nsec;
};

static_assert(sizeof(ktimespec) == 16, "__kernel_timespec ABI");

struct sq_offsets {
  u32 head;
  u32 tail;
  u32 ring_mask;
  u32 ring_entries;
  u32 flags;
  u32 dropped;
  u32 array;
  u32 resv1;
  u64 user_addr;
};

struct cq_offsets {
  u32 head;
  u32 tail;
  u32 ring_mask;
  u32 ring_entries;
  u32 overflow;
  u32 cqes;
  u32 flags;
  u32 resv1;
  u64 user_addr;
};

struct params {
  u32 sq_entries;
  u32 cq_entries;
  u32 flags;
  u32 sq_thread_cpu;
  u32 sq_thread_idle;
  u32 features;
  u32 wq_fd;
  u32 resv[3];
  sq_offsets sq_off;
  cq_offsets cq_off;
};

static_assert(sizeof(sq_offsets) == 40, "io_sqring_offsets ABI");
static_assert(sizeof(cq_offsets) == 40, "io_cqring_offsets ABI");
static_assert(sizeof(params) == 120, "io_uring_params ABI");
static_assert(__builtin_offsetof(params, features) == 20, "io_uring_params.features ABI");
static_assert(__builtin_offsetof(params, sq_off) == 40, "io_uring_params.sq_off ABI");
static_assert(__builtin_offsetof(params, cq_off) == 80, "io_uring_params.cq_off ABI");

struct sqe {
  u8 opcode;
  u8 flags;      // sqe_* bits
  u16 ioprio;
  i32 fd;
  u64 off;
  u64 addr;
  u32 len;
  u32 rw_flags;
  u64 user_data;
  u16 buf_index;
  u16 personality;
  u32 file_index;
  u64 addr3;
  u64 __pad2;
};

struct cqe {
  u64 user_data;
  i32 res;      // result or -errno
  u32 flags;
};

static_assert(sizeof(sqe) == 64, "io_uring_sqe ABI");
static_assert(__builtin_offsetof(sqe, fd) == 4 && __builtin_offsetof(sqe, off) == 8 && __builtin_offsetof(sqe, addr) == 16
                  && __builtin_offsetof(sqe, len) == 24 && __builtin_offsetof(sqe, rw_flags) == 28
                  && __builtin_offsetof(sqe, user_data) == 32 && __builtin_offsetof(sqe, buf_index) == 40
                  && __builtin_offsetof(sqe, personality) == 42 && __builtin_offsetof(sqe, file_index) == 44
                  && __builtin_offsetof(sqe, addr3) == 48,
              "io_uring_sqe ABI");
static_assert(sizeof(cqe) == 16 && __builtin_offsetof(cqe, res) == 8 && __builtin_offsetof(cqe, flags) == 12, "io_uring_cqe ABI");

inline long
__io_uring_setup(u32 entries, params *p) noexcept
{
  return micron::syscall(SYS_io_uring_setup, entries, p);
}

inline long
__io_uring_enter(i32 fd, u32 to_submit, u32 min_complete, u32 flags, void *argp, usize argsz) noexcept
{
  return micron::syscall(SYS_io_uring_enter, fd, to_submit, min_complete, flags, argp, argsz);
}

inline long
__io_uring_register(i32 fd, u32 opcode, void *arg, u32 nr_args) noexcept
{
  return micron::syscall(SYS_io_uring_register, fd, opcode, arg, nr_args);
}

[[gnu::always_inline]] inline void
__prep(sqe *s, u8 op, i32 fd, u64 addr, u32 len, u64 off) noexcept
{
  *s = sqe{};
  s->opcode = op;
  s->fd = fd;
  s->addr = addr;
  s->len = len;
  s->off = off;
}

[[gnu::always_inline]] inline void
prep_nop(sqe *s) noexcept
{
  __prep(s, op_nop, -1, 0, 0, 0);
}

[[gnu::always_inline]] inline void
prep_read(sqe *s, i32 fd, void *buf, u32 n, u64 off = static_cast<u64>(-1)) noexcept
{
  __prep(s, op_read, fd, reinterpret_cast<u64>(buf), n, off);      // off==-1: use the file position
}

[[gnu::always_inline]] inline void
prep_write(sqe *s, i32 fd, const void *buf, u32 n, u64 off = static_cast<u64>(-1)) noexcept
{
  __prep(s, op_write, fd, reinterpret_cast<u64>(buf), n, off);
}

[[gnu::always_inline]] inline void
prep_fsync(sqe *s, i32 fd, u32 fsync_flags = 0) noexcept
{
  __prep(s, op_fsync, fd, 0, 0, 0);
  s->rw_flags = fsync_flags;
}

[[gnu::always_inline]] inline void
prep_openat(sqe *s, i32 dirfd, const char *path, u32 open_flags, u32 mode) noexcept
{
  __prep(s, op_openat, dirfd, reinterpret_cast<u64>(path), mode, 0);
  s->rw_flags = open_flags;
}

[[gnu::always_inline]] inline void
prep_close(sqe *s, i32 fd) noexcept
{
  __prep(s, op_close, fd, 0, 0, 0);
}

[[gnu::always_inline]] inline void
prep_accept(sqe *s, i32 fd, void *addr, u32 *addrlen, u32 accept_flags = 0) noexcept
{
  __prep(s, op_accept, fd, reinterpret_cast<u64>(addr), 0, reinterpret_cast<u64>(addrlen));
  s->rw_flags = accept_flags;
}

[[gnu::always_inline]] inline void
prep_connect(sqe *s, i32 fd, const void *addr, u32 addrlen) noexcept
{
  __prep(s, op_connect, fd, reinterpret_cast<u64>(addr), 0, addrlen);
}

[[gnu::always_inline]] inline void
prep_send(sqe *s, i32 fd, const void *buf, u32 n, u32 msg_flags = 0) noexcept
{
  __prep(s, op_send, fd, reinterpret_cast<u64>(buf), n, 0);
  s->rw_flags = msg_flags;
}

[[gnu::always_inline]] inline void
prep_recv(sqe *s, i32 fd, void *buf, u32 n, u32 msg_flags = 0) noexcept
{
  __prep(s, op_recv, fd, reinterpret_cast<u64>(buf), n, 0);
  s->rw_flags = msg_flags;
}

[[gnu::always_inline]] inline void
prep_shutdown(sqe *s, i32 fd, i32 how) noexcept
{
  __prep(s, op_shutdown, fd, 0, static_cast<u32>(how), 0);
}

[[gnu::always_inline]] inline void
prep_poll_add(sqe *s, i32 fd, u32 poll_mask) noexcept
{
  __prep(s, op_poll_add, fd, 0, 0, 0);
  s->rw_flags = poll_mask;      // poll32_events (little-endian layout on all micron targets)
}

// ts must stay alive until the cqe lands
[[gnu::always_inline]] inline void
prep_timeout(sqe *s, const ktimespec *ts, u32 timeout_flags = 0) noexcept
{
  __prep(s, op_timeout, -1, reinterpret_cast<u64>(ts), 1, 0);
  s->rw_flags = timeout_flags;      // timeout_abs -> ts is a CLOCK_MONOTONIC deadline
}

[[gnu::always_inline]] inline void
prep_timeout_remove(sqe *s, u64 target_user_data) noexcept
{
  __prep(s, op_timeout_remove, -1, target_user_data, 0, 0);
}

[[gnu::always_inline]] inline void
prep_cancel(sqe *s, u64 target_user_data) noexcept
{
  __prep(s, op_async_cancel, -1, target_user_data, 0, 0);
}

[[gnu::always_inline]] inline void
prep_msg_ring(sqe *s, i32 target_ring_fd, u32 res, u64 data) noexcept
{
  __prep(s, op_msg_ring, target_ring_fd, msg_data, res, data);
}

[[gnu::always_inline]] inline void
prep_futex_wait(sqe *s, u32 *word, u32 expected, u64 mask = futex_match_any) noexcept
{
  __prep(s, op_futex_wait, static_cast<i32>(futex2_size_u32 | futex2_private), reinterpret_cast<u64>(word), 0, expected);
  s->addr3 = mask;
}

[[gnu::always_inline]] inline void
prep_futex_wake(sqe *s, u32 *word, u32 nwake, u64 mask = futex_match_any) noexcept
{
  __prep(s, op_futex_wake, static_cast<i32>(futex2_size_u32 | futex2_private), reinterpret_cast<u64>(word), 0, nwake);
  s->addr3 = mask;
}

struct ring {
  i32 fd = -1;
  u32 features = 0;
  u32 to_submit = 0;      // unsubmitted sqe count; accessed atomically (see advance_sq/enter) - a parked sentinel may enter() lock-free
  u32 *sq_head = nullptr;
  u32 *sq_tail = nullptr;
  u32 *sq_mask = nullptr;
  u32 *sq_array = nullptr;
  u32 *cq_head = nullptr;
  u32 *cq_tail = nullptr;
  u32 *cq_mask = nullptr;
  sqe *sqes = nullptr;
  cqe *cqes = nullptr;
  byte *__sq_ptr = nullptr;
  byte *__cq_ptr = nullptr;
  usize __sq_len = 0, __cq_len = 0, __sqes_len = 0;
  u32 __sq_entries = 0;

  ring() noexcept = default;
  ring(const ring &) = delete;
  ring &operator=(const ring &) = delete;
  ring(ring &&) = delete;
  ring &operator=(ring &&) = delete;

  ~ring() { shutdown(); }

  [[gnu::always_inline]] static bool
  __map_failed(const void *p) noexcept
  {
    return reinterpret_cast<usize>(p) >= static_cast<usize>(-4095);      // raw mmap returns -errno
  }

  int
  init(u32 entries, u32 setup_flags = 0) noexcept
  {
    params p{};
    p.flags = setup_flags;
    long r = __io_uring_setup(entries, &p);
    if ( r < 0 ) return static_cast<int>(r);
    fd = static_cast<i32>(r);
    features = p.features;
    __sq_entries = p.sq_entries;

    __sq_len = p.sq_off.array + p.sq_entries * sizeof(u32);
    __cq_len = p.cq_off.cqes + p.cq_entries * sizeof(cqe);
    if ( features & feat_single_mmap ) {
      if ( __cq_len > __sq_len ) __sq_len = __cq_len;
      __cq_len = __sq_len;
    }
    __sq_ptr = reinterpret_cast<byte *>(
        micron::mmap(nullptr, __sq_len, prot_read | prot_write, map_shared | map_populate, fd, static_cast<posix::off_t>(off_sq_ring)));
    if ( __map_failed(__sq_ptr) ) {
      int __e = static_cast<int>(reinterpret_cast<i64>(__sq_ptr));
      __close_fd();
      return __e;
    }
    if ( features & feat_single_mmap ) {
      __cq_ptr = __sq_ptr;
    } else {
      __cq_ptr = reinterpret_cast<byte *>(
          micron::mmap(nullptr, __cq_len, prot_read | prot_write, map_shared | map_populate, fd, static_cast<posix::off_t>(off_cq_ring)));
      if ( __map_failed(__cq_ptr) ) {
        int __e = static_cast<int>(reinterpret_cast<i64>(__cq_ptr));
        shutdown();
        return __e;
      }
    }
    __sqes_len = p.sq_entries * sizeof(sqe);
    sqes = reinterpret_cast<sqe *>(
        micron::mmap(nullptr, __sqes_len, prot_read | prot_write, map_shared | map_populate, fd, static_cast<posix::off_t>(off_sqes)));
    if ( __map_failed(sqes) ) {
      int __e = static_cast<int>(reinterpret_cast<i64>(sqes));
      sqes = nullptr;
      shutdown();
      return __e;
    }

    sq_head = reinterpret_cast<u32 *>(__sq_ptr + p.sq_off.head);
    sq_tail = reinterpret_cast<u32 *>(__sq_ptr + p.sq_off.tail);
    sq_mask = reinterpret_cast<u32 *>(__sq_ptr + p.sq_off.ring_mask);
    sq_array = reinterpret_cast<u32 *>(__sq_ptr + p.sq_off.array);
    cq_head = reinterpret_cast<u32 *>(__cq_ptr + p.cq_off.head);
    cq_tail = reinterpret_cast<u32 *>(__cq_ptr + p.cq_off.tail);
    cq_mask = reinterpret_cast<u32 *>(__cq_ptr + p.cq_off.ring_mask);
    cqes = reinterpret_cast<cqe *>(__cq_ptr + p.cq_off.cqes);
    return 0;
  }

  [[nodiscard]] bool
  live() const noexcept
  {
    return fd >= 0;
  }

  void
  shutdown() noexcept
  {
    if ( sqes != nullptr ) micron::munmap(reinterpret_cast<addr_t *>(sqes), __sqes_len);
    if ( __cq_ptr != nullptr && __cq_ptr != __sq_ptr ) micron::munmap(reinterpret_cast<addr_t *>(__cq_ptr), __cq_len);
    if ( __sq_ptr != nullptr ) micron::munmap(reinterpret_cast<addr_t *>(__sq_ptr), __sq_len);
    sqes = nullptr;
    __sq_ptr = nullptr;
    __cq_ptr = nullptr;
    __close_fd();
  }

  [[nodiscard]] sqe *
  get_sqe() noexcept
  {
    const u32 __t = *sq_tail;      // single preparer: plain read of our own tail
    if ( __t - atom::load(sq_head, __ATOMIC_ACQUIRE) >= __sq_entries ) return nullptr;
    const u32 __i = __t & *sq_mask;
    sq_array[__i] = __i;
    return &sqes[__i];
  }

  [[gnu::always_inline]] void
  advance_sq() noexcept
  {
    atom::store(sq_tail, *sq_tail + 1, __ATOMIC_RELEASE);
    atom::fetch_add(&to_submit, 1u, __ATOMIC_ACQ_REL);
  }

  long
  enter(u32 wait_nr = 0) noexcept
  {
    const u32 __n = atom::exchange(&to_submit, 0u, __ATOMIC_ACQ_REL);
    long r = __io_uring_enter(fd, __n, wait_nr, wait_nr != 0 ? enter_getevents : 0u, nullptr, 0);
    if ( r >= 0 ) {
      if ( static_cast<u32>(r) < __n ) atom::fetch_add(&to_submit, __n - static_cast<u32>(r), __ATOMIC_ACQ_REL);
      return r;
    }
    atom::fetch_add(&to_submit, __n, __ATOMIC_ACQ_REL);
    if ( r == -4 /*EINTR*/ && wait_nr != 0 ) return 0;
    return r;
  }

  [[nodiscard]] bool
  peek_cqe(cqe *out) noexcept
  {
    const u32 __h = *cq_head;
    if ( __h == atom::load(cq_tail, __ATOMIC_ACQUIRE) ) return false;
    *out = cqes[__h & *cq_mask];
    atom::store(cq_head, __h + 1, __ATOMIC_RELEASE);
    return true;
  }

  int
  wait_cqe(cqe *out) noexcept
  {
    for ( ;; ) {
      if ( peek_cqe(out) ) return 0;
      long r = enter(1);
      if ( r < 0 ) return static_cast<int>(r);
    }
  }

private:
  void
  __close_fd() noexcept
  {
    if ( fd >= 0 ) micron::syscall(SYS_close, fd);
    fd = -1;
  }
};

};      // namespace uring
};      // namespace micron
