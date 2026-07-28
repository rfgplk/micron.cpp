//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "__alines.hpp"
#include "pipe.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// coro stdio

namespace micron
{
namespace io
{
namespace coro
{

[[nodiscard]] inline __aline_cursor
stdin_lines(usize chunk_sz = __lines::chunk)
{
  return __aline_cursor(0, static_cast<u64>(-1), chunk_sz);
}

[[nodiscard]] inline micron::task<max_t>
read_in(micron::string &out)
{
  micron::buffer chunk(__lines::chunk);
  max_t total = 0;
  for ( ;; ) {
    i32 r = co_await micron::coro::io::read(0, chunk.data(), static_cast<u32>(chunk.size()));
    if ( r < 0 ) [[unlikely]] {
      if ( r == -4 /*EINTR*/ ) continue;
      co_return static_cast<max_t>(r);
    }
    if ( r == 0 ) break;
    out.append(reinterpret_cast<const char *>(chunk.data()), static_cast<usize>(r));
    total += r;
  }
  co_return total;
}

[[nodiscard]] inline micron::task<max_t>
write_out(const void *p, usize n)
{
  max_t w = co_await __impl::__write_full(1, p, n, static_cast<u64>(-1));
  co_return w;
}

template<is_string S>
[[nodiscard]] micron::task<max_t>
write_out(const S &s)
{
  max_t w = co_await __impl::__write_full(1, s.c_str(), s.size() * sizeof(typename S::value_type), static_cast<u64>(-1));
  co_return w;
}

[[nodiscard]] inline micron::task<max_t>
write_err(const void *p, usize n)
{
  max_t w = co_await __impl::__write_full(2, p, n, static_cast<u64>(-1));
  co_return w;
}

template<is_string S>
[[nodiscard]] micron::task<max_t>
write_err(const S &s)
{
  max_t w = co_await __impl::__write_full(2, s.c_str(), s.size() * sizeof(typename S::value_type), static_cast<u64>(-1));
  co_return w;
}

};      // namespace coro
};      // namespace io
};      // namespace micron

#endif
