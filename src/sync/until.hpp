//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "invoke.hpp"

#include "../atomic/atomic.hpp"
#include "../bits/__pause.hpp"
#include "../chrono.hpp"
#include "../memory/actions.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "future.hpp"

namespace micron
{

template <typename T, typename F, typename... Args>
void
until(const T &cond, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;
  for ( ;; ) {
    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;
    __cpu_pause();
  }
}

template <typename T>
void
until(const T &cond, const T &var)
{
  while ( var != cond ) {
    __cpu_pause();
  }
}

template <typename T>
void
until(const T &cond, const micron::atomic<T> &var)
{
  while ( var.get(micron::memory_order_acquire) != cond ) {
    __cpu_pause();
  }
}

template <typename T, typename Compare>
void
until(const T &cond, const T &var, Compare comp)
{
  while ( !comp(var, cond) ) {
    __cpu_pause();
  }
}

template <typename P>
  requires micron::is_invocable_r_v<bool, P>
void
until(P pred)
{
  while ( !pred() ) {
    __cpu_pause();
  }
}

template <typename P, typename... Args>
  requires micron::is_invocable_r_v<bool, P, Args...>
void
until_true(P pred, Args &&...args)
{
  while ( !micron::invoke(pred, micron::forward<Args>(args)...) ) {
    __cpu_pause();
  }
}

template <typename F, typename... Args>
bool
until_timeout(auto cond, micron::duration_d timeout, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;
  auto start = micron::system_clock<>::now();

  for ( ;; ) {
    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      return true;

    if ( micron::system_clock<>::now() - start >= timeout )
      return false;

    __cpu_pause();
  }
}

template <typename P>
bool
until_timeout(micron::duration_d timeout, P pred)
{
  auto start = micron::system_clock<>::now();

  while ( !pred() ) {
    if ( micron::system_clock<>::now() - start >= timeout )
      return false;
    __cpu_pause();
  }
  return true;
}

template <typename F, typename... Args>
bool
until_max_iter(auto cond, size_t max_iters, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( size_t i = 0; i < max_iters; ++i ) {
    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      return true;
    __cpu_pause();
  }
  return false;
}

template <typename F>
auto
until_ready(F &fut)
{
  while ( fut.wait_for(micron::duration_d(0)) != micron::future_status::ready ) {
    __cpu_pause();
  }
  return fut.get();
}

template <typename F>
bool
until_ready_timeout(F &fut, micron::duration_d timeout)
{
  auto start = micron::system_clock<>::now();

  while ( fut.wait_for(micron::duration_d(0)) != micron::future_status::ready ) {
    if ( micron::system_clock<>::now() - start >= timeout )
      return false;
    __cpu_pause();
  }
  return true;
}

template <typename T, typename F, typename Fn, typename... Args>
void
until_with_wait(const T &cond, Fn wait, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;
    wait();
  }
}

template <typename T, typename F, typename Cb, typename... Args>
void
until_with_callback(const T &cond, Cb cb, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    cb(res);
    if ( res == cond )
      break;
    __cpu_pause();
  }
}

template <typename F, typename... C>
void
until_any(F f, C &&...conds)
{
  using ret_t = micron::invoke_result_t<F>;

  for ( ;; ) {
    ret_t res = f();
    if ( ((res == conds) || ...) )
      break;
    __cpu_pause();
  }
}

template <typename... Args>
void
until_any(Args &&...args, bool value)
{
  while ( (args() || ...) != value ) {
    __cpu_pause();
  }
}

template <typename... Args>
void
until_any_true(Args &&...args)
{
  while ( !(args() || ...) ) {
    __cpu_pause();
  }
}

template <typename... Args>
void
until_all_true(Args &&...args)
{
  while ( !(args() && ...) ) {
    __cpu_pause();
  }
}

template <typename... Args>
void
until_any_false(Args &&...args)
{
  while ( (args() || ...) ) {
    __cpu_pause();
  }
}

