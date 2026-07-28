//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "../../string/strings.hpp"

#include "../__lines.hpp"
#include "__acore.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// async line cursor

namespace micron
{
namespace io
{
namespace coro
{

class __aline_cursor
{
  i32 __fd;
  u64 __off;
  micron::buffer __chunk;
  usize __chunk_sz;
  usize __pos = 0;
  usize __valid = 0;
  i32 __err = 0;
  bool __eof = false;
  bool __stream;      // off == -1: unseekable fd (pipe/tty/stdin) -> kernel position

public:
  explicit __aline_cursor(i32 fd, u64 off = 0, usize chunk_sz = __lines::chunk)
      : __fd(fd), __off(off), __chunk(chunk_sz ? chunk_sz : __lines::chunk), __chunk_sz(chunk_sz ? chunk_sz : __lines::chunk),
        __stream(off == static_cast<u64>(-1))
  {
    if ( __stream ) __off = 0;
  }

  __aline_cursor(const __aline_cursor &) = delete;
  __aline_cursor &operator=(const __aline_cursor &) = delete;
  __aline_cursor(__aline_cursor &&) = delete;
  __aline_cursor &operator=(__aline_cursor &&) = delete;

  [[nodiscard]] micron::task<bool>
  next(micron::string &out)
  {
    if ( __eof || __err != 0 ) co_return false;
    out.set_size(0);
    const char *base = reinterpret_cast<const char *>(__chunk.data());
    for ( ;; ) {
      for ( usize i = __pos; i < __valid; ++i ) {
        if ( base[i] != '\n' ) continue;
        if ( i > __pos ) out.append(base + __pos, i - __pos);
        __pos = i + 1;
        if ( out.size() && out[out.size() - 1] == '\r' ) out.set_size(out.size() - 1);
        co_return true;
      }
      if ( __valid > __pos ) out.append(base + __pos, __valid - __pos);
      __pos = __valid;
      const u64 at = __stream ? static_cast<u64>(-1) : __off;
      i32 r = co_await micron::coro::io::read(__fd, __chunk.data(), static_cast<u32>(__chunk_sz), at);
      if ( r < 0 ) [[unlikely]] {
        if ( r == -4 /*EINTR*/ ) continue;
        __err = r;
        out.set_size(0);
        co_return false;
      }
      if ( r == 0 ) {
        __eof = true;
        co_return !out.empty();      // final unterminated line, emitted once
      }
      __off += static_cast<u64>(r);
      __pos = 0;
      __valid = static_cast<usize>(r);
    }
  }

  [[nodiscard]] i32
  error() const noexcept
  {
    return __err;
  }

  [[nodiscard]] bool
  at_eof() const noexcept
  {
    return __eof;
  }

  [[nodiscard]] u64
  offset() const noexcept
  {
    return __off - (__valid - __pos);
  }
};

};      // namespace coro
};      // namespace io
};      // namespace micron

#endif
