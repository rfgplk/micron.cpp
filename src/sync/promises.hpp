//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{
class broken_promise
{
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