template <typename... Args>
void
until_all_false(Args &&...args)
{
  while ( (args() && ...) ) {
    __cpu_pause();
  }
}

template <typename F, typename... Args>
void
until_backoff(auto cond, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;
  size_t backoff = 1;
  constexpr size_t max_backoff = 1024;

  for ( ;; ) {
    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;

    for ( size_t i = 0; i < backoff; ++i ) {
      __cpu_pause();
    }

    if ( backoff < max_backoff )
      backoff *= 2;
  }
}

template <typename T, typename F, typename... Args>
void
until_in_range(const T &min, const T &max, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res >= min && res <= max )
      break;
    __cpu_pause();
  }
}

template <typename F, typename... Args>
void
until_not(auto cond, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res != cond )
      break;
    __cpu_pause();
  }
}

template <typename T, typename F, typename Fut, typename... Args>
void
until_or_future(const T &cond, Fut &fut, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;

    __cpu_pause();
  }
}

template <typename T, typename Fut>
void
until_or_future(const T &cond, const T &var, Fut &fut)
{
  while ( var != cond ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;
    __cpu_pause();
  }
}

template <typename T, typename Fut>
void
until_or_future(const T &cond, const micron::atomic<T> &var, Fut &fut)
{
  while ( var.__get(micron::memory_order_acquire) != cond ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;
    __cpu_pause();
  }
}

template <typename P, typename Fut>
  requires micron::is_invocable_r_v<bool, P>
void
until_or_future(P pred, Fut &fut)
{
  while ( !pred() ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;
    __cpu_pause();
  }
}

template <typename P, typename Fut, typename... Args>
  requires micron::is_invocable_r_v<bool, P, Args...>
void
until_true_or_future(P pred, Fut &fut, Args &&...args)
{
  while ( !micron::invoke(pred, micron::forward<Args>(args)...) ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;
    __cpu_pause();
  }
}

template <typename F, typename Fut, typename... Args>
bool
until_timeout_or_future(auto cond, micron::duration_d timeout, Fut &fut, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;
  auto start = micron::system_clock<>::now();

  for ( ;; ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      return false;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      return true;

    if ( micron::system_clock<>::now() - start >= timeout )
      return false;

    __cpu_pause();
  }
}

template <typename P, typename Fut>
bool
until_timeout_or_future(micron::duration_d timeout, Fut &fut, P pred)
{
  auto start = micron::system_clock<>::now();

  while ( !pred() ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      return false;

    if ( micron::system_clock<>::now() - start >= timeout )
      return false;

    __cpu_pause();
  }
  return true;
}

template <typename T, typename F, typename Fut, typename Fn, typename... Args>
void
until_with_wait_or_future(const T &cond, Fut &fut, Fn wait, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;

    wait();
  }
}

template <typename F, typename Fut, typename... Args>
void
until_backoff_or_future(auto cond, Fut &fut, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;
  size_t backoff = 1;
  constexpr size_t max_backoff = 1024;

  for ( ;; ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;

    for ( size_t i = 0; i < backoff; ++i ) {
      __cpu_pause();
    }

    if ( backoff < max_backoff )
      backoff *= 2;
  }
}

template <typename T, typename F, typename... Args>
void
until_or_flag(const T &cond, const micron::atomic<bool> &flag, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;

    __cpu_pause();
  }
}

template <typename T>
void
until_or_flag(const T &cond, const T &var, const micron::atomic<bool> &flag)
{
  while ( var != cond ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;
    __cpu_pause();
  }
}

template <typename T>
void
until_or_flag(const T &cond, const micron::atomic<T> &var, const micron::atomic<bool> &flag)
{
  while ( var.__get(micron::memory_order_acquire) != cond ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;
    __cpu_pause();
  }
}

template <typename P>
  requires micron::is_invocable_r_v<bool, P>
void
until_or_flag(P pred, const micron::atomic<bool> &flag)
{
  while ( !pred() ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;
    __cpu_pause();
  }
}

