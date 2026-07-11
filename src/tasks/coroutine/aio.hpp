//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "cl_sched.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// awaitable io over the reactor ring
//
// NOTE: partial port of code from libjkr

namespace micron
{
namespace coro
{
namespace io
{

struct [[nodiscard]] __uring_awaitable {
  __io_op __op{};
  u8 __opcode;
  i32 __fd;
  u64 __addr;
  u32 __len;
  u64 __off;
  u32 __rwflags;

  __uring_awaitable(u8 opc, i32 fd, u64 addr, u32 len, u64 off, u32 rwflags) noexcept
      : __opcode(opc), __fd(fd), __addr(addr), __len(len), __off(off), __rwflags(rwflags)
  {
  }

  bool
  await_ready() noexcept
  {
    if ( __io.live ) return false;
    __op.__res = -38;      // ENOSYS: no ring on this system
    return true;
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __op.__f = &__h.promise();
    __io.pending.fetch_add(1, micron::memory_order_acq_rel);
    if ( __io_submit(reinterpret_cast<u64>(&__op), [&](micron::uring::sqe *__s) {
           micron::uring::__prep(__s, __opcode, __fd, __addr, __len, __off);
           __s->rw_flags = __rwflags;
         }) )
      return true;
    __io.pending.sub_fetch(1, micron::memory_order_acq_rel);
    __op.__res = -11;      // EAGAIN: SQ stayed full even after a flush; resume inline
    return false;
  }

  i32
  await_resume() const noexcept
  {
    return __op.__res;
  }
};

// positional reads/writes; off==-1 uses (and advances) the file position
[[nodiscard]] inline __uring_awaitable
read(i32 fd, void *buf, u32 n, u64 off = static_cast<u64>(-1)) noexcept
{
  return { micron::uring::op_read, fd, reinterpret_cast<u64>(buf), n, off, 0 };
}

[[nodiscard]] inline __uring_awaitable
write(i32 fd, const void *buf, u32 n, u64 off = static_cast<u64>(-1)) noexcept
{
  return { micron::uring::op_write, fd, reinterpret_cast<u64>(buf), n, off, 0 };
}

[[nodiscard]] inline __uring_awaitable
openat(i32 dirfd, const char *path, u32 open_flags, u32 mode = 0) noexcept
{
  return { micron::uring::op_openat, dirfd, reinterpret_cast<u64>(path), mode, 0, open_flags };
}

[[nodiscard]] inline __uring_awaitable
close(i32 fd) noexcept
{
  return { micron::uring::op_close, fd, 0, 0, 0, 0 };
}

[[nodiscard]] inline __uring_awaitable
fsync(i32 fd, u32 fsync_flags = 0) noexcept
{
  return { micron::uring::op_fsync, fd, 0, 0, 0, fsync_flags };
}

[[nodiscard]] inline __uring_awaitable
accept(i32 fd, void *addr = nullptr, u32 *addrlen = nullptr, u32 accept_flags = 0) noexcept
{
  return { micron::uring::op_accept, fd, reinterpret_cast<u64>(addr), 0, reinterpret_cast<u64>(addrlen), accept_flags };
}

[[nodiscard]] inline __uring_awaitable
connect(i32 fd, const void *addr, u32 addrlen) noexcept
{
  return { micron::uring::op_connect, fd, reinterpret_cast<u64>(addr), 0, addrlen, 0 };
}

[[nodiscard]] inline __uring_awaitable
send(i32 fd, const void *buf, u32 n, u32 msg_flags = 0) noexcept
{
  return { micron::uring::op_send, fd, reinterpret_cast<u64>(buf), n, 0, msg_flags };
}

[[nodiscard]] inline __uring_awaitable
recv(i32 fd, void *buf, u32 n, u32 msg_flags = 0) noexcept
{
  return { micron::uring::op_recv, fd, reinterpret_cast<u64>(buf), n, 0, msg_flags };
}

// resumes when any event in poll_mask is ready on fd (res = ready mask)
[[nodiscard]] inline __uring_awaitable
poll(i32 fd, u32 poll_mask) noexcept
{
  return { micron::uring::op_poll_add, fd, 0, 0, 0, poll_mask };
}

[[nodiscard]] inline __uring_awaitable
shutdown(i32 fd, i32 how) noexcept
{
  return { micron::uring::op_shutdown, fd, 0, static_cast<u32>(how), 0, 0 };
}

};      // namespace io
};      // namespace coro
};      // namespace micron

#endif
