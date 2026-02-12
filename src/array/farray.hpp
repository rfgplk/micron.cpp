//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../algorithm/memory.hpp"
#include "../except.hpp"
#include "../math/sqrt.hpp"
#include "../math/trig.hpp"
#include "../memory/addr.hpp"
#include "../memory/memory.hpp"
#include "../tags.hpp"
#include "../types.hpp"

namespace micron
{
// general purpose fundamental array class, only allows fundamental types
// (int, char, etc) stack allocated, notthreadsafe, mutable. default to 64
template <class T, size_t N = 64>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T> && (N > 0)
           && micron::is_fundamental_v<T>     // avoid weird stuff with N = 0
class farray
{
  alignas(64) T stack[N];

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

  ~farray() = default;
  farray() { micron::czero<N>(&stack[0]); }
  farray(const T &o) { micron::cmemset<N>(&stack[0], o); }
  farray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N )
      exc<except::runtime_error>("micron::farray init_list too large.");
    size_t i = 0;
    for ( T value : lst )
      stack[i++] = micron::move(value);
  }
  template <is_container A>
    requires(!micron::is_same_v<A, farray>)
  farray(const A &o)
  {
    micron::copy<N>(&o[0], &stack[0]);
  }
  template <is_container A>
    requires(!micron::is_same_v<A, farray>)
  farray(A &&o)
  {
    micron::copy<N>(&o[0], &stack[0]);
  }
  farray(const farray &o) { micron::copy<N>(&o.stack[0], &stack[0]); }
  farray(farray &&o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    micron::cmemset<N>(&o.stack[0], 0x0);
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
  iterator
  data()
  {
    return stack;
  }
  const_iterator
  data() const
  {
    return stack;
  }
  inline T &
  at(const size_t i)
  {
    if ( i >= N )
      exc<except::runtime_error>("micron::farray at() out of range.");
    return stack[i];
  }
  inline const T &
  at(const size_t i) const
  {
    if ( i >= N )
      exc<except::runtime_error>("micron::farray at() out of range.");
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
  farray &
  operator=(T (&o)[M])
    requires micron::is_array_v<F> && (M <= N)
  {
    micron::copy<N>(&o.stack[0], &o[0]);
    return *this;
  }
  template <typename F>
  farray &
  operator=(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::cmemset<N>(&stack[0], o);
    return *this;
  }
  farray &
  operator=(const farray &o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    return *this;
  }
  template <is_constexpr_container A>
  farray &
  operator=(const A &o)
  {
    micron::copy<N>(&o[0], &stack[0]);
    return *this;
  }
  template <is_container A>
    requires(!micron::is_same_v<A, farray>)
  farray &
  operator=(const A &o)
  {
    micron::copy<N>(&o[0], &stack[0]);
    return *this;
  }
  farray &
  operator=(farray &&o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    micron::cmemset<N>(&stack[0], 0x0);
  }

  template <size_t M>
    requires(M <= N)
  farray &
  operator+(const farray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] += o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  farray &
  operator-(const farray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] -= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  farray &
  operator*(const farray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  farray &
  operator/(const farray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] /= o.stack[o];
    return *this;
  }
  template <size_t M>
    requires(M <= N)
  farray &
  operator%(const farray<T, M> &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] %= o.stack[o];
    return *this;
  }
  byte *
  operator&()
  {
    return reinterpret_cast<byte *>(stack);
  }
  const byte *
  operator&() const
  {
    return reinterpret_cast<byte *>(stack);
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

  farray &
  operator*=(const T &o)
  {
    for ( size_t i = 0; i < N; i++ )
      stack[i] *= o;
    return *this;
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
      stack[i] = math::sqrt(static_cast<float>(stack[i]));
  }
  template <typename F>
  farray &
  fill(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::cmemset<N>(micron::addr(stack[0]), o);
    return *this;
  }
  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }
};

};
