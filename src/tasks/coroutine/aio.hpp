//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "cl_sched.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// awaitable io over the per-worker reactor rings
//
// NOTE: partial port of code from libjkr

namespace micron
{
namespace coro
{
namespace io
{

struct [[nodiscard]] __uring_awaitable {
  micron::uring::sqe __sqe;
  __io_op __op{};
  __io_cxl_node __cxl{};
  u8 __fixed = 0;

  explicit __uring_awaitable(const micron::uring::sqe &__q, u8 __fx = 0) noexcept : __sqe(__q), __fixed(__fx) { }

  ~__uring_awaitable()
  {
    if ( __cxl.__flag != nullptr ) __io_cxl_unlink(&__cxl);
  }

  bool
  await_ready() noexcept
  {
    if ( __io.any_live.get(micron::memory_order_acquire) != 0 ) return false;
    __op.__res = -38;      // ENOSYS: no ring on this system
    return true;
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __frame_base *__f = &__h.promise();
    __op.__f = __f;
    if ( reinterpret_cast<u64>(&__op) >> __io_ud_tag_shift != 0 ) [[unlikely]]
      __builtin_trap();      // non-canonical op address would alias a user_data tag
    __wring *__r = __io_own_ring();
    const bool __own = (__r != nullptr);
    if ( !__own ) {
      if ( __io_fb.__live.get(micron::memory_order_acquire) == 0 ) {
        __op.__res = -38;
        return false;
      }
      __r = &__io_fb;
    }
    if ( __fixed != 0 && !__io_fixed_reg(*__r) ) {
      __op.__res = -95;      // EOPNOTSUPP: slab registration refused (memlock/old kernel)
      return false;
    }
    const micron::atomic_token<u32> *__cxf = __f->__cancel;
    if ( __cxf != nullptr ) {
      if ( __cxf->get(micron::memory_order_acquire) != 0 ) {
        __op.__res = -125;      // ECANCELED before submission
        return false;
      }
      __cxl.__flag = __cxf;
      __cxl.__ud = reinterpret_cast<u64>(&__op);
      __cxl.__ring_fd = __r->__r.fd;
      __io_cxl_push(&__cxl);
    }
    __r->__pending.fetch_add(1, micron::memory_order_acq_rel);
    bool __ok = __own ? __io_submit_own(*__r, __sqe, reinterpret_cast<u64>(&__op)) : __io_submit_fb(__sqe, reinterpret_cast<u64>(&__op));
    if ( !__ok ) [[unlikely]] {
      // kernel refused our enter (CQ overflow backpressure)
      if ( __io_cq_acquire(*__r) ) {
        micron::uring::cqe __c{};
        while ( __r->__r.peek_cqe(&__c) ) __global_engine->__dispatch_cqe(*__r, __c);
        __io_unlock(__r->__cq_lk);
      }
      __ok = __own ? __io_submit_own(*__r, __sqe, reinterpret_cast<u64>(&__op)) : __io_submit_fb(__sqe, reinterpret_cast<u64>(&__op));
    }
    if ( !__ok ) {
      __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
      // ENOBUFS
      __op.__res = -105;
      return false;
    }
    if ( __cxf != nullptr && __cxf->get(micron::memory_order_acquire) != 0 )
      __io_sync_cancel_ud(__r->__r.fd, reinterpret_cast<u64>(&__op));      // cancel raced the submit
    if ( __io_cq_acquire(*__r) ) {
      micron::uring::cqe __c{};
      u32 __budget = 8;
      bool __mine = false;
      while ( __budget-- != 0 && __r->__r.peek_cqe(&__c) ) {
        if ( __io_ud_tag(__c.user_data) == __io_ud_op && reinterpret_cast<__io_op *>(__c.user_data) == &__op ) {
          __op.__res = __c.res;
          __mine = true;
          break;
        }
        __global_engine->__dispatch_cqe(*__r, __c);      // foreign cqe: normal completer path
      }
      __io_unlock(__r->__cq_lk);
      if ( __mine ) {
        __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
#if defined(MICRON_CORO_STATS)
        __r->__stat.inline_completions.fetch_add(1, micron::memory_order_relaxed);
#endif
        return false;
      }
    }
    // commit to suspension; on failure a completer already delivered __res
    u32 __exp = __io_st_submitted;
    if ( __op.__st.compare_exchange_strong(__exp, __io_st_suspended, micron::memory_order_acq_rel, micron::memory_order_acquire) )
      return true;      // LAST access to this awaitable/frame: the frame may already be running elsewhere
    __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
    return false;
  }

