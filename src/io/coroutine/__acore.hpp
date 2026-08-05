//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "../../concepts.hpp"
#include "../../memory_block.hpp"
#include "../../type_traits.hpp"

#include "../../tasks/coroutine/aio.hpp"
#include "../../tasks/task.hpp"

#include "../__serial_core.hpp"
#include "../bits.hpp"
#include "../file.hpp"
#include "../fn.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// coro io core
//
// WARNING:
//  a) never co_await inside a call argument or placement-new (GCC ICE)
//  b) never cache worker/ring state across a suspension point;
//     our aio ops reresolve reactor state per submission

namespace micron
{
namespace io
{
namespace coro
{

[[nodiscard]] inline bool
available() noexcept
{
  return micron::coro::__io.any_live.get(micron::memory_order_acquire) != 0;
}

namespace __impl
{

inline constexpr u32 __chunk_cap = 1u << 30;        // sqe len is u32
inline constexpr usize __probe_sz = 64 * 1024;      // growth step for size-0 virtual files

[[gnu::always_inline]] inline i32
__check(i32 fd) noexcept
{
  return fd < 0 ? fd : 0;      // invalid handles carry -errno inline (os_file convention)
}

inline constexpr i32 __nb_unknown = -1;

// cached fd classification, probed once per fd
inline constexpr i32 __fdc_nonblock = 1;      // O_NONBLOCK is set
inline constexpr i32 __fdc_seekable = 2;      // regular file or block device -- p{read,write} is legal, poll is not

[[gnu::always_inline]] inline i32
__fd_class(i32 fd, i32 &__c) noexcept
{
  if ( __c < 0 ) {
    i32 __v = 0;
    const max_t __fl = posix::fcntl(fd_t{ fd }, posix::f_getfl);
    if ( __fl >= 0 && (__fl & posix::o_nonblock) != 0 ) __v |= __fdc_nonblock;
    stat_t __st{};
    if ( posix::fstat(fd_t{ fd }, __st) == 0 && (posix::__impl::stat_is_reg(__st) || posix::__impl::stat_is_blk(__st)) )
      __v |= __fdc_seekable;
    __c = __v;
  }
  return __c;
}

[[gnu::always_inline]] inline bool
__fd_nonblocking(i32 fd, i32 &__c) noexcept
{
  return (__fd_class(fd, __c) & (__fdc_nonblock | __fdc_seekable)) == __fdc_nonblock;
}

[[gnu::always_inline]] inline bool
__fd_seekable(i32 fd, i32 &__c) noexcept
{
  return (__fd_class(fd, __c) & __fdc_seekable) != 0;
}

[[gnu::always_inline]] inline u64
__off_for(i32 fd, u64 off, i32 &__c) noexcept
{
  return (off == static_cast<u64>(-1) || !__fd_seekable(fd, __c)) ? static_cast<u64>(-1) : off;
}

// bound polls that report ready and hit by a second -EAGAIN
inline constexpr u32 __ready_spins = 8;

// suspend until fd is ready
[[nodiscard]] inline micron::task<i32>
__await_ready(i32 fd, u32 __mask)
{
  const i32 __ev = co_await micron::coro::io::poll(fd, __mask);
  if ( __ev < 0 ) co_return __ev;
  if ( (static_cast<u32>(__ev) & micron::uring::poll_nval) != 0 ) co_return -error::bad_fd;
  co_return 0;
}

// WARNING: *_some on a nonblocking pollable fd must __not__ go through the ring; io_uring arms
// its own internal poll and parks instead of returning -EAGAIN, which is right for read() and fatal
// for read_some()
[[nodiscard]] inline micron::task<max_t>
__read_once(i32 fd, void *p, usize n, u64 off, i32 &__nb)
{
  const u64 __at = __off_for(fd, off, __nb);
  if ( __fd_nonblocking(fd, __nb) )
    co_return __at == static_cast<u64>(-1) ? posix::read(fd_t{ fd }, static_cast<byte *>(p), n)
                                           : posix::pread(fd_t{ fd }, static_cast<byte *>(p), n, static_cast<off64_t>(__at));
  const i32 r = co_await micron::coro::io::read(fd, p, static_cast<u32>(n), __at);
  co_return static_cast<max_t>(r);
}

[[nodiscard]] inline micron::task<max_t>
__write_once(i32 fd, const void *p, usize n, u64 off, i32 &__nb)
{
  const u64 __at = __off_for(fd, off, __nb);
  if ( __fd_nonblocking(fd, __nb) )
    co_return __at == static_cast<u64>(-1) ? posix::write(fd_t{ fd }, static_cast<const byte *>(p), n)
                                           : posix::pwrite(fd_t{ fd }, static_cast<const byte *>(p), n, static_cast<off64_t>(__at));
  const i32 w = co_await micron::coro::io::write(fd, p, static_cast<u32>(n), __at);
  co_return static_cast<max_t>(w);
}

[[nodiscard]] inline micron::task<i32>
__retry_after(i32 __res, i32 fd, u32 __mask, i32 &__nb, u32 &__spun)
{
  if ( __res == -error::no_buffer_space ) {        // ring full even after the submit path reaped
    co_await micron::coro::reschedule_fair();      // pumps the ring, then yields behind the worker's other work
    co_return 0;
  }
  if ( __res != -error::try_again ) co_return __res;
  // WARNING: no progress bound must be set before the blocking/nonblocking split
  if ( ++__spun > __ready_spins ) co_return __res;      // readiness keeps loopin; surrender
  if ( !__fd_nonblocking(fd, __nb) ) {
    co_await micron::coro::reschedule_fair();
    co_return 0;
  }
  const i32 __r = co_await __await_ready(fd, __mask);
  co_return __r < 0 ? __r : 0;
}

[[nodiscard]] inline micron::task<i32>
__retry_after2(i32 __res, i32 __in, i32 __out, u32 &__spun, u32 &__turn)
{
  // WARNING: both ends must be polled regardless of O_NONBLOCK, unlike the single fd retry
  if ( __res == -error::no_buffer_space ) {
    co_await micron::coro::reschedule_fair();
    co_return 0;
  }
  if ( __res != -error::try_again ) co_return __res;
  const bool __probe_in = (__turn++ & 1u) == 0u;
  if ( !__probe_in && ++__spun > __ready_spins ) co_return __res;      // both ends probed, nothing moved
  const i32 __r = co_await __await_ready(__probe_in ? __in : __out, __probe_in ? micron::uring::poll_in : micron::uring::poll_out);
  co_return __r < 0 ? __r : 0;
}

[[nodiscard]] inline micron::task<max_t>
__read_full(i32 fd, void *p, usize n, u64 off)
{
  byte *dst = static_cast<byte *>(p);
  usize got = 0;
  i32 nb = __nb_unknown;
  u32 spun = 0;
  // NOTE: no seekability probe
  while ( got < n ) {
    usize want = n - got;
    if ( want > __chunk_cap ) want = __chunk_cap;
    const u64 at = off == static_cast<u64>(-1) ? off : off + got;
    i32 r = co_await micron::coro::io::read(fd, dst + got, static_cast<u32>(want), at);
    if ( r < 0 ) [[unlikely]] {
      if ( r == -4 /*EINTR*/ ) continue;
      const i32 a = co_await __retry_after(r, fd, micron::uring::poll_in, nb, spun);
      if ( a == 0 ) continue;
      co_return got ? static_cast<max_t>(got) : static_cast<max_t>(a);
    }
    if ( r == 0 ) break;
    got += static_cast<usize>(r);
    spun = 0;
  }
  co_return static_cast<max_t>(got);
}

[[nodiscard]] inline micron::task<max_t>
__write_full(i32 fd, const void *p, usize n, u64 off)
{
  const byte *src = static_cast<const byte *>(p);
  usize done = 0;
  i32 nb = __nb_unknown;
  u32 spun = 0;
  while ( done < n ) {
    usize want = n - done;
    if ( want > __chunk_cap ) want = __chunk_cap;
    const u64 at = off == static_cast<u64>(-1) ? off : off + done;
    i32 w = co_await micron::coro::io::write(fd, src + done, static_cast<u32>(want), at);
    if ( w < 0 ) [[unlikely]] {
      if ( w == -4 /*EINTR*/ ) continue;
      const i32 a = co_await __retry_after(w, fd, micron::uring::poll_out, nb, spun);
      if ( a == 0 ) continue;
      co_return done ? static_cast<max_t>(done) : static_cast<max_t>(a);
    }
    if ( w == 0 ) break;
    done += static_cast<usize>(w);
    spun = 0;
  }
  co_return static_cast<max_t>(done);
}

[[nodiscard]] inline micron::task<max_t>
__read_remainder(i32 fd, u64 off, micron::buffer &out)
{
  stat_t st{};
  usize total = 0;
  if ( posix::fstat(fd_t{ fd }, st) == 0 && st.st_size > 0 ) total = static_cast<usize>(st.st_size);
  if ( total > off ) {
    const usize want = total - static_cast<usize>(off);
    if ( out.size() < want ) out = micron::buffer(want);
    max_t got = co_await __read_full(fd, out.data(), want, off);
    co_return got;
  }
  usize cap = __probe_sz;
  micron::buffer grow(cap);
  usize got = 0;
  i32 nb = __nb_unknown;
  u32 spun = 0;
  for ( ;; ) {
    if ( got == cap ) {
      cap *= 2;
      micron::buffer bigger(cap);
      micron::bytecpy(bigger.data(), grow.data(), got);
      grow = micron::move(bigger);
    }
    usize want = cap - got;
    if ( want > __chunk_cap ) want = __chunk_cap;
    i32 r = co_await micron::coro::io::read(fd, reinterpret_cast<byte *>(grow.data()) + got, static_cast<u32>(want), off + got);
    if ( r < 0 ) [[unlikely]] {
      if ( r == -4 ) continue;
      const i32 a = co_await __retry_after(r, fd, micron::uring::poll_in, nb, spun);
      if ( a == 0 ) continue;
      co_return static_cast<max_t>(a);
    }
    if ( r == 0 ) break;
    got += static_cast<usize>(r);
    spun = 0;
  }
  out = micron::move(grow);
  co_return static_cast<max_t>(got);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// tier engines

template<typename T>
  requires __readable_value<T>
[[nodiscard]] micron::task<max_t>
__write_value(i32 fd, u64 off, const T &v)
{
  if constexpr ( __bulk_value<T> ) {
    max_t w = co_await __write_full(fd, v.data(), v.size() * sizeof(typename T::value_type), off);
    co_return w;
  } else if constexpr ( __framed_value<T> ) {
    max_t need = serialize::framed_size(v);
    if ( need < 0 ) [[unlikely]]
      co_return need;
    micron::buffer out(static_cast<usize>(need));
    max_t n = serialize::frame_into(reinterpret_cast<byte *>(out.data()), out.size(), v);
    if ( n < 0 ) [[unlikely]]
      co_return n;
    max_t w = co_await __write_full(fd, out.data(), static_cast<usize>(n), off);
    co_return w;
  } else {
    max_t w = co_await __write_full(fd, micron::addressof(v), sizeof(T), off);
    co_return w;
  }
}

template<typename T>
[[nodiscard]] inline max_t
__marshal_size(const T &v) noexcept
{
  if constexpr ( __bulk_value<T> )
    return static_cast<max_t>(v.size() * sizeof(typename T::value_type));
  else if constexpr ( __framed_value<T> )
    return serialize::framed_size(v);
  else
    return static_cast<max_t>(sizeof(T));
}

template<typename T>
static void
__set_length(T &c, usize elems)
{
  if constexpr ( requires(T t, usize n) { t.set_size(n); } )
    c.set_size(elems);
  else if constexpr ( requires(T t, usize n) { t._buf_set_length(n); } )
    c._buf_set_length(elems);
}

template<typename T>
  requires __readable_value<T>
[[nodiscard]] micron::task<max_t>
__read_value_into(i32 fd, u64 off, T &out)
{
  if constexpr ( __bulk_value<T> ) {
    micron::buffer raw(0);
    max_t got = co_await __read_remainder(fd, off, raw);
    if ( got < 0 ) [[unlikely]]
      co_return got;
    const usize elems = static_cast<usize>(got) / sizeof(typename T::value_type);
    if constexpr ( requires(T t, usize n) { t.reserve(n); } ) out.reserve(elems + 1);
    if constexpr ( requires(const T &t) { t.max_size(); } ) {
      if ( elems > out.max_size() ) [[unlikely]]
        co_return -error::file_too_big;
    }
    __set_length(out, elems);
    if ( elems != 0 ) micron::bytecpy(reinterpret_cast<byte *>(out.data()), raw.data(), elems * sizeof(typename T::value_type));
    co_return got;
  } else if constexpr ( __framed_value<T> ) {
    micron::buffer raw(0);
    max_t got = co_await __read_remainder(fd, off, raw);
    if ( got < 0 ) [[unlikely]]
      co_return got;
    if ( got == 0 ) co_return 0;
    max_t used = serialize::unframe_from(reinterpret_cast<const byte *>(raw.data()), static_cast<usize>(got), out);
    if ( used < 0 ) [[unlikely]]
      co_return used;
    co_return got;
  } else {
    max_t got = co_await __read_full(fd, micron::addressof(out), sizeof(T), off);
    if ( got < 0 ) [[unlikely]]
      co_return got;
    if ( static_cast<usize>(got) != sizeof(T) ) [[unlikely]]
      co_return -error::io_error;      // a short object read is corruption
    co_return got;
  }
}

template<class X> struct __is_task: micron::false_type {
  using value_t = X;
};

template<class U> struct __is_task<micron::task<U>>: micron::true_type {
  using value_t = U;
};

template<class X> inline constexpr bool __is_task_v = __is_task<micron::remove_cvref_t<X>>::value;

template<class X> using __task_inner_t = typename __is_task<micron::remove_cvref_t<X>>::value_t;

};      // namespace __impl

};      // namespace coro
};      // namespace io
};      // namespace micron

#endif
