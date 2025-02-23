#pragma once

#include "types.hpp"

#include <atomic>
#include <initializer_list>
#include <type_traits>

#ifdef __GNUC__
#define USE_GCC_ATOMICS
#endif

// atomic library wrapped around standard C/C++
namespace micron
{
//#define ATOMIC_LOCKED 0
//#define ATOMIC_OPEN 1

constexpr bool ATOMIC_OPEN = 0;
constexpr bool ATOMIC_LOCKED = 1;


template <typename T, size_t N> constexpr bool size_of = (sizeof(T) == N);
// simplest atomic_token implementable, with stl use std::atomic
#ifdef USE_GCC_ATOMICS
template <typename T> struct atomic_token {
  volatile T v;
  static_assert(size_of<T, 1> or size_of<T, 2> or size_of<T, 4> or size_of<T, 8>, "Size must be 1, 2, 4, or 8 bytes");

  atomic_token(const T t = ATOMIC_OPEN) : v(t) {};
  atomic_token(const atomic_token &o) : v(o.v) {};
  atomic_token(atomic_token &&t) : v(std::move(t.v)) {};
  void operator=(const atomic_token &) = delete;

  T
  operator++()
  {
    return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST);
  };
  T
  operator--()
  {
    return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST);
  };
  bool
  compare_and_swap(const T old, const T new_)
  {
    T tmp = old;
    return __atomic_compare_exchange_n(&v, &tmp, new_, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  };

  void
  store(const T new_)
  {
    __atomic_store_n(&v, new_, __ATOMIC_SEQ_CST);
  };

  T
  get() const
  {
    return __atomic_load_n(&v, __ATOMIC_SEQ_CST);
  }

  T
  swap(const T new_)
  {
    return __atomic_exchange_n(&v, new_, __ATOMIC_SEQ_CST);
  };

  T
  operator()() const
  {
    return get();
  }
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
  atomic() : type(), tk() {};
  template <typename F> atomic(std::initializer_list<F> list) : type(list), tk(){};
  template <typename... Args> atomic(Args... args) : type(args...), tk(){};
  atomic(atomic<T> &&o) : type(std::move(o.type)), tk(std::move(o.tk)) {};
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
  }     // must release
  void
  release()
  {
    unlock();
  }
  // operator overloads
  T
  operator++(void){
    lock_check();
    ++type;
    unlock();
    return type;
  }
  T
  operator--(void){
    lock_check();
    --type;
    unlock();
    return type;
  }
  T
  operator*=(const T a) {
    lock_check();
    type *= a;
    unlock();
    return type;
  }
  T
  operator/=(const T a) {
    lock_check();
    type /= a;
    unlock();
    return type;
  }
  T
  operator-=(const T a) {
    lock_check();
    type -= a;
    unlock();
    return type;
  }
  T
  operator+=(const T a) {
    lock_check();
    type += a;
    unlock();
    return type;
  }
  T
  operator%=(const T a) {
    lock_check();
    type %= a;
    unlock();
    return type;
  }
  T
  operator&=(const T a) {
    lock_check();
    type &= a;
    unlock();
    return type;
  }
  T
  operator|=(const T a) {
    lock_check();
    type |= a;
    unlock();
    return type;
  }
  T
  operator^=(const T a) {
    lock_check();
    type ^= a;
    unlock();
    return type;
  }
};
};     // namespace micron