  i32
  await_resume() noexcept
  {
    if ( __cxl.__flag != nullptr ) {
      __io_cxl_unlink(&__cxl);
      __cxl.__flag = nullptr;
    }
    return __op.__res;
  }
};

struct timeout {
  u64 __ns;
};

[[nodiscard]] inline constexpr timeout
after(u64 __ns) noexcept
{
  return timeout{ __ns };
}

struct [[nodiscard]] __uring_timed_awaitable {
  micron::uring::sqe __sqe;
  micron::uring::ktimespec __kts;
  __io_op __op{};
  __io_cxl_node __cxl{};
  u8 __fixed = 0;      // carried over from the untimed awaitable; see operator|

  __uring_timed_awaitable(const micron::uring::sqe &__q, u64 __ns, u8 __fx = 0) noexcept
      : __sqe(__q), __kts{ static_cast<i64>(__ns / 1000000000ull), static_cast<i64>(__ns % 1000000000ull) }, __fixed(__fx)
  {
    __sqe.flags |= micron::uring::sqe_io_link;      // hard-pair with the link_timeout staged next
  }

  ~__uring_timed_awaitable()
  {
    if ( __cxl.__flag != nullptr ) __io_cxl_unlink(&__cxl);
  }

  bool
  await_ready() noexcept
  {
    if ( __io.any_live.get(micron::memory_order_acquire) != 0 ) return false;
    __op.__res = -38;
    return true;
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __frame_base *__f = &__h.promise();
    __op.__f = __f;
    if ( reinterpret_cast<u64>(&__op) >> __io_ud_tag_shift != 0 ) [[unlikely]]
      __builtin_trap();
    __wring *__r = __io_own_ring();
    const bool __own = (__r != nullptr);
    if ( !__own ) {
      if ( __io_fb.__live.get(micron::memory_order_acquire) == 0 ) {
        __op.__res = -38;
        return false;
      }
      __r = &__io_fb;
    }
    if ( __fixed != 0 && !__io_fixed_reg(*__r) ) {      // same lazy per-ring registration the untimed path does
      __op.__res = -95;                                 // EOPNOTSUPP: slab registration refused (memlock/old kernel)
      return false;
    }
    const micron::atomic_token<u32> *__cxf = __f->__cancel;
    if ( __cxf != nullptr ) {
      if ( __cxf->get(micron::memory_order_acquire) != 0 ) {
        __op.__res = -125;
        return false;
      }
      __cxl.__flag = __cxf;
      __cxl.__ud = reinterpret_cast<u64>(&__op);
      __cxl.__ring_fd = __r->__r.fd;
      __io_cxl_push(&__cxl);
    }
    micron::uring::sqe __lt;
    micron::uring::prep_link_timeout(&__lt, &__kts);      // kernel copies the timespec at prep
    __r->__pending.fetch_add(1, micron::memory_order_acq_rel);
    bool __ok = __own ? __io_submit_own2(*__r, __sqe, reinterpret_cast<u64>(&__op), __lt, __io_ud_make(__io_ud_ltimer, 0))
                      : __io_submit_fb2(__sqe, reinterpret_cast<u64>(&__op), __lt, __io_ud_make(__io_ud_ltimer, 0));
    if ( !__ok ) [[unlikely]] {
      // reap then retry
      if ( __io_cq_acquire(*__r) ) {
        micron::uring::cqe __c{};
        while ( __r->__r.peek_cqe(&__c) ) __global_engine->__dispatch_cqe(*__r, __c);
        __io_unlock(__r->__cq_lk);
      }
      __ok = __own ? __io_submit_own2(*__r, __sqe, reinterpret_cast<u64>(&__op), __lt, __io_ud_make(__io_ud_ltimer, 0))
                   : __io_submit_fb2(__sqe, reinterpret_cast<u64>(&__op), __lt, __io_ud_make(__io_ud_ltimer, 0));
    }
    if ( !__ok ) {
      __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
      __op.__res = -105;      // ENOBUFS
      return false;
    }
    if ( __cxf != nullptr && __cxf->get(micron::memory_order_acquire) != 0 ) __io_sync_cancel_ud(__r->__r.fd, reinterpret_cast<u64>(&__op));
    if ( __io_cq_acquire(*__r) ) {
      micron::uring::cqe __c{};
      u32 __budget = 8;
      bool __mine = false;
      while ( __budget-- != 0 && __r->__r.peek_cqe(&__c) ) {
        if ( __io_ud_tag(__c.user_data) == __io_ud_op && reinterpret_cast<__io_op *>(__c.user_data) == &__op ) {
          __op.__res = __c.res;
          __mine = true;
          break;
        }
        __global_engine->__dispatch_cqe(*__r, __c);
      }
      __io_unlock(__r->__cq_lk);
      if ( __mine ) {
        __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
#if defined(MICRON_CORO_STATS)
        __r->__stat.inline_completions.fetch_add(1, micron::memory_order_relaxed);
#endif
        return false;
      }
    }
    u32 __exp = __io_st_submitted;
    if ( __op.__st.compare_exchange_strong(__exp, __io_st_suspended, micron::memory_order_acq_rel, micron::memory_order_acquire) )
      return true;      // LAST access
    __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
    return false;
  }

