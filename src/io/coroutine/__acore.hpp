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

[[gnu::always_inline]] inline bool
__eagain_transient(i32 fd, i32 &__nb) noexcept
{
  if ( __nb < 0 ) {
    const max_t __fl = posix::fcntl(fd_t{ fd }, posix::f_getfl);
    __nb = (__fl >= 0 && (__fl & posix::o_nonblock) != 0) ? 1 : 0;
  }
  return __nb == 0;
}

[[nodiscard]] inline micron::task<max_t>
__read_full(i32 fd, void *p, usize n, u64 off)
{
  byte *dst = static_cast<byte *>(p);
  usize got = 0;
  i32 nb = -1;
  while ( got < n ) {
    usize want = n - got;
    if ( want > __chunk_cap ) want = __chunk_cap;
    const u64 at = off == static_cast<u64>(-1) ? off : off + got;
    i32 r = co_await micron::coro::io::read(fd, dst + got, static_cast<u32>(want), at);
    if ( r < 0 ) [[unlikely]] {
      if ( r == -4 /*EINTR*/ ) continue;
      if ( r == -105 /*ENOBUFS: ring full even after the submit path reaped*/
           || (r == -11 /*EAGAIN*/ && __eagain_transient(fd, nb)) ) {
        co_await micron::coro::reschedule_fair();      // pumps the ring, then yields behind the worker's other work
        continue;
      }
      co_return got ? static_cast<max_t>(got) : static_cast<max_t>(r);
    }
    if ( r == 0 ) break;
    got += static_cast<usize>(r);
  }
  co_return static_cast<max_t>(got);
}

[[nodiscard]] inline micron::task<max_t>
__write_full(i32 fd, const void *p, usize n, u64 off)
{
  const byte *src = static_cast<const byte *>(p);
  usize done = 0;
  i32 nb = -1;
  while ( done < n ) {
    usize want = n - done;
    if ( want > __chunk_cap ) want = __chunk_cap;
    const u64 at = off == static_cast<u64>(-1) ? off : off + done;
    i32 w = co_await micron::coro::io::write(fd, src + done, static_cast<u32>(want), at);
    if ( w < 0 ) [[unlikely]] {
      if ( w == -4 /*EINTR*/ ) continue;
      if ( w == -105 /*ENOBUFS*/ || (w == -11 /*EAGAIN*/ && __eagain_transient(fd, nb)) ) {
        co_await micron::coro::reschedule_fair();      // pumps the ring, then yields behind the worker's other work
        continue;
      }
      co_return done ? static_cast<max_t>(done) : static_cast<max_t>(w);
    }
    if ( w == 0 ) break;
    done += static_cast<usize>(w);
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
  // grow until EOF
  usize cap = __probe_sz;
  micron::buffer grow(cap);
  usize got = 0;
  i32 nb = -1;
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
      if ( r == -105 /*ENOBUFS*/ || (r == -11 && __eagain_transient(fd, nb)) ) {
        co_await micron::coro::reschedule_fair();      // pumps the ring, then yields behind the worker's other work
        continue;
      }
      co_return static_cast<max_t>(r);
    }
    if ( r == 0 ) break;
    got += static_cast<usize>(r);
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
