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

namespace micron
{
// carray = cache-array. mutable & notthread safe.
template <class T, size_t N = 64>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T> && (N > 0)
           && ((N * sizeof(T)) % 16 == 0) && (N % 4 == 0)     // alignment just.
class alignas(32) carray
{
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
  ~carray()
  {
    // explicit
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i].~T();
    } else {
      micron::czero<N>(micron::addr(stack[0]));
    }
  }
  carray() { __impl_zero(micron::addr(stack[0])); }
  carray(const T &o)
  {
    __impl_set(micron::addr(stack[0]), o);
    // if constexpr (micron::is_class<T>::value) {
    //   for (size_t i = 0; i < N; i++)
    //     stack[i] = o;
    // } else {
    //   if constexpr (N * (sizeof(T) / sizeof(byte)) % (256 / 8) == 0)
    //     micron::memset128(micron::addr(stack[0], o, N);
    //   else if constexpr (N * (sizeof(T) / sizeof(byte)) % (128 / 8) ==
    //                      0) // verbose for clarity, is it 128-bit aligned
    //     micron::memset128(micron::addr(stack[0], o, N);
    //   else if constexpr (true) {
    //     micron::cmemset<N>(micron::addr(stack[0], o);
    //   }
    // }
  }
  carray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N )
      throw except::runtime_error("micron::array init_list too large.");
    size_t i = 0;
    for ( T value : lst )
      stack[i++] = micron::move(value);
  }

  template <is_container A>
    requires(!micron::is_same_v<A, carray>)
  carray(const A &o)
  {
    __impl_copy(micron::addr(o.stack[0]), micron::addr(stack[0]));
  }
  template <is_container A>
    requires(!micron::is_same_v<A, carray>)
  carray(A &&o)
  {
    __impl_move(micron::addr(o.stack[0]), micron::addr(stack[0]));
  }
  carray(const carray &o) { micron::copy<N>(micron::addr(o.stack[0]), micron::addr(stack[0])); }
  carray(carray &&o)
  {
    __impl_copy(micron::addr(o.stack[0]), micron::addr(stack[0]));
    __impl_zero(micron::addr(o.stack[0]));
    // micron::copy<N>(micron::addr(o.stack[0], micron::addr(stack[0]);
    // micron::memset(micron::addr(stack[0], 0x0, N);
  }

  const_iterator
  begin() const noexcept
  {
    return micron::addr(stack[0]);
  }
  const_iterator
  cbegin() const noexcept
  {
    return micron::addr(stack[0]);
  }
  const_iterator
  end() const noexcept
  {
    return micron::addr(stack[N]);
  }
  const_iterator
  cend() const noexcept
  {
    return micron::addr(stack[N]);
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
  T *
  data()
  {
    return stack;
  }
  const T *
  data() const
  {
    return stack;
  }
  inline T &
  at(const size_t i)
  {
    if ( i >= N )
      throw except::runtime_error("micron::array at() out of range.");
    return stack[i];
  }
  inline const T &
  at(const size_t i) const
  {
    if ( i >= N )
      throw except::runtime_error("micron::array at() out of range.");
    return stack[i];
  }
  inline T &
  operator[](const size_t i)
  {
    return stack[i];
  }
  inline const T &
  operator[](const size_t i) const
  {
    return stack[i];
  }
  template <typename F, size_t M>
  carray &
  operator=(T (&o)[M])
    requires micron::is_array_v<F> && (M <= N)
  {
    __impl_copy(&o[0], micron::addr(stack[0]));
    return *this;
  }
  template <typename F>
  carray &
  operator=(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::memset(micron::addr(stack[0]), o, N);
    return *this;
  }
  carray &
  operator=(const carray &o)
  {
    __impl_copy(micron::addr(o.stack[0]), micron::addr(stack[0]));
    return *this;
  }
  carray &
  operator=(carray &&o)
  {
    __impl_move(micron::addr(o.stack[0]), micron::addr(stack[0]));
  }

  template <size_t M>
    requires(M <= N)
  carray &
  operator+(const carray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  carray &
  operator-(const carray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= o.stack[o];
    return *this;
  }
  carray &
  operator*=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o;
    return *this;
  }

  carray &
  operator/=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= o;
    return *this;
  }
  carray &
  operator-=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= o;
    return *this;
  }
  carray &
  operator+=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += o;
    return *this;
  }

  template <size_t M>
    requires(M <= N)
  carray &
  operator*(const carray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  carray &
  operator/(const carray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  carray &
  operator%(const carray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] %= o.stack[o];
    return *this;
  }

  // special functions - no idea why the stl doesn't have these
  size_t
  sum(void) const
  {
    size_t sm = 0;
    for ( size_t i = 0; i < N; i++ )
      sm += stack[i];
    return sm;
  }
  void
  mul(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= n;
  }
  void
  div(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= n;
  }
  void
  sub(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= n;
  }
  void
  add(const size_t n)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += n;
  }
  size_t
  mul(void) const
  {
    size_t mul_ = stack[0];
    for ( size_t i = 1; i < N; i++ )
      mul_ *= stack[i];
    return mul_;
  }

  bool
  all(const T &o) const
  {
    for ( size_t i = 0; i < N; i++ )
      if ( stack[i] != o )
        return false;
    return true;
  }
  bool
  any(const T &o) const
  {
    for ( size_t i = 0; i < N; i++ )
      if ( stack[i] == o )
        return true;
    return false;
  }
  void
  sqrt(void) const
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] = micron::sqrt(static_cast<float>(stack[i]));
  }
  template <typename F>
  carray &
  fill(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::cmemset<N>(micron::addr(stack[0]), o);
    return *this;
  }
};
};