  i32
  await_resume() noexcept
  {
    if ( __cxl.__flag != nullptr ) {
      __io_cxl_unlink(&__cxl);
      __cxl.__flag = nullptr;
    }
    return __op.__res;
  }
};

[[nodiscard]] inline __uring_timed_awaitable
operator|(const __uring_awaitable &__a, timeout __t) noexcept
{
  return __uring_timed_awaitable{ __a.__sqe, __t.__ns, __a.__fixed };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// data path

// positional reads/writes; off==-1 uses (and advances) the file position
[[nodiscard]] inline __uring_awaitable
read(i32 fd, void *buf, u32 n, u64 off = static_cast<u64>(-1)) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_read(&__q, fd, buf, n, off);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
write(i32 fd, const void *buf, u32 n, u64 off = static_cast<u64>(-1)) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_write(&__q, fd, buf, n, off);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
readv(i32 fd, const micron::uring::iovec *iov, u32 nr, u64 off = static_cast<u64>(-1), u32 rw_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_readv(&__q, fd, iov, nr, off, rw_flags);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
writev(i32 fd, const micron::uring::iovec *iov, u32 nr, u64 off = static_cast<u64>(-1), u32 rw_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_writev(&__q, fd, iov, nr, off, rw_flags);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
read_fixed(i32 fd, void *buf, u32 n, u64 off, u16 buf_index) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_read_fixed(&__q, fd, buf, n, off, buf_index);
  return __uring_awaitable{ __q, 1 };
}

[[nodiscard]] inline __uring_awaitable
write_fixed(i32 fd, const void *buf, u32 n, u64 off, u16 buf_index) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_write_fixed(&__q, fd, buf, n, off, buf_index);
  return __uring_awaitable{ __q, 1 };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// lifecycle + metadata

[[nodiscard]] inline __uring_awaitable
openat(i32 dirfd, const char *path, u32 open_flags, u32 mode = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_openat(&__q, dirfd, path, open_flags, mode);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
openat2(i32 dirfd, const char *path, const void *how, u32 how_len = 24) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_openat2(&__q, dirfd, path, how, how_len);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
close(i32 fd) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_close(&__q, fd);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
fsync(i32 fd, u32 fsync_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_fsync(&__q, fd, fsync_flags);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
sync_file_range(i32 fd, u32 nbytes, u64 off, u32 range_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_sync_file_range(&__q, fd, nbytes, off, range_flags);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
statx(i32 dirfd, const char *path, u32 statx_flags, u32 mask, void *statx_buf) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_statx(&__q, dirfd, path, statx_flags, mask, statx_buf);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
renameat(i32 olddirfd, const char *oldpath, i32 newdirfd, const char *newpath, u32 rename_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_renameat(&__q, olddirfd, oldpath, newdirfd, newpath, rename_flags);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
unlinkat(i32 dirfd, const char *path, u32 unlink_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_unlinkat(&__q, dirfd, path, unlink_flags);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
mkdirat(i32 dirfd, const char *path, u32 mode) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_mkdirat(&__q, dirfd, path, mode);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
symlinkat(const char *target, i32 newdirfd, const char *linkpath) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_symlinkat(&__q, target, newdirfd, linkpath);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
linkat(i32 olddirfd, const char *oldpath, i32 newdirfd, const char *newpath, u32 hardlink_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_linkat(&__q, olddirfd, oldpath, newdirfd, newpath, hardlink_flags);
  return __uring_awaitable{ __q };
}

// kernel >=6.9
[[nodiscard]] inline __uring_awaitable
ftruncate(i32 fd, u64 len) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_ftruncate(&__q, fd, len);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
fallocate(i32 fd, i32 mode, u64 off, u64 len) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_fallocate(&__q, fd, mode, off, len);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
fadvise(i32 fd, u64 off, u32 len, i32 advice) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_fadvise(&__q, fd, off, len, advice);
  return __uring_awaitable{ __q };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pipes / data movement

[[nodiscard]] inline __uring_awaitable
splice(i32 fd_in, u64 off_in, i32 fd_out, u64 off_out, u32 nbytes, u32 splice_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_splice(&__q, fd_in, off_in, fd_out, off_out, nbytes, splice_flags);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
tee(i32 fd_in, i32 fd_out, u32 nbytes, u32 splice_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_tee(&__q, fd_in, fd_out, nbytes, splice_flags);
  return __uring_awaitable{ __q };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sockets (libjkr substrate)

[[nodiscard]] inline __uring_awaitable
accept(i32 fd, void *addr = nullptr, u32 *addrlen = nullptr, u32 accept_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_accept(&__q, fd, addr, addrlen, accept_flags);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
connect(i32 fd, const void *addr, u32 addrlen) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_connect(&__q, fd, addr, addrlen);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
send(i32 fd, const void *buf, u32 n, u32 msg_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_send(&__q, fd, buf, n, msg_flags);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
recv(i32 fd, void *buf, u32 n, u32 msg_flags = 0) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_recv(&__q, fd, buf, n, msg_flags);
  return __uring_awaitable{ __q };
}

// resumes when any event in poll_mask is ready on fd (res = ready mask)
[[nodiscard]] inline __uring_awaitable
poll(i32 fd, u32 poll_mask) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_poll_add(&__q, fd, poll_mask);
  return __uring_awaitable{ __q };
}

[[nodiscard]] inline __uring_awaitable
shutdown(i32 fd, i32 how) noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_shutdown(&__q, fd, how);
  return __uring_awaitable{ __q };
}

// no-op through the full submit/complete path (bench + protocol tests)
[[nodiscard]] inline __uring_awaitable
nop() noexcept
{
  micron::uring::sqe __q;
  micron::uring::prep_nop(&__q);
  return __uring_awaitable{ __q };
}

// %%%%%%%%%%%%%%%%%%%%%%%
// multishot recv

struct mrecv_t;
inline void mrecv_cancel(mrecv_t &__mr) noexcept;

struct mrecv_t {
  __io_mop __op;

  mrecv_t() noexcept = default;
  // not allowed to be copied nor moved, controlled by the kernel uth
  mrecv_t(const mrecv_t &) = delete;
  mrecv_t &operator=(const mrecv_t &) = delete;
  mrecv_t(mrecv_t &&) = delete;
  mrecv_t &operator=(mrecv_t &&) = delete;

  ~mrecv_t() noexcept
  {
    if ( __op.__live.get(micron::memory_order_acquire) != 0 ) [[unlikely]]
      mrecv_cancel(*this);
  }
};

[[nodiscard]] inline i32
mrecv_arm(mrecv_t &__mr, i32 __fd, u32 __msg_flags = 0) noexcept
{
  worker *__w = current_worker();
  __wring *__r = __io_own_ring();
  if ( __w == nullptr || __r == nullptr ) return -95;      // EOPNOTSUPP
  if ( __io_pb_init(*__r) != 0 ) return -95;
  __io_mop &__m = __mr.__op;
  if ( __m.__live.get(micron::memory_order_acquire) != 0 ) return -114;      // EALREADY armed
  if ( reinterpret_cast<u64>(&__m) >> __io_ud_tag_shift != 0 ) [[unlikely]]
    __builtin_trap();      // non-canonical address would alias the user_data tag
  __m.__f = nullptr;
  __m.__st.store(__io_mop_idle, micron::memory_order_relaxed);
  __m.__ring = static_cast<u8>(__w->id);
  __m.__live.store(1u, micron::memory_order_release);
  micron::uring::sqe __q;
  micron::uring::prep_recv_multishot(&__q, __fd, __io_pb_bgid, __msg_flags);
  const u64 __ud = __io_ud_make(__io_ud_mop, reinterpret_cast<u64>(&__m));
  __r->__pending.fetch_add(1, micron::memory_order_acq_rel);
  bool __ok = __io_submit_own(*__r, __q, __ud);
  if ( !__ok ) [[unlikely]] {
    if ( __io_cq_acquire(*__r) ) {
      micron::uring::cqe __c{};
      while ( __r->__r.peek_cqe(&__c) ) __global_engine->__dispatch_cqe(*__r, __c);
      __io_unlock(__r->__cq_lk);
    }
    __ok = __io_submit_own(*__r, __q, __ud);
  }
  if ( !__ok ) {
    __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
    __m.__live.store(0u, micron::memory_order_release);
    return -105;      // ENOBUFS: ring full even after flush+reap
  }
  return 0;
}

struct [[nodiscard]] __mrecv_awaitable {
  __io_mop *__m;
  __io_mev __e{};
  bool __have = false;

  bool
  await_ready() noexcept
  {
    if ( __m->__pop(__e) ) {
      __have = true;
      return true;
    }
    if ( __m->__live.get(micron::memory_order_acquire) == 0 ) {
      __e = __io_mev{};
      __e.__res = -61;      // ENODATA: not armed, or the terminal event was already consumed
      __have = true;
      return true;
    }
    return false;
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __m->__f = &__h.promise();
    // __dispatch_cqe may only tag it as __pending from here, never resume it
    __m->__st.store(__io_mop_parking, micron::memory_order_seq_cst);
    if ( __m->__qt.get(micron::memory_order_seq_cst) != __m->__qh.get(micron::memory_order_relaxed)
         || __m->__live.get(micron::memory_order_acquire) == 0 ) {
      __m->__st.store(__io_mop_idle, micron::memory_order_release);
      return false;
    }
    u32 __exp = __io_mop_parking;
    if ( __m->__st.compare_exchange_strong(__exp, __io_mop_parked, micron::memory_order_seq_cst, micron::memory_order_seq_cst) )
      return true;      // last access to this frame; may already be running elsewhere
    __m->__st.store(__io_mop_idle, micron::memory_order_release);
    return false;
  }

  __io_mev
  await_resume() noexcept
  {
    if ( __have ) return __e;
    if ( __m->__pop(__e) ) return __e;
    __io_mev __t{};
    __t.__res = -61;
    return __t;
  }
};

[[nodiscard]] inline __mrecv_awaitable
mrecv_next(mrecv_t &__mr) noexcept
{
  return __mrecv_awaitable{ &__mr.__op };
}

[[nodiscard]] [[gnu::always_inline]] inline const byte *
mrecv_data(const __io_mev &__e) noexcept
{
  if ( (__e.__fl & __io_mev_buf) == 0 ) return nullptr;
  return __io_pb_data(__e.__ring, __e.__bid);
}

[[gnu::always_inline]] inline void
mrecv_recycle(const __io_mev &__e) noexcept
{
  if ( (__e.__fl & __io_mev_buf) != 0 ) __io_pb_recycle(__e.__ring, __e.__bid);
}

[[nodiscard]] [[gnu::always_inline]] inline bool
mrecv_live(const mrecv_t &__mr) noexcept
{
  return __mr.__op.__live.get(micron::memory_order_acquire) != 0;
}

// WARNING: DESTROYING AN ARMED MRECV_T IS A USE AFTER FREE
inline void
mrecv_cancel(mrecv_t &__mr) noexcept
{
  __io_mop &__m = __mr.__op;
  if ( __m.__live.get(micron::memory_order_acquire) != 0 ) {
    const u8 __rid = __m.__ring;
    if ( __rid < __io_ring_cap && __global_engine != nullptr ) {
      __wring &__wr = __io_rings[__rid];
      const u64 __ud = __io_ud_make(__io_ud_mop, reinterpret_cast<u64>(&__m));
      __io_sync_cancel_ud(__wr.__r.fd, __ud);
      for ( u32 __spin = 0; __m.__live.get(micron::memory_order_acquire) != 0; ++__spin ) {
        if ( __io_cq_acquire(__wr) ) {
          micron::uring::cqe __c{};
          while ( __wr.__r.peek_cqe(&__c) ) __global_engine->__dispatch_cqe(__wr, __c);
          __io_unlock(__wr.__cq_lk);
          continue;
        }
        if ( __wr.__live.get(micron::memory_order_acquire) == 0 ) break;      // ring torn down
        micron::cpu_pause<1>();                                               // another reaper holds the cq; it dispatches ours
        if ( (__spin & 0x3ffu) == 0x3ffu ) __io_sync_cancel_ud(__wr.__r.fd, __ud);
      }
    } else {
      __m.__live.store(0u, micron::memory_order_release);      // never armed on a real ring
    }
  }
  __io_mev __e{};
  while ( __m.__pop(__e) ) mrecv_recycle(__e);
  __m.__f = nullptr;
  __m.__st.store(__io_mop_idle, micron::memory_order_relaxed);
}

};      // namespace io
};      // namespace coro
};      // namespace micron

#endif
