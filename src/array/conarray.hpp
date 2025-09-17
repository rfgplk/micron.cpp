//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../algorithm/mem.hpp"
#include "../except.hpp"
#include "../math/sqrt.hpp"
#include "../math/trig.hpp"
#include "../memory/addr.hpp"
#include "../memory/memory.hpp"
#include "../tags.hpp"
#include "../types.hpp"

#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{

// general purpose concurrent array class, stack allocated, thread-safe, mutable.
// default to 64
template <class T, size_t N = 64>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T>
           && (N > 0)     // avoid weird stuff with N = 0
class conarray
{
  micron::mutex __mtx;
  alignas(64) T stack[N];
  inline void
  __impl_zero(T *src)
  {
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = micron::move(T());
    } else {
      micron::cmemset<N>(src, 0x0);
    }
  }
  void
  __impl_set(T *__restrict src, const T &val)
  {
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = val;
    } else {
      micron::cmemset<N>(src, val);
    }
  }
  void
  __impl_copy(T *__restrict src, T *__restrict dest)
  {
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = dest[i];
    } else {
      micron::copy<N>(src, dest);
    }
  }
  void
  __impl_move(T *__restrict src, T *__restrict dest)
  {
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = micron::move(dest[i]);
    } else {
      micron::copy<N>(src, dest);
      micron::czero<N>(src);
    }
  }

public:
  using category_type = array_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  ~conarray()
  {
    // explicit
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i].~T();
    } else {
      micron::czero<N>(micron::addr(stack[0]));
    }
  }
  conarray() { __impl_zero(micron::addr(stack[0])); }
  conarray(const T &o) { __impl_set(micron::addr(stack[0]), o); }
  conarray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N )
      throw except::runtime_error("micron::conarray conarray(init_list): init_list too large.");
    size_t i = 0;
    for ( T value : lst )
      stack[i++] = micron::move(value);
  }
  template <is_container A>
    requires(!micron::is_same_v<A, conarray>)
  conarray(const A &o)
  {
    if ( o.size() < N )
      throw except::runtime_error("micron::conarray conarray(const&) invalid size");
    __impl_copy(micron::addr(o[0]), micron::addr(stack[0]));
  }
  template <is_container A>
    requires(!micron::is_same_v<A, conarray>)
  conarray(A &&o)
  {
    if ( o.size() < N )
      throw except::runtime_error("micron::conarray conarray(&&) invalid size");
    __impl_move(micron::addr(o[0]), micron::addr(stack[0]));
  }
  conarray(const conarray &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(o.__mtx);
    __impl_copy(micron::addr(o.stack[0]), micron::addr(stack[0]));
  }     // micron::copy<N>(micron::addr(o.stack[0], micron::addr(stack[0]); }
  conarray(conarray &&o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(o.__mtx);
    __impl_move(micron::addr(o.stack[0]), micron::addr(stack[0]));
    // micron::copy<N>(micron::addr(o.stack[0], micron::addr(stack[0]);
    // micron::cmemset<N>(micron::addr(stack[0], 0x0);
  }

  const_iterator
  begin() const noexcept
  {
    return micron::addr(stack[0]);
  }
  const_iterator
  end() const noexcept
  {
    return micron::addr(stack[N]);
  }
  const_iterator
  cbegin() const noexcept
  {
    return micron::addr(stack[0]);
  }
  const_iterator
  cend() const noexcept
  {
    return micron::addr(stack[N]);
  }
  const_iterator
  view() const
  {
    return micron::addr(stack[0]);
  }
  iterator
  get()     // NOTE: main method of getting the underlying conarray, mtx must be released manually
  {
    __mtx.lock();
    return micron::addr(stack[0]);
  }
  void
  release()
  {
    void (micron::mutex::*rptr)() = __mtx.retrieve();
    (__mtx.*rptr)();
  }
  size_t
  size() const
  {
    return N;
  }
  size_t
  max_size() const
  {
    return N;
  }
  template <typename F, size_t M>
  conarray &
  operator=(T (&o)[M])
    requires micron::is_array_v<F> && (M <= N)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __impl_copy(micron::addr(o.stack[0]), &o[0]);
    // micron::copy<N>(micron::addr(o.stack[0], &o[0]);
    return *this;
  }
  template <typename F>
  conarray &
  operator=(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    micron::cmemset<N>(micron::addr(stack[0]), o);
    return *this;
  }
  conarray &
  operator=(const conarray &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __impl_copy(micron::addr(o.stack[0]), micron::addr(stack[0]));
    return *this;
  }
  conarray &
  operator=(conarray &&o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __impl_move(micron::addr(o.stack[0]), micron::addr(stack[0]));
    return *this;
  }

  template <size_t M>
    requires(M <= N)
  conarray &
  operator+(const conarray<T, M> &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] += o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  conarray &
  operator-(const conarray<T, M> &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  conarray &
  operator*(const conarray<T, M> &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  conarray &
  operator/(const conarray<T, M> &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  conarray &
  operator%(const conarray<T, M> &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] %= o.stack[o];
    return *this;
  }

  // special functions - no idea why the stl doesn't have these
  size_t
  sum(void) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(micron::mutable_cast(__mtx));
    size_t sm = 0;
    for ( size_t i = 0; i < N; i++ )
      sm += stack[i];
    return sm;
  }

  conarray &
  operator*=(const T &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o;
    return *this;
  }
  conarray &
  operator/=(const T &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= o;
    return *this;
  }
  conarray &
  operator-=(const T &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= o;
    return *this;
  }
  conarray &
  operator+=(const T &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] += o;
    return *this;
  }
  void
  mul(const size_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= n;
  }
  void
  div(const size_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= n;
  }
  void
  sub(const size_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= n;
  }
  void
  add(const size_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    for ( size_t i = 0; i < N; i++ )
      stack[i] += n;
  }
  size_t
  mul(void) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(micron::mutable_cast(__mtx));
    size_t mul_ = stack[0];
    for ( size_t i = 1; i < N; i++ )
      mul_ *= stack[i];
    return mul_;
  }

  bool
  all(const T &o) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(micron::mutable_cast(__mtx));
    for ( size_t i = 0; i < N; i++ )
      if ( stack[i] != o )
        return false;
    return true;
  }
  bool
  any(const T &o) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(micron::mutable_cast(__mtx));
    for ( size_t i = 0; i < N; i++ )
      if ( stack[i] == o )
        return true;
    return false;
  }
  void
  sqrt(void) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(micron::mutable_cast(__mtx));
    for ( size_t i = 0; i < N; i++ )
      stack[i] = micron::sqrt(static_cast<float>(stack[i]));
  }
  template <typename F>
  conarray &
  fill(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    micron::cmemset<N>(micron::addr(stack[0]), o);
    return *this;
  }
};
};     // namespace micron
