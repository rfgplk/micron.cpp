//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../memory/actions.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{

enum class future_errc { broken_promise = 1, future_already_retrieved, promise_already_satisfied, no_state };

enum class future_status { ready, timeout, deferred };

enum class launch { async = 1, deferred = 2 };

class broken_promise
{
  const char *__msg;

public:
  broken_promise() noexcept : __msg("broken promise") { }

  explicit broken_promise(const char *m) noexcept : __msg(m ? m : "broken promise") { }

  const char *
  what() const noexcept
  {
    return __msg;
  }
};

template<typename T> class shared_state
{
  mutable mutex mtx;
  atomic_token<usize> refs;
  bool ready;
  bool retrieved;
  bool has_exception;
  const char *exc_msg;

  union storage_t {
    char dummy;
    T value;

    storage_t() : dummy(0) { }

    ~storage_t() { }
  } storage;

public:
  shared_state() : refs(1), ready(false), retrieved(false), has_exception(false), exc_msg(nullptr) { }

  ~shared_state()
  {
    if ( ready && !has_exception ) {
      storage.value.~T();
    }
  }

  shared_state(const shared_state &) = delete;
  shared_state &operator=(const shared_state &) = delete;

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
    for ( ;; ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready ) return;
      }
      __cpu_pause();
    }
  }

  future_status
  wait_for(fduration_t timeout) const
  {
    auto start = micron::system_clock<>::now();
    future_status r;
    for ( ;; ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready or (micron::system_clock<>::now() - start >= timeout) ) {
          r = ready ? future_status::ready : future_status::timeout;
          break;
        }
      }
      __cpu_pause();
    }
    return r;
  }

  future_status wait_until();

  template<typename... Args>
  void
  set_value(Args &&...args)
  {
    lock_guard<mutex> lck(mtx);
    if ( ready ) {
      exc<except::future_error>("");
    }
    new (&storage.value) T(micron::forward<Args>(args)...);
    ready = true;
    has_exception = false;
  }

  void
  set_exception(const char *msg) noexcept
  {
    lock_guard<mutex> lck(mtx);
    if ( ready ) return;
    exc_msg = msg ? msg : "broken promise";
    has_exception = true;
    ready = true;
  }

  T
  get()
  {
    wait();

    lock_guard<mutex> lck(mtx);
    if ( retrieved ) {
      exc<except::future_error>("");
    }
    retrieved = true;

    if ( has_exception ) {
#if !defined(__micron_freestanding) || defined(__micron_eh)
      throw micron::broken_promise{ exc_msg };
#else
      // freestanding without exceptions (-k): cannot unwind; trap on a broken promise instead
      micron::exc<except::future_error>(exc_msg);
#endif
    }

    return micron::move(storage.value);
  }

  bool
  is_ready() const
  {
    lock_guard<mutex> lck(mtx);
    return ready;
  }

  bool
  valid() const
  {
    return true;
  }
};

template<> class shared_state<void>
{
  mutable mutex mtx;
  atomic_token<usize> refs;
  bool ready;
  bool retrieved;
  bool has_exception;
  const char *exc_msg;

public:
  shared_state() : refs(1), ready(false), retrieved(false), has_exception(false), exc_msg(nullptr) { }

  shared_state(const shared_state &) = delete;
  shared_state &operator=(const shared_state &) = delete;

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
    while ( true ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready ) return;
      }
      __cpu_pause();
    }
  }

  future_status
  wait_for(fduration_t timeout) const
  {
    auto start = micron::system_clock<>::now();
    future_status r;
    for ( ;; ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready or (micron::system_clock<>::now() - start >= timeout) ) {
          r = ready ? future_status::ready : future_status::timeout;
          break;
        }
      }
      __cpu_pause();
    }
    return r;
  }

  future_status wait_until();

  void
  set_value()
  {
    lock_guard<mutex> lck(mtx);
    if ( ready ) {
      exc<except::future_error>("");
    }
    ready = true;
  }

  void
  set_exception(const char *msg) noexcept
  {
    lock_guard<mutex> lck(mtx);
    if ( ready ) return;
    exc_msg = msg ? msg : "broken promise";
    has_exception = true;
    ready = true;
  }

  void
  get()
  {
    wait();

    lock_guard<mutex> lck(mtx);
    if ( retrieved ) {
      exc<except::future_error>("");
    }
    retrieved = true;
    if ( has_exception ) {
#if !defined(__micron_freestanding) || defined(__micron_eh)
      throw micron::broken_promise{ exc_msg };
#else
      // freestanding without exceptions (-k): cannot unwind; trap on a broken promise instead
      micron::exc<except::future_error>(exc_msg);
#endif
    }
  }

  bool
  is_ready() const
  {
    lock_guard<mutex> lck(mtx);
    return ready;
  }

  bool
  valid() const
  {
    return true;
  }
};

