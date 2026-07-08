//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../chrono.hpp"
#include "../defs.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../mutex/locks.hpp"      // future.hpp uses lock_guard but relies on its includer to provide it
#include "../types.hpp"

#include "futex.hpp"
#include "future.hpp"      // reuse future_status / future_errc / broken_promise

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// futex_shared_state<T> + futex_future<T> / futex_promise<T>
//
// futex backed alternative to future.hpp's spin wait shared_state

// NOTE: separate file so we don't mess with include spaghetti

namespace micron
{

template<class T> class futex_future;
template<class T> class futex_promise;

template<class T> class futex_shared_state
{
  mutable micron::atomic_token<u32> __rdy{ 0 };      // 0 = pending, 1 = ready (the futex word)
  micron::atomic_token<usize> refs;
  bool has_exception;
  bool retrieved;
  const char *exc_msg;

  union storage_t {
    char dummy;
    T value;

    storage_t() : dummy(0) { }

    ~storage_t() { }
  } storage;

  micron::atomic_token<usize> __wstate{ 0 };
  void (*__wresume)(usize) = nullptr;      // set before arming, read after the exchange sees the token

  void
  __wake_waiter() noexcept
  {
    usize __old = __wstate.swap(1, memory_order_acq_rel);
    if ( __old > 1 ) __wresume(__old);
  }

  [[gnu::always_inline]] future_status
  __wait_impl(const timespec_t *__abs) const
  {
    while ( __rdy.get(memory_order_acquire) == 0 ) {
      auto __r = micron::__futex(__rdy.ptr(), futex_wait_bitset | futex_private_flag, 0u, const_cast<timespec_t *>(__abs), nullptr,
                                 0xffffffffu);                      // FUTEX_BITSET_MATCH_ANY
      if ( __r == -110 ) break;                                     // ETIMEDOUT (deadline reached)
      if ( __r < 0 && __r != -11 && __r != -4 && __r != -110 )      // not EAGAIN/EINTR/ETIMEDOUT -> bail
        break;
    }
    return __rdy.get(memory_order_acquire) ? future_status::ready : future_status::timeout;
  }

public:
  futex_shared_state() : refs(1), has_exception(false), retrieved(false), exc_msg(nullptr) { }

  ~futex_shared_state()
  {
    if ( __rdy.get(memory_order_acquire) == 1 && !has_exception ) storage.value.~T();
  }

  futex_shared_state(const futex_shared_state &) = delete;
  futex_shared_state &operator=(const futex_shared_state &) = delete;

  void
  __acquire() noexcept
  {
    refs.fetch_add(1, memory_order::relaxed);
  }

  bool
  __release() noexcept
  {
    return refs.sub_fetch(1, memory_order::acq_rel) == 0;
  }

  void
  wait() const
  {
    while ( __rdy.get(memory_order_acquire) == 0 ) micron::wait_futex(__rdy.ptr(), 0u);
  }

  future_status
  wait_for(fduration_t __ms) const
  {
    if ( __rdy.get(memory_order_acquire) ) return future_status::ready;
    if ( __ms <= 0.0 ) return future_status::timeout;
    timespec_t __ts{};
    micron::clock_gettime(static_cast<clockid_t>(clock_monotonic), __ts);
    const time64_t __sec = static_cast<time64_t>(__ms / 1000.0);
    const slong_t __nsec = static_cast<slong_t>((__ms - static_cast<f64>(__sec) * 1000.0) * 1.0e6);
    __ts.tv_sec += __sec;
    __ts.tv_nsec += __nsec;
    if ( __ts.tv_nsec >= 1'000'000'000L ) {
      __ts.tv_sec += 1;
      __ts.tv_nsec -= 1'000'000'000L;
    }
    return __wait_impl(&__ts);
  }

  template<class... Args>
  void
  set_value(Args &&...args)
  {
    ::new (static_cast<void *>(&storage.value)) T(micron::forward<Args>(args)...);
    has_exception = false;
    micron::release_futex(__rdy.ptr(), 1u, 0x7fffffffu);      // release-store ready + wake all thread waiters
    __wake_waiter();                                          // resume a coroutine waiter, if any
  }

  void
  set_exception(const char *msg) noexcept
  {
    if ( __rdy.get(memory_order_acquire) == 1 ) return;
    exc_msg = msg ? msg : "broken promise";
    has_exception = true;
    micron::release_futex(__rdy.ptr(), 1u, 0x7fffffffu);
    __wake_waiter();
  }

  bool
  __arm(usize __tok, void (*__res)(usize)) noexcept
  {
    __wresume = __res;
    usize __exp = 0;
    if ( __wstate.compare_exchange_strong(__exp, __tok, memory_order_acq_rel, memory_order_acquire) ) return false;
    return true;
  }

  T
  get()
  {
    wait();
    if ( retrieved ) exc<except::future_error>("future already retrieved");
    retrieved = true;
    if ( has_exception ) {
#if !defined(__micron_freestanding) || defined(__micron_eh)
      throw micron::broken_promise{ exc_msg };
#else
      micron::exc<except::future_error>(exc_msg);
#endif
    }
    return micron::move(storage.value);
  }

  bool
  is_ready() const noexcept
  {
    return __rdy.get(memory_order_acquire) == 1;
  }

  bool
  valid() const noexcept
  {
    return true;
  }
};

// void
template<> class futex_shared_state<void>
{
  mutable micron::atomic_token<u32> __rdy{ 0 };
  micron::atomic_token<usize> refs;
  bool has_exception;
  bool retrieved;
  const char *exc_msg;

  micron::atomic_token<usize> __wstate{ 0 };
  void (*__wresume)(usize) = nullptr;

  void
  __wake_waiter() noexcept
  {
    usize __old = __wstate.swap(1, memory_order_acq_rel);
    if ( __old > 1 ) __wresume(__old);
  }

public:
  futex_shared_state() : refs(1), has_exception(false), retrieved(false), exc_msg(nullptr) { }

  futex_shared_state(const futex_shared_state &) = delete;
  futex_shared_state &operator=(const futex_shared_state &) = delete;

  void
  __acquire() noexcept
  {
    refs.fetch_add(1, memory_order::relaxed);
  }

  bool
  __release() noexcept
  {
    return refs.sub_fetch(1, memory_order::acq_rel) == 0;
  }

  void
  wait() const
  {
    while ( __rdy.get(memory_order_acquire) == 0 ) micron::wait_futex(__rdy.ptr(), 0u);
  }

  future_status
  wait_for(fduration_t __ms) const
  {
    if ( __rdy.get(memory_order_acquire) ) return future_status::ready;
    if ( __ms <= 0.0 ) return future_status::timeout;
    timespec_t __ts{};
    micron::clock_gettime(static_cast<clockid_t>(clock_monotonic), __ts);
    const time64_t __sec = static_cast<time64_t>(__ms / 1000.0);
    const slong_t __nsec = static_cast<slong_t>((__ms - static_cast<f64>(__sec) * 1000.0) * 1.0e6);
    __ts.tv_sec += __sec;
    __ts.tv_nsec += __nsec;
    if ( __ts.tv_nsec >= 1'000'000'000L ) {
      __ts.tv_sec += 1;
      __ts.tv_nsec -= 1'000'000'000L;
    }
    while ( __rdy.get(memory_order_acquire) == 0 ) {
      auto __r = micron::__futex(__rdy.ptr(), futex_wait_bitset | futex_private_flag, 0u, &__ts, nullptr, 0xffffffffu);
      if ( __r == -110 ) break;
      if ( __r < 0 && __r != -11 && __r != -4 && __r != -110 ) break;
    }
    return __rdy.get(memory_order_acquire) ? future_status::ready : future_status::timeout;
  }

  void
  set_value()
  {
    has_exception = false;
    micron::release_futex(__rdy.ptr(), 1u, 0x7fffffffu);
    __wake_waiter();
  }

  void
  set_exception(const char *msg) noexcept
  {
    if ( __rdy.get(memory_order_acquire) == 1 ) return;
    exc_msg = msg ? msg : "broken promise";
    has_exception = true;
    micron::release_futex(__rdy.ptr(), 1u, 0x7fffffffu);
    __wake_waiter();
  }

  bool
  __arm(usize __tok, void (*__res)(usize)) noexcept
  {
    __wresume = __res;
    usize __exp = 0;
    if ( __wstate.compare_exchange_strong(__exp, __tok, memory_order_acq_rel, memory_order_acquire) ) return false;
    return true;
  }

  void
  get()
  {
    wait();
    if ( retrieved ) exc<except::future_error>("future already retrieved");
    retrieved = true;
    if ( has_exception ) {
#if !defined(__micron_freestanding) || defined(__micron_eh)
      throw micron::broken_promise{ exc_msg };
#else
      micron::exc<except::future_error>(exc_msg);
#endif
    }
  }

  bool
  is_ready() const noexcept
  {
    return __rdy.get(memory_order_acquire) == 1;
  }

  bool
  valid() const noexcept
  {
    return true;
  }
};

template<class T>
class [[nodiscard("a dropped micron::futex_future discards the result and broken-promise "
                  "detection")]] futex_future
{
  futex_shared_state<T> *state = nullptr;
  friend class futex_promise<T>;

  explicit futex_future(futex_shared_state<T> *__s) noexcept : state(__s) { }

public:
  futex_future() noexcept = default;
  futex_future(const futex_future &) = delete;
  futex_future &operator=(const futex_future &) = delete;

  futex_future(futex_future &&__o) noexcept : state(__o.state) { __o.state = nullptr; }

  futex_future &
  operator=(futex_future &&__o) noexcept
  {
    if ( this != &__o ) {
      if ( state && state->__release() ) delete state;
      state = __o.state;
      __o.state = nullptr;
    }
    return *this;
  }

  ~futex_future()
  {
    if ( state && state->__release() ) delete state;
  }

  // internal: the shared state (for the coroutine co_await adapter in the engine layer)
  futex_shared_state<T> *
  __shared() const noexcept
  {
    return state;
  }

  T
  get()
  {
    if ( !state ) [[unlikely]]
      exc<except::future_error>("micron::futex_future: get() on a future with no shared state");
    return state->get();
  }

  bool
  valid() const noexcept
  {
    return state != nullptr;
  }

  void
  wait() const
  {
    if ( !state ) [[unlikely]]
      exc<except::future_error>("micron::futex_future: wait() on a future with no shared state");
    state->wait();
  }

  future_status
  wait_for(fduration_t __t) const
  {
    if ( !state ) [[unlikely]]
      exc<except::future_error>("micron::futex_future: wait_for() on a future with no shared state");
    return state->wait_for(__t);
  }
};

template<> class [[nodiscard("a dropped micron::futex_future discards broken-promise detection")]] futex_future<void>
{
  futex_shared_state<void> *state = nullptr;
  friend class futex_promise<void>;

  explicit futex_future(futex_shared_state<void> *__s) noexcept : state(__s) { }

public:
  futex_future() noexcept = default;
  futex_future(const futex_future &) = delete;
  futex_future &operator=(const futex_future &) = delete;

  futex_future(futex_future &&__o) noexcept : state(__o.state) { __o.state = nullptr; }

  futex_future &
  operator=(futex_future &&__o) noexcept
  {
    if ( this != &__o ) {
      if ( state && state->__release() ) delete state;
      state = __o.state;
      __o.state = nullptr;
    }
    return *this;
  }

  ~futex_future()
  {
    if ( state && state->__release() ) delete state;
  }

  futex_shared_state<void> *
  __shared() const noexcept
  {
    return state;
  }

  void
  get()
  {
    if ( !state ) [[unlikely]]
      exc<except::future_error>("micron::futex_future: get() on a future with no shared state");
    state->get();
  }

  bool
  valid() const noexcept
  {
    return state != nullptr;
  }

  void
  wait() const
  {
    if ( !state ) [[unlikely]]
      exc<except::future_error>("micron::futex_future: wait() on a future with no shared state");
    state->wait();
  }

  future_status
  wait_for(fduration_t __t) const
  {
    if ( !state ) [[unlikely]]
      exc<except::future_error>("micron::futex_future: wait_for() on a future with no shared state");
    return state->wait_for(__t);
  }
};

template<class T> class futex_promise
{
  futex_shared_state<T> *state;

public:
  futex_promise() : state(new futex_shared_state<T>()) { }

  futex_promise(const futex_promise &) = delete;
  futex_promise &operator=(const futex_promise &) = delete;

  futex_promise(futex_promise &&__o) noexcept : state(__o.state) { __o.state = nullptr; }

  futex_promise &
  operator=(futex_promise &&__o) noexcept
  {
    if ( this != &__o ) {
      __abandon();
      state = __o.state;
      __o.state = nullptr;
    }
    return *this;
  }

  ~futex_promise() { __abandon(); }

  futex_future<T>
  get_future()
  {
    state->__acquire();
    return futex_future<T>{ state };
  }

  void
  set_value(const T &__v)
  {
    state->set_value(__v);
  }

  void
  set_value(T &&__v)
  {
    state->set_value(micron::move(__v));
  }

  void
  set_exception(const char *__msg = "user exception")
  {
    state->set_exception(__msg);
  }

private:
  void
  __abandon() noexcept
  {
    if ( state ) {
      if ( !state->is_ready() ) state->set_exception("broken promise");
      if ( state->__release() ) delete state;
      state = nullptr;
    }
  }
};

template<> class futex_promise<void>
{
  futex_shared_state<void> *state;

public:
  futex_promise() : state(new futex_shared_state<void>()) { }

  futex_promise(const futex_promise &) = delete;
  futex_promise &operator=(const futex_promise &) = delete;

  futex_promise(futex_promise &&__o) noexcept : state(__o.state) { __o.state = nullptr; }

  futex_promise &
  operator=(futex_promise &&__o) noexcept
  {
    if ( this != &__o ) {
      __abandon();
      state = __o.state;
      __o.state = nullptr;
    }
    return *this;
  }

  ~futex_promise() { __abandon(); }

  futex_future<void>
  get_future()
  {
    state->__acquire();
    return futex_future<void>{ state };
  }

  void
  set_value()
  {
    state->set_value();
  }

  void
  set_exception(const char *__msg = "user exception")
  {
    state->set_exception(__msg);
  }

private:
  void
  __abandon() noexcept
  {
    if ( state ) {
      if ( !state->is_ready() ) state->set_exception("broken promise");
      if ( state->__release() ) delete state;
      state = nullptr;
    }
  }
};

}      // namespace micron