template <typename P, typename... Args>
  requires micron::is_invocable_r_v<bool, P, Args...>
void
until_true_or_flag(P pred, const micron::atomic<bool> &flag, Args &&...args)
{
  while ( !micron::invoke(pred, micron::forward<Args>(args)...) ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;
    __cpu_pause();
  }
}

template <typename F, typename... Args>
bool
until_timeout_or_flag(auto cond, micron::duration_d timeout, const micron::atomic<bool> &flag, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;
  auto start = micron::system_clock<>::now();

  for ( ;; ) {
    if ( flag.__get(micron::memory_order_acquire) )
      return false;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      return true;

    if ( micron::system_clock<>::now() - start >= timeout )
      return false;

    __cpu_pause();
  }
}

template <typename P>
bool
until_timeout_or_flag(micron::duration_d timeout, const micron::atomic<bool> &flag, P pred)
{
  auto start = micron::system_clock<>::now();

  while ( !pred() ) {
    if ( flag.__get(micron::memory_order_acquire) )
      return false;

    if ( micron::system_clock<>::now() - start >= timeout )
      return false;

    __cpu_pause();
  }
  return true;
}

template <typename F, typename... Args>
bool
until_max_iter_or_flag(auto cond, size_t max_iters, const micron::atomic<bool> &flag, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( size_t i = 0; i < max_iters; ++i ) {
    if ( flag.__get(micron::memory_order_acquire) )
      return false;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      return true;

    __cpu_pause();
  }
  return false;
}

template <typename T, typename F, typename Fn, typename... Args>
void
until_with_wait_or_flag(const T &cond, const micron::atomic<bool> &flag, Fn wait, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;

    wait();
  }
}

template <typename T, typename F, typename Cb, typename... Args>
void
until_with_callback_or_flag(const T &cond, const micron::atomic<bool> &flag, Cb cb, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    cb(res);
    if ( res == cond )
      break;

    __cpu_pause();
  }
}

template <typename F, typename... Args>
void
until_backoff_or_flag(auto cond, const micron::atomic<bool> &flag, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;
  size_t backoff = 1;
  constexpr size_t max_backoff = 1024;

  for ( ;; ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;

    for ( size_t i = 0; i < backoff; ++i ) {
      __cpu_pause();
    }

    if ( backoff < max_backoff )
      backoff *= 2;
  }
}

template <typename T, typename F, typename... Args>
void
until_in_range_or_flag(const T &min, const T &max, const micron::atomic<bool> &flag, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res >= min && res <= max )
      break;

    __cpu_pause();
  }
}

template <typename F, typename... Args>
void
until_not_or_flag(auto cond, const micron::atomic<bool> &flag, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res != cond )
      break;

    __cpu_pause();
  }
}

template <typename F, typename... C>
void
until_any_or_flag(const micron::atomic<bool> &flag, F f, C &&...conds)
{
  using ret_t = micron::invoke_result_t<F>;

  for ( ;; ) {
    if ( flag.__get(micron::memory_order_acquire) )
      break;

    ret_t res = f();
    if ( ((res == conds) || ...) )
      break;

    __cpu_pause();
  }
}

template <typename T, typename F, typename Fut, typename... Args>
void
until_or_future_or_flag(const T &cond, Fut &fut, const micron::atomic<bool> &flag, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;

  for ( ;; ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;

    if ( flag.__get(micron::memory_order_acquire) )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;

    __cpu_pause();
  }
}

template <typename T, typename Fut>
void
until_or_future_or_flag(const T &cond, const micron::atomic<T> &var, Fut &fut, const micron::atomic<bool> &flag)
{
  while ( var.__get(micron::memory_order_acquire) != cond ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;

    if ( flag.__get(micron::memory_order_acquire) )
      break;

    __cpu_pause();
  }
}

template <typename P, typename Fut>
  requires micron::is_invocable_r_v<bool, P>