template<typename T> class shared_state<T &>
{
  mutable mutex mtx;
  atomic_token<usize> refs;
  bool ready;
  bool retrieved;
  bool has_exception;
  const char *exc_msg;
  T *ptr;

public:
  shared_state() : refs(1), ready(false), retrieved(false), has_exception(false), exc_msg(nullptr), ptr(nullptr) { }

  shared_state(const shared_state &) = delete;
  shared_state &operator=(const shared_state &) = delete;

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
    while ( true ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready ) return;
      }
      __cpu_pause();
    }
  }

  future_status
  wait_for(fduration_t timeout) const
  {
    auto start = micron::system_clock<>::now();
    future_status r;
    for ( ;; ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready or (micron::system_clock<>::now() - start >= timeout) ) {
          r = ready ? future_status::ready : future_status::timeout;
          break;
        }
      }
      __cpu_pause();
    }
    return r;
  }

  future_status wait_until();

  void
  set_value(T &ref)
  {
    lock_guard<mutex> lck(mtx);
    if ( ready ) {
      exc<except::future_error>("");
    }
    ptr = &ref;
    ready = true;
  }

  void
  set_exception(const char *msg) noexcept
  {
    lock_guard<mutex> lck(mtx);
    if ( ready ) return;
    exc_msg = msg ? msg : "broken promise";
    has_exception = true;
    ready = true;
  }

  T &
  get()
  {
    wait();

    lock_guard<mutex> lck(mtx);
    if ( retrieved ) {
      exc<except::future_error>("");
    }
    retrieved = true;
    if ( has_exception ) {
#if !defined(__micron_freestanding) || defined(__micron_eh)
      throw micron::broken_promise{ exc_msg };
#else
      // freestanding without exceptions (-k): cannot unwind; trap on a broken promise instead
      micron::exc<except::future_error>(exc_msg);
#endif
    }

    return *ptr;
  }

  bool
  is_ready() const
  {
    lock_guard<mutex> lck(mtx);
    return ready;
  }

  bool
  valid() const
  {
    return true;
  }
};

template<typename T> class promise;
template<typename T> class future;

template<typename T> class future
{
  shared_state<T> *state;

  template<typename U> friend class promise;

  explicit future(shared_state<T> *s) : state(s) { }

public:
  future() noexcept : state(nullptr) { }

  future(const future &) = delete;
  future &operator=(const future &) = delete;

  future(future &&other) noexcept : state(other.state) { other.state = nullptr; }

  future &
  operator=(future &&other) noexcept
  {
    if ( this != &other ) {
      if ( state && state->__release() ) {
        delete state;
      }
      state = other.state;
      other.state = nullptr;
    }
    return *this;
  }

  ~future()
  {
    if ( state && state->__release() ) {
      delete state;
    }
  }

  T
  get()
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
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
    if ( !state ) {
      exc<except::future_error>("");
    }
    state->wait();
  }

  future_status
  wait_for(fduration_t timeout_duration) const
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    return state->wait_for(timeout_duration);
  }

  future_status wait_until();
};

template<> class future<void>
{
  shared_state<void> *state;

  template<typename U> friend class promise;

  explicit future(shared_state<void> *s) : state(s) { }

public:
  future() noexcept : state(nullptr) { }

  future(const future &) = delete;
  future &operator=(const future &) = delete;

  future(future &&other) noexcept : state(other.state) { other.state = nullptr; }

  future &
  operator=(future &&other) noexcept
  {
    if ( this != &other ) {
      if ( state && state->__release() ) {
        delete state;
      }
      state = other.state;
      other.state = nullptr;
    }
    return *this;
  }

  ~future()
  {
    if ( state && state->__release() ) {
      delete state;
    }
  }

  void
  get()
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
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
    if ( !state ) {
      exc<except::future_error>("");
    }
    state->wait();
  }

  future_status
  wait_for(fduration_t timeout_duration) const
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    return state->wait_for(timeout_duration);
  }

  future_status wait_until();
};

template<typename T> class future<T &>
{
  shared_state<T &> *state;

  template<typename U> friend class promise;

  explicit future(shared_state<T &> *s) : state(s) { }

public:
  future() noexcept : state(nullptr) { }

  future(const future &) = delete;
  future &operator=(const future &) = delete;

  future(future &&other) noexcept : state(other.state) { other.state = nullptr; }

  future &
  operator=(future &&other) noexcept
  {
    if ( this != &other ) {
      if ( state && state->__release() ) {
        delete state;
      }
      state = other.state;
      other.state = nullptr;
    }
    return *this;
  }

  ~future()
  {
    if ( state && state->__release() ) {
      delete state;
    }
  }

  T &
  get()
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
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
    if ( !state ) {
      exc<except::future_error>("");
    }
    state->wait();
  }

  future_status
  wait_for(fduration_t timeout_duration) const
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    return state->wait_for(timeout_duration);
  }

  future_status wait_until();
};

};      // namespace micron
