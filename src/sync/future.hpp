//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/actions.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{

enum class future_errc { broken_promise = 1, future_already_retrieved, promise_already_satisfied, no_state };

enum class future_status { ready, timeout, deferred };

enum class launch { async = 1, deferred = 2 };

template <typename T> class shared_state
{
  mutable mutex mtx;
  bool ready;
  bool retrieved;
  bool has_exception;

  union storage_t {
    char dummy;
    T value;

    storage_t() : dummy(0) {}
    ~storage_t() {}
  } storage;

public:
  shared_state() : ready(false), retrieved(false), has_exception(false) {}

  ~shared_state()
  {
    if ( ready && !has_exception ) {
      storage.value.~T();
    }
  }

  shared_state(const shared_state &) = delete;
  shared_state &operator=(const shared_state &) = delete;

  void
  wait() const
  {
    for ( ;; ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready )
          return;
      }
      __cpu_pause();
    }
  }

  future_status
  wait_for(micron::duration_d timeout) const
  {

    auto start = micron::system_clock<>::now();
    for ( ;; ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready or (micron::system_clock<>::now() - start >= timeout) )
          break;
      }
      __cpu_pause();
    }
    return ready ? future_status::ready : future_status::timeout;
  }

  future_status wait_until();

  template <typename... Args>
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

  T
  get()
  {
    wait();

    lock_guard<mutex> lck(mtx);
    if ( retrieved ) {
      exc<except::future_error>("");
    }
    retrieved = true;

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

template <> class shared_state<void>
{
  mutable mutex mtx;
  bool ready;
  bool retrieved;

public:
  shared_state() : ready(false), retrieved(false) {}

  shared_state(const shared_state &) = delete;
  shared_state &operator=(const shared_state &) = delete;

  void
  wait() const
  {
    while ( true ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready )
          return;
      }
      __cpu_pause();
    }
  }

  future_status
  wait_for(micron::duration_d timeout) const
  {
    auto start = micron::system_clock<>::now();
    for ( ;; ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready or (micron::system_clock<>::now() - start >= timeout) )
          break;
      }
      __cpu_pause();
    }
    return ready ? future_status::ready : future_status::timeout;
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
  get()
  {
    wait();

    lock_guard<mutex> lck(mtx);
    if ( retrieved ) {
      exc<except::future_error>("");
    }
    retrieved = true;
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

template <typename T> class shared_state<T &>
{
  mutable mutex mtx;
  bool ready;
  bool retrieved;
  T *ptr;

public:
  shared_state() : ready(false), retrieved(false), ptr(nullptr) {}

  shared_state(const shared_state &) = delete;
  shared_state &operator=(const shared_state &) = delete;

  void
  wait() const
  {
    while ( true ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready )
          return;
      }
      __cpu_pause();
    }
  }

  future_status
  wait_for(micron::duration_d timeout) const
  {

    auto start = micron::system_clock<>::now();
    for ( ;; ) {
      {
        lock_guard<mutex> lck(mtx);
        if ( ready or (micron::system_clock<>::now() - start >= timeout) )
          break;
      }
      __cpu_pause();
    }
    return ready ? future_status::ready : future_status::timeout;
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
  T &
  get()
  {
    wait();

    lock_guard<mutex> lck(mtx);
    if ( retrieved ) {
      exc<except::future_error>("");
    }
    retrieved = true;

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

template <typename T> class promise;
template <typename T> class future;

template <typename T> class future
{
  shared_state<T> *state;

  template <typename U> friend class promise;

  explicit future(shared_state<T> *s) : state(s) {}

public:
  future() noexcept : state(nullptr) {}

  future(const future &) = delete;
  future &operator=(const future &) = delete;

  future(future &&other) noexcept : state(other.state) { other.state = nullptr; }

  future &
  operator=(future &&other) noexcept
  {
    if ( this != &other ) {
      if ( state ) {
        delete state;
      }
      state = other.state;
      other.state = nullptr;
    }
    return *this;
  }

  ~future()
  {
    if ( state ) {
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
  wait_for(micron::duration_d timeout_duration) const
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    return state->wait_for(timeout_duration);
  }

  future_status wait_until();
};

template <> class future<void>
{
  shared_state<void> *state;

  template <typename U> friend class promise;

  explicit future(shared_state<void> *s) : state(s) {}

public:
  future() noexcept : state(nullptr) {}

  future(const future &) = delete;
  future &operator=(const future &) = delete;

  future(future &&other) noexcept : state(other.state) { other.state = nullptr; }

  future &
  operator=(future &&other) noexcept
  {
    if ( this != &other ) {
      if ( state ) {
        delete state;
      }
      state = other.state;
      other.state = nullptr;
    }
    return *this;
  }

  ~future()
  {
    if ( state ) {
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
  wait_for(micron::duration_d timeout_duration) const
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    return state->wait_for(timeout_duration);
  }

  future_status wait_until();
};

template <typename T> class future<T &>
{
  shared_state<T &> *state;

  template <typename U> friend class promise;

  explicit future(shared_state<T &> *s) : state(s) {}

public:
  future() noexcept : state(nullptr) {}

  future(const future &) = delete;
  future &operator=(const future &) = delete;

  future(future &&other) noexcept : state(other.state) { other.state = nullptr; }

  future &
  operator=(future &&other) noexcept
  {
    if ( this != &other ) {
      if ( state ) {
        delete state;
      }
      state = other.state;
      other.state = nullptr;
    }
    return *this;
  }

  ~future()
  {
    if ( state ) {
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
  wait_for(micron::duration_d timeout_duration) const
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    return state->wait_for(timeout_duration);
  }

  future_status wait_until();
};

template <typename T> class promise
{
  shared_state<T> *state;
  bool future_retrieved;

public:
  promise() : state(new shared_state<T>()), future_retrieved(false) {}

  promise(const promise &) = delete;
  promise &operator=(const promise &) = delete;

  promise(promise &&other) noexcept : state(other.state), future_retrieved(other.future_retrieved)
  {
    other.state = nullptr;
    other.future_retrieved = false;
  }

  promise &
  operator=(promise &&other) noexcept
  {
    if ( this != &other ) {
      if ( state && !state->is_ready() ) {
        // broken promise
      }
      state = other.state;
      future_retrieved = other.future_retrieved;
      other.state = nullptr;
      other.future_retrieved = false;
    }
    return *this;
  }

  ~promise()
  {
    if ( state && !state->is_ready() ) {
      // broken promise
    }
  }

  future<T>
  get_future()
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    if ( future_retrieved ) {
      exc<except::future_error>("");
    }
    future_retrieved = true;
    return future<T>(state);
  }

  void
  set_value(const T &value)
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    state->set_value(value);
  }

  void
  set_value(T &&value)
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    state->set_value(micron::move(value));
  }
  void
  set_value_at_thread_exit(const T &value)
  {
    set_value(value);
  }

  void
  set_value_at_thread_exit(T &&value)
  {
    set_value(micron::move(value));
  }
};

template <> class promise<void>
{
  shared_state<void> *state;
  bool future_retrieved;

public:
  promise() : state(new shared_state<void>()), future_retrieved(false) {}

  promise(const promise &) = delete;
  promise &operator=(const promise &) = delete;

  promise(promise &&other) noexcept : state(other.state), future_retrieved(other.future_retrieved)
  {
    other.state = nullptr;
    other.future_retrieved = false;
  }

  promise &
  operator=(promise &&other) noexcept
  {
    if ( this != &other ) {
      if ( state && !state->is_ready() ) {
        // broken promise
      }
      state = other.state;
      future_retrieved = other.future_retrieved;
      other.state = nullptr;
      other.future_retrieved = false;
    }
    return *this;
  }

  ~promise()
  {
    if ( state && !state->is_ready() ) {
      // broken promise
    }
  }

  future<void>
  get_future()
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    if ( future_retrieved ) {
      exc<except::future_error>("");
    }
    future_retrieved = true;
    return future<void>(state);
  }

  void
  set_value()
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    state->set_value();
  }
  void
  set_value_at_thread_exit()
  {
    set_value();
  }
};

template <typename T> class promise<T &>
{
  shared_state<T &> *state;
  bool future_retrieved;

public:
  promise() : state(new shared_state<T &>()), future_retrieved(false) {}

  promise(const promise &) = delete;
  promise &operator=(const promise &) = delete;

  promise(promise &&other) noexcept : state(other.state), future_retrieved(other.future_retrieved)
  {
    other.state = nullptr;
    other.future_retrieved = false;
  }

  promise &
  operator=(promise &&other) noexcept
  {
    if ( this != &other ) {
      if ( state && !state->is_ready() ) {
        // broken promise
      }
      state = other.state;
      future_retrieved = other.future_retrieved;
      other.state = nullptr;
      other.future_retrieved = false;
    }
    return *this;
  }

  ~promise()
  {
    if ( state && !state->is_ready() ) {
      // broken promise
    }
  }

  future<T &>
  get_future()
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    if ( future_retrieved ) {
      exc<except::future_error>("");
    }
    future_retrieved = true;
    return future<T &>(state);
  }

  void
  set_value(T &value)
  {
    if ( !state ) {
      exc<except::future_error>("");
    }
    state->set_value(value);
  }
  void
  set_value_at_thread_exit(T &value)
  {
    set_value(value);
  }
};

};
