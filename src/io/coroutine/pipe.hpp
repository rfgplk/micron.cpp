//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "../pipe.hpp"
#include "__acore.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// coroio pipes + raw fds

namespace micron
{
namespace io
{
namespace coro
{

struct fd_io {
  i32 fd = -1;
  i32 __nb = __impl::__nb_unknown;      // O_NONBLOCK probe

  [[nodiscard]] micron::task<max_t>
  read_some(void *p, usize n)
  {
    if ( i32 e = __impl::__check(fd) ) [[unlikely]]
      co_return e;
    if ( n > __impl::__chunk_cap ) n = __impl::__chunk_cap;
    max_t r = co_await __impl::__read_once(fd, p, n, static_cast<u64>(-1), __nb);
    co_return r;
  }

  [[nodiscard]] micron::task<max_t>
  write_some(const void *p, usize n)
  {
    if ( i32 e = __impl::__check(fd) ) [[unlikely]]
      co_return e;
    if ( n > __impl::__chunk_cap ) n = __impl::__chunk_cap;
    max_t w = co_await __impl::__write_once(fd, p, n, static_cast<u64>(-1), __nb);
    co_return w;
  }

  [[nodiscard]] micron::task<max_t>
  read(void *p, usize n)
  {
    if ( i32 e = __impl::__check(fd) ) [[unlikely]]
      co_return e;
    max_t r = co_await __impl::__read_full(fd, p, n, static_cast<u64>(-1));
    co_return r;
  }

  [[nodiscard]] micron::task<max_t>
  write(const void *p, usize n)
  {
    if ( i32 e = __impl::__check(fd) ) [[unlikely]]
      co_return e;
    max_t w = co_await __impl::__write_full(fd, p, n, static_cast<u64>(-1));
    co_return w;
  }
};

[[nodiscard]] inline micron::task<max_t>
read_some(i32 fd, void *p, usize n)
{
  fd_io io{ fd };
  max_t r = co_await io.read_some(p, n);
  co_return r;
}

[[nodiscard]] inline micron::task<max_t>
read_some(upipe &p, void *dst, usize n)
{
  fd_io io{ static_cast<i32>(p.read_fd()) };
  max_t r = co_await io.read_some(dst, n);
  co_return r;
}

[[nodiscard]] inline micron::task<max_t>
write_all(i32 fd, const void *p, usize n)
{
  fd_io io{ fd };
  max_t w = co_await io.write(p, n);
  co_return w;
}

[[nodiscard]] inline micron::task<max_t>
write_all(upipe &p, const void *src, usize n)
{
  fd_io io{ static_cast<i32>(p.write_fd()) };
  max_t w = co_await io.write(src, n);
  co_return w;
}

template<is_string S>
[[nodiscard]] micron::task<max_t>
write_all(upipe &p, const S &s)
{
  fd_io io{ static_cast<i32>(p.write_fd()) };
  max_t w = co_await io.write(s.c_str(), s.size() * sizeof(typename S::value_type));
  co_return w;
}

namespace __impl
{

template<chunk_fn Fn>
[[nodiscard]] micron::task<max_t>
__each_chunk_fd(i32 fd, Fn fn, usize chunk_sz)
{
  micron::buffer win(chunk_sz);
  max_t total = 0;
  i32 nb = __nb_unknown;
  u32 spun = 0;
  for ( ;; ) {
    i32 r = co_await micron::coro::io::read(fd, win.data(), static_cast<u32>(chunk_sz));
    if ( r < 0 ) [[unlikely]] {
      if ( r == -4 /*EINTR*/ ) continue;
      const i32 a = co_await __retry_after(r, fd, micron::uring::poll_in, nb, spun);
      if ( a == 0 ) continue;
      co_return static_cast<max_t>(a);
    }
    if ( r == 0 ) break;
    fn(reinterpret_cast<const byte *>(win.data()), static_cast<usize>(r));
    total += r;
    spun = 0;
  }
  co_return total;
}

template<producer_fn Fn>
[[nodiscard]] micron::task<max_t>
__write_with_fd(i32 fd, Fn fn, usize chunk_sz)
{
  micron::buffer win(chunk_sz);
  max_t total = 0;
  for ( ;; ) {
    usize n = fn(reinterpret_cast<byte *>(win.data()), chunk_sz);
    if ( n == 0 ) break;
    if ( n > chunk_sz ) [[unlikely]]
      co_return -error::invalid_arg;
    max_t w = co_await __write_full(fd, win.data(), n, static_cast<u64>(-1));
    if ( w < 0 ) [[unlikely]]
      co_return w;
    if ( static_cast<usize>(w) != n ) [[unlikely]]
      co_return -error::io_error;
    total += w;
  }
  co_return total;
}

};      // namespace __impl

template<chunk_fn Fn>
[[nodiscard]] micron::task<max_t>
each_chunk(upipe &p, Fn fn, usize chunk_sz = 4096)
{
  max_t r = co_await __impl::__each_chunk_fd(static_cast<i32>(p.read_fd()), micron::move(fn), chunk_sz);
  co_return r;
}

template<producer_fn Fn>
[[nodiscard]] micron::task<max_t>
write_with(upipe &p, Fn fn, usize chunk_sz = 4096)
{
  max_t r = co_await __impl::__write_with_fd(static_cast<i32>(p.write_fd()), micron::move(fn), chunk_sz);
  co_return r;
}

[[nodiscard]] inline micron::task<max_t>
splice(i32 in_fd, i32 out_fd, usize n)
{
  usize moved = 0;
  u32 spun = 0;
  u32 turn = 0;
  while ( moved < n ) {
    usize want = n - moved;
    if ( want > __impl::__chunk_cap ) want = __impl::__chunk_cap;
    i32 r = co_await micron::coro::io::splice(in_fd, static_cast<u64>(-1), out_fd, static_cast<u64>(-1), static_cast<u32>(want));
    if ( r < 0 ) [[unlikely]] {
      if ( r == -4 ) continue;
      const i32 a = co_await __impl::__retry_after2(r, in_fd, out_fd, spun, turn);
      if ( a == 0 ) continue;
      co_return moved ? static_cast<max_t>(moved) : static_cast<max_t>(a);
    }
    if ( r == 0 ) break;
    moved += static_cast<usize>(r);
    spun = 0;
    turn = 0;
  }
  co_return static_cast<max_t>(moved);
}

[[nodiscard]] inline micron::task<max_t>
tee(upipe &from, upipe &to, usize n)
{
  if ( n > __impl::__chunk_cap ) n = __impl::__chunk_cap;
  i32 r = co_await micron::coro::io::tee(static_cast<i32>(from.read_fd()), static_cast<i32>(to.write_fd()), static_cast<u32>(n));
  co_return static_cast<max_t>(r);
}

};      // namespace coro
};      // namespace io
};      // namespace micron

#endif
