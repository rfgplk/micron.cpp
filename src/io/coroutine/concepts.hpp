//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "file.hpp"
#include "pipe.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// async io concepts (libjkr-forward)

namespace micron
{
namespace io
{
namespace coro
{

// direct awaitables (aio plumbing)
template<class A>
concept __plain_awaitable = requires(A a) {
  { a.await_ready() } -> micron::convertible_to<bool>;
  a.await_resume();
};

template<class A>
concept __member_co_await = requires(A a) { static_cast<A &&>(a).operator co_await(); };

template<class A>
concept __awaitable = __plain_awaitable<A> || __member_co_await<A>;

namespace __impl
{

template<class A> struct __await_result {
};

template<__plain_awaitable A> struct __await_result<A> {
  using type = decltype(micron::declval<A &>().await_resume());
};

template<class A>
  requires(__member_co_await<A> && !__plain_awaitable<A>)
struct __await_result<A> {
  using type = decltype(micron::declval<A>().operator co_await().await_resume());
};

};      // namespace __impl

template<__awaitable A> using __await_result_t = typename __impl::__await_result<A>::type;

template<class A>
concept __io_awaitable = __awaitable<A> && micron::convertible_to<__await_result_t<A>, max_t>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the contracts

template<class T>
concept async_readable = requires(T t, void *p, usize n) {
  { t.read_some(p, n) } -> __io_awaitable;
};

template<class T>
concept async_writable = requires(T t, const void *p, usize n) {
  { t.write_some(p, n) } -> __io_awaitable;
};

template<class T>
concept async_stream = async_readable<T> && async_writable<T>;

template<class T>
concept async_seekable = async_readable<T> && requires(T t, u64 off, void *p, usize n) {
  { t.read_at(off, p, n) } -> __io_awaitable;
};

static_assert(async_readable<file>);
static_assert(async_writable<file>);
static_assert(async_seekable<file>);
static_assert(async_stream<fd_io>);

};      // namespace coro
};      // namespace io
};      // namespace micron

#endif