void
until_or_future_or_flag(P pred, Fut &fut, const micron::atomic<bool> &flag)
{
  while ( !pred() ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;

    if ( flag.__get(micron::memory_order_acquire) )
      break;

    __cpu_pause();
  }
}

template <typename F, typename Fut, typename... Args>
bool
until_timeout_or_future_or_flag(auto cond, micron::duration_d timeout, Fut &fut, const micron::atomic<bool> &flag, F f,
                                Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;
  auto start = micron::system_clock<>::now();

  for ( ;; ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      return false;

    if ( flag.__get(micron::memory_order_acquire) )
      return false;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      return true;

    if ( micron::system_clock<>::now() - start >= timeout )
      return false;

    __cpu_pause();
  }
}

template <typename F, typename Fut, typename... Args>
void
until_backoff_or_future_or_flag(auto cond, Fut &fut, const micron::atomic<bool> &flag, F f, Args &&...args)
{
  using ret_t = micron::invoke_result_t<F, Args...>;
  size_t backoff = 1;
  constexpr size_t max_backoff = 1024;

  for ( ;; ) {
    if ( fut.wait_for(micron::duration_d(0)) == micron::future_status::ready )
      break;

    if ( flag.__get(micron::memory_order_acquire) )
      break;

    ret_t res = micron::invoke(f, micron::forward<Args>(args)...);
    if ( res == cond )
      break;

    for ( size_t i = 0; i < backoff; ++i ) {
      __cpu_pause();
    }

    if ( backoff < max_backoff )
      backoff *= 2;
  }
}

template <typename A>
void
until_flag_set(const A &flag)
{
  while ( !flag.__get(micron::memory_order_acquire) ) {
    __cpu_pause();
  }
}

template <typename A>
void
until_flag_clear(const A &flag)
{
  while ( flag.__get(micron::memory_order_acquire) ) {
    __cpu_pause();
  }
}

template <typename A>
bool
until_flag_set_timeout(const A &flag, micron::duration_d timeout)
{
  auto start = micron::system_clock<>::now();

  while ( !flag.__get(micron::memory_order_acquire) ) {
    if ( micron::system_clock<>::now() - start >= timeout )
      return false;
    __cpu_pause();
  }
  return true;
}

template <typename A>
bool
until_flag_clear_timeout(const A &flag, micron::duration_d timeout)
{
  auto start = micron::system_clock<>::now();

  while ( flag.__get(micron::memory_order_acquire) ) {
    if ( micron::system_clock<>::now() - start >= timeout )
      return false;
    __cpu_pause();
  }
  return true;
}

template <typename A, typename Fn>
void
until_flag_set_with_wait(const A &flag, Fn wait)
{
  while ( !flag.__get(micron::memory_order_acquire) ) {
    wait();
  }
}

template <typename A>
void
until_flag_set_backoff(const A &flag)
{
  size_t backoff = 1;
  constexpr size_t max_backoff = 1024;

  while ( !flag.__get(micron::memory_order_acquire) ) {
    for ( size_t i = 0; i < backoff; ++i ) {
      __cpu_pause();
    }

    if ( backoff < max_backoff )
      backoff *= 2;
  }
}

template <typename... F>
void
until_any_flag_set(const F &...flags)
{
  while ( !(flags.__get(micron::memory_order_acquire) || ...) ) {
    __cpu_pause();
  }
}

template <typename... F>
void
until_all_flags_set(const F &...flags)
{
  while ( !(flags.__get(micron::memory_order_acquire) && ...) ) {
    __cpu_pause();
  }
}

template <typename... F>
void
until_any_flag_clear(const F &...flags)
{
  while ( (flags.__get(micron::memory_order_acquire) && ...) ) {
    __cpu_pause();
  }
}

template <typename... F>
void
until_all_flags_clear(const F &...flags)
{
  while ( (flags.__get(micron::memory_order_acquire) || ...) ) {
    __cpu_pause();
  }
}

};     // namespace micron
