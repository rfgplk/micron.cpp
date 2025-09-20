//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "intrin.hpp"

#include "../memory/actions.hpp"
#include "../types.hpp"

#include "../__special/initializer_list"
#include "../concepts.hpp"
#include "../type_traits.hpp"

#ifdef __GNUC__
#define USE_GCC_ATOMICS
#endif

// atomic library wrapped around standard C/C++
namespace micron
{
// #define ATOMIC_LOCKED 0
// #define ATOMIC_OPEN 1

constexpr bool ATOMIC_OPEN = 0;
constexpr bool ATOMIC_LOCKED = 1;

enum class memory_order : int {
  relaxed = atomic_seq_cst,
  consume = atomic_consume,
  acquire = atomic_acquire,
  release = atomic_release,
  acq_rel = atomic_acq_rel,
  seq_cst = atomic_seq_cst,
  __end
};

inline constexpr memory_order memory_order_relaxed = memory_order::relaxed;
inline constexpr memory_order memory_order_consume = memory_order::consume;
inline constexpr memory_order memory_order_acquire = memory_order::acquire;
inline constexpr memory_order memory_order_release = memory_order::release;
inline constexpr memory_order memory_order_acq_rel = memory_order::acq_rel;
inline constexpr memory_order memory_order_seq_cst = memory_order::seq_cst;

template <typename T, size_t N> constexpr bool size_of = (sizeof(T) == N);
#ifdef USE_GCC_ATOMICS
template <typename T> struct atomic_token {
  T v;
  static_assert(size_of<T, 1> or size_of<T, 2> or size_of<T, 4> or size_of<T, 8>, "Size must be 1, 2, 4, or 8 bytes");

  atomic_token(const T t = ATOMIC_OPEN) : v(t) {};
  atomic_token(const atomic_token &o) : v(o.v) {};
  atomic_token(atomic_token &&t) : v(micron::move(t.v)) {};
  void operator=(const atomic_token &) = delete;
  atomic_token& operator=(atomic_token &&o){
    v = o.v;
    o.v = {};
    return *this;
  }

  T
  fetch_add(T set, memory_order order) noexcept
  {
    return atom::fetch_sub(&v, set, (int)order);
  };
  T
  fetch_sub(T set, memory_order order) noexcept
  {
    return atom::fetch_sub(&v, set, (int)order);
  };
  T
  add_fetch(T set, memory_order order) noexcept
  {
    return atom::add_fetch(&v, set, (int)order);
  };
  T
  sub_fetch(T set, memory_order order) noexcept
  {
    return atom::sub_fetch(&v, set, (int)order);
  };
  bool
  compare_and_swap(const T old, const T new_) noexcept
  {
    T tmp = old;
    return atom::compare_exchange(&v, &tmp, new_, true, atomic_seq_cst, atomic_seq_cst);
  };
  bool
  compare_exchange_strong(T &expected, T desired, memory_order order) noexcept
  {
    return atom::compare_exchange(&v, &expected, desired, false, (int)order);
  }
  bool
  compare_exchange_strong(T &expected, T desired, memory_order success, memory_order failure) noexcept
  {
    return atom::compare_exchange(&v, &expected, desired, false, (int)success, (int)failure);
  }
  bool
  compare_exchange_weak(T &expected, T desired, memory_order success, memory_order failure) noexcept
  {
    return atom::compare_exchange(&v, &expected, desired, true, (int)success, (int)failure);
  }
  void
  store(const T new_, memory_order mr = memory_order::seq_cst) noexcept
  {
    atom::store(&v, new_, (int)mr);
  };
  T
  get(memory_order mrd = memory_order::seq_cst) const
  {
    return atom::load(&v, (int)mrd);
  }
  T*
  ptr()
  {
    return &v;
  }
  T
  swap(const T new_) noexcept
  {
    return atom::exchange(&v, new_, atomic_seq_cst);
  };

  // high levels
  T
  operator()() const
  {
    return get();
  }
  T
  operator++() noexcept
  {
    return atom::add_fetch(&v, 1, atomic_seq_cst);
  };
  T
  operator--() noexcept
  {
    return atom::sub_fetch(&v, 1, atomic_seq_cst);
  };
  T
  operator=(const T new_)
  {
    store(new_);
    return new_;
  }
};
#endif
// wraps around atomic_token so actually appears atomic
template <class T> class atomic
{
  T type;     // internal data, can be any
  atomic_token<size_t> tk;

  void
  lock_check()
  {
    while ( !tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED) )
      ;
  }
  void
  lock()
  {
    tk.store(ATOMIC_LOCKED);
  }
  void
  unlock()
  {
    tk.store(ATOMIC_OPEN);
  }

public:
  void
  __store(const T new_, memory_order mr = memory_order::seq_cst) noexcept
  {
    return tk.store(new_, mr);
  };
  T
  __get(memory_order mrd = memory_order::seq_cst) const
  {
    return tk.get(mrd);
  }
  bool
  compare_exchange_strong(T &expected, T desired, memory_order order) noexcept
  {
    return tk.compare_exchange_strong(expected, desired, order);
  }
  bool
  compare_exchange_strong(T &expected, T desired, memory_order success, memory_order failure) noexcept
  {
    return tk.compare_exchange_strong(expected, desired, success, failure);
  }
  bool
  compare_exchange_weak(T &expected, T desired, memory_order success, memory_order failure) noexcept
  {
    return tk.compare_exchange_weak(expected, desired, success, failure);
  }
  atomic() : type(), tk() {};
  template <typename F> atomic(std::initializer_list<F> list) : type(list), tk(){};
  template <typename... Args> atomic(Args... args) : type(args...), tk(){};
  atomic(atomic<T> &&o) : type(micron::move(o.type)), tk(micron::move(o.tk)) {};
  atomic(const atomic<T> &o) : type(o.type), tk(o.tk) {};
  T
  operator=(const T &o)
  {
    lock_check();
    type = o;
    auto tmp = type;
    unlock();
    return tmp;
  };
  template <typename N>
  T
  operator[](const N n)
  {
    lock_check();
    auto tmp = type[n];
    unlock();
    return tmp;
  };
  T *
  operator->()
  {
    lock_check();
    return &type;
  }
  // manual functions
  T *
  get()
  {
    lock_check();
    return &type;
  }     // NOTE: must release
  void
  release()
  {
    unlock();
  }
  // operator overloads
  T
  operator++(void)
  {
    lock_check();
    ++type;
    unlock();
    return type;
  }
  T
  operator--(void)
  {
    lock_check();
    --type;
    unlock();
    return type;
  }
  T
  operator*=(const T a)
  {
    lock_check();
    type *= a;
    unlock();
    return type;
  }
  T
  operator/=(const T a)
  {
    lock_check();
    type /= a;
    unlock();
    return type;
  }
  T
  operator-=(const T a)
  {
    lock_check();
    type -= a;
    unlock();
    return type;
  }
  T
  operator+=(const T a)
  {
    lock_check();
    type += a;
    unlock();
    return type;
  }
  T
  operator%=(const T a)
  {
    lock_check();
    type %= a;
    unlock();
    return type;
  }
  T
  operator&=(const T a)
  {
    lock_check();
    type &= a;
    unlock();
    return type;
  }
  T
  operator|=(const T a)
  {
    lock_check();
    type |= a;
    unlock();
    return type;
  }
  T
  operator^=(const T a)
  {
    lock_check();
    type ^= a;
    unlock();
    return type;
  }
};

