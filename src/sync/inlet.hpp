//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"

#include "../mutex/locks.hpp"

// inspired by Cilk inlets

namespace micron
{

class base_inlet_t
{
public:
  virtual ~base_inlet_t() = default;
  virtual const char *tag() const noexcept = 0;
  virtual bool locked() const noexcept = 0;
  virtual void reset() = 0;

protected:
  base_inlet_t() = default;
};

template <typename T, class Mtx = micron::mutex> class inlet : public base_inlet_t
{
  T value;
  mutable Mtx mtx;
  const char *_tag;

public:
  class handle_t
  {
    inlet *src;

    explicit handle_t(inlet *s) noexcept : src(s) {}
    friend class inlet;

  public:
    ~handle_t()
    {
      if ( src )
        src->mtx.unlock();
    }

    handle_t(const handle_t &) = delete;

    handle_t(handle_t &&o) noexcept : src(o.src) { o.src = nullptr; }

    handle_t &operator=(const handle_t &) = delete;
    handle_t &operator=(handle_t &&) = delete;

    T &
    operator*() noexcept
    {
      return src->value;
    }

    const T &
    operator*() const noexcept
    {
      return src->value;
    }

    T *
    operator->() noexcept
    {
      return &src->value;
    }

    const T *
    operator->() const noexcept
    {
      return &src->value;
    }

    T &
    get() noexcept
    {
      return src->value;
    }

    const T &
    get() const noexcept
    {
      return src->value;
    }

    void
    set(const T &v)
    {
      src->value = v;
    }

    void
    set(T &&v)
    {
      src->value = static_cast<T &&>(v);
    }
  };

  ~inlet() override = default;

  inlet() noexcept(noexcept(T())) : value(), _tag("") {}

  explicit inlet(const char *tag_str) noexcept(noexcept(T())) : value(), _tag(tag_str) {}

  explicit inlet(const T &init, const char *tag_str = "") : value(init), _tag(tag_str) {}

  explicit inlet(T &&init, const char *tag_str = "") : value(static_cast<T &&>(init)), _tag(tag_str) {}

  inlet(const inlet &) = delete;
  inlet(inlet &&) = delete;
  inlet &operator=(const inlet &) = delete;
  inlet &operator=(inlet &&) = delete;

  const char *
  tag() const noexcept override
  {
    return _tag;
  }

  bool
  locked() const noexcept override
  {
    return mtx.is_locked();
  }

  void
  reset() override
  {
    mtx.lock();
    value = T{};
    mtx.unlock();
  }

  handle_t
  access()
  {
    mtx.lock();
    return handle_t(this);
  }

  bool
  try_access(handle_t &out)
  {
    if ( mtx.try_lock() ) {
      out = handle_t(this);
      return true;
    }
    return false;
  }

  template <typename Fn>
  auto
  apply(Fn &&fn) -> decltype(fn(value))
  {
    mtx.lock();

    struct guard_t {
      Mtx &m;

      ~guard_t() { m.unlock(); }
    } g{ mtx };

    return fn(value);
  }

  T
  load() const
  {
    const_cast<Mtx &>(mtx).lock();
    T tmp = value;
    const_cast<Mtx &>(mtx).unlock();
    return tmp;
  }

  void
  store(const T &v)
  {
    mtx.lock();
    value = v;
    mtx.unlock();
  }

  void
  store(T &&v)
  {
    mtx.lock();
    value = static_cast<T &&>(v);
    mtx.unlock();
  }

  inlet &
  operator=(const T &v)
  {
    store(v);
    return *this;
  }

  inlet &
  operator=(T &&v)
  {
    store(static_cast<T &&>(v));
    return *this;
  }

  explicit
  operator T() const
  {
    return load();
  }
};

template <typename T> using mutex_inlet = inlet<T, micron::mutex>;

template <typename T> using spin_inlet = inlet<T, micron::spin_lock>;

struct queuing_mutex_adapter {
  queuing_mutex mtx;
  queuing_mutex::node_type node;

  void
  lock()
  {
    mtx(node);
  }

  bool
  try_lock()
  {
    return mtx.try_lock(node);
  }

  void
  unlock()
  {
    mtx.unlock(node);
  }

  bool
  is_locked() const noexcept
  {
    return mtx.is_locked();
  }
};

template <typename T> using queuing_inlet = inlet<T, queuing_mutex_adapter>;
template <typename T> using recursive_inlet = inlet<T, micron::recursive_lock>;

}     // namespace micron
