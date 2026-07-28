//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "fsys.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// functional coroio

namespace micron
{
namespace io
{
namespace coro
{

template<typename Fn>
  requires micron::invocable<Fn, file &>
[[nodiscard]] auto
with_file(io::path_t p, modes m, Fn fn)
    -> micron::task<micron::option<__unit_if_void_t<micron::conditional_t<__impl::__is_task_v<micron::invoke_result_t<Fn, file &>>,
                                                                          __impl::__task_inner_t<micron::invoke_result_t<Fn, file &>>,
                                                                          micron::invoke_result_t<Fn, file &>>>,
                                   io::error_t>>
{
  using R0 = micron::invoke_result_t<Fn, file &>;
  using RV = micron::conditional_t<__impl::__is_task_v<R0>, __impl::__task_inner_t<R0>, R0>;
  using Ret = micron::option<__unit_if_void_t<RV>, io::error_t>;
  file f = co_await coro::open_file(micron::move(p), m);
  if ( !f.valid() ) [[unlikely]]
    co_return Ret{ io::error_t(f.raw_fd()) };
  if constexpr ( __impl::__is_task_v<R0> ) {
    R0 t = fn(f);
    if constexpr ( micron::is_void_v<RV> ) {
      co_await micron::move(t);
      co_return Ret{ unit_t{} };
    } else {
      RV v = co_await micron::move(t);
      co_return Ret{ micron::move(v) };
    }
  } else {
    if constexpr ( micron::is_void_v<R0> ) {
      fn(f);
      co_return Ret{ unit_t{} };
    } else {
      co_return Ret{ fn(f) };
    }
  }
}

template<typename Fn>
  requires micron::invocable<Fn, file &>
[[nodiscard]] auto
with_file(io::path_t p, Fn fn)
{

  return coro::with_file(micron::move(p), modes::read, micron::move(fn));
}

template<typename Fn>
  requires requires(file &f, Fn fn) { f.modify(micron::move(fn)); }
[[nodiscard]] micron::task<max_t>
modify(io::path_t p, Fn fn)
{
  file f = co_await coro::open_file(micron::move(p), modes::readwrite);
  if ( !f.valid() ) [[unlikely]]
    co_return f.raw_fd();
  max_t r = co_await f.modify(micron::move(fn));
  co_return r;
}

template<typename C = micron::vector<micron::string>>
  requires requires(C c, micron::string s) { c.push_back(micron::move(s)); }
[[nodiscard]] micron::task<micron::option<C, io::error_t>>
read_lines(io::path_t p)
{
  using Ret = micron::option<C, io::error_t>;
  file f = co_await coro::open_file(micron::move(p));
  if ( !f.valid() ) [[unlikely]]
    co_return Ret{ io::error_t(f.raw_fd()) };
  C out{};
  __aline_cursor cur = f.lines();
  micron::string ln;
  for ( ;; ) {
    bool more = co_await cur.next(ln);
    if ( !more ) break;
    out.push_back(ln);
  }
  if ( i32 e = cur.error() ) [[unlikely]]
    co_return Ret{ io::error_t(e) };
  co_return Ret{ micron::move(out) };
}

template<typename C>
  requires requires(C c, micron::string s) { c.push_back(micron::move(s)); }
[[nodiscard]] micron::task<max_t>
read_lines(io::path_t p, C &target)
{
  file f = co_await coro::open_file(micron::move(p));
  if ( !f.valid() ) [[unlikely]]
    co_return f.raw_fd();
  __aline_cursor cur = f.lines();
  micron::string ln;
  max_t count = 0;
  for ( ;; ) {
    bool more = co_await cur.next(ln);
    if ( !more ) break;
    target.push_back(ln);
    ++count;
  }
  if ( i32 e = cur.error() ) [[unlikely]]
    co_return e;
  co_return count;
}

template<typename Fn>
  requires(micron::is_invocable_v<Fn, micron::string &&> && is_string<micron::invoke_result_t<Fn, micron::string &&>>)
[[nodiscard]] micron::task<max_t>
interact(Fn fn)
{
  micron::string in{};
  micron::buffer chunk(__lines::chunk);
  for ( ;; ) {
    i32 r = co_await micron::coro::io::read(0, chunk.data(), static_cast<u32>(chunk.size()));
    if ( r < 0 ) [[unlikely]] {
      if ( r == -4 /*EINTR*/ ) continue;
      co_return static_cast<max_t>(r);
    }
    if ( r == 0 ) break;
    in.append(reinterpret_cast<const char *>(chunk.data()), static_cast<usize>(r));
  }
  auto out = fn(micron::move(in));
  max_t w = co_await __impl::__write_full(1, out.c_str(), out.size() * sizeof(typename decltype(out)::value_type), static_cast<u64>(-1));
  co_return w;
}

};      // namespace coro
};      // namespace io
};      // namespace micron

#endif