template <typename T> class atomic_ref
{
  static_assert(micron::is_trivially_copyable_v<T>, "atomic_ref requires a trivially copyable type");

  T *ptr;

public:
  using value_type = T;

  constexpr explicit atomic_ref(T &obj) noexcept : ptr(&obj) {}

  atomic_ref(const atomic_ref &) = delete;
  atomic_ref &operator=(const atomic_ref &) = delete;

  T
  load(memory_order order = memory_order::seq_cst) const noexcept
  {
    atomic_token<T> tmp(*ptr);
    return tmp.get(order);
  }

  void
  store(T val, memory_order order = memory_order::seq_cst) noexcept
  {
    atomic_token<T> tmp(*ptr);
    tmp.store(val, order);
  }

  T
  exchange(T val, memory_order order = memory_order::seq_cst) noexcept
  {
    atomic_token<T> tmp(*ptr);
    return tmp.swap(val);
  }

  bool
  compare_exchange_strong(T &expected, T desired, memory_order success = memory_order::seq_cst,
                          memory_order failure = memory_order::seq_cst) noexcept
  {
    atomic_token<T> tmp(*ptr);
    return tmp.compare_exchange_strong(expected, desired, success, failure);
  }

  T
  operator++() noexcept
  {
    atomic_token<T> tmp(*ptr);
    return ++tmp;
  }

  T
  operator--() noexcept
  {
    atomic_token<T> tmp(*ptr);
    return --tmp;
  }

  template <typename U = T>
  micron::enable_if_t<micron::is_arithmetic_v<U>, U>
  fetch_add(U val, memory_order order = memory_order::seq_cst) noexcept
  {
    atomic_token<T> tmp(*ptr);
    T old = tmp.get(order);
    T new_ = old + val;
    while ( !tmp.compare_and_swap(old, new_) ) {
      new_ = old + val;
    }
    return old;
  }

  template <typename U = T>
  micron::enable_if_t<micron::is_arithmetic_v<U>, U>
  fetch_sub(U val, memory_order order = memory_order::seq_cst) noexcept
  {
    atomic_token<T> tmp(*ptr);
    T old = tmp.get(order);
    T new_ = old - val;
    while ( !tmp.compare_and_swap(old, new_) ) {
      new_ = old - val;
    }
    return old;
  }
};
};     // namespace micron
