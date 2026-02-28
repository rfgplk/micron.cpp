//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../bits/__container.hpp"
#include "../type_traits.hpp"

#include "../algorithm/algorithm.hpp"
#include "../algorithm/memory.hpp"
#include "../except.hpp"
#include "../math/sqrt.hpp"
#include "../math/trig.hpp"
#include "../memory/addr.hpp"
#include "../memory/memory.hpp"
#include "../slice_forward.hpp"
#include "../tags.hpp"
#include "../types.hpp"

#include "../concepts.hpp"

namespace micron
{

// general purpose carray class, stack allocated, notthreadsafe, mutable.
// default to 64
template <is_regular_object T, usize N = 64>
  requires(N > 0 and ((N * sizeof(T)) < (1 << 22)))     // avoid weird stuff with N = 0
class carray
{
  alignas(64) T stack[N];

public:
  using category_type = array_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef usize size_type;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;
  static constexpr size_type length = N;

  ~carray() { __impl_container::destroy<N>(micron::addr(stack[0])); }

  carray() { __impl_container::zero<N>(micron::addr(stack[0])); }

  template <typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  carray(Fn &&fn)
  {
    __impl_container::set<N>(micron::addr(stack[0]), T{});
    micron::generate(begin(), end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  carray(Fn &&fn)
  {
    __impl_container::set<N>(micron::addr(stack[0]), T{});
    micron::transform(begin(), end(), fn);
  }

  carray(const T &o) { __impl_container::set<N>(micron::addr(stack[0]), o); }

  carray(const std::initializer_list<T> &&lst)
  {
    size_type i = 0;
    for ( auto &&value : lst )
      stack[i++] = micron::move(value);
  }

  template <is_container A>
    requires(!micron::is_same_v<A, carray>)
  carray(A &&o)
  {
    if constexpr ( micron::is_rvalue_reference_v<A &&> )
      __impl_container::move<N, T>(micron::addr(stack[0]), o.begin());
    else
      __impl_container::copy<N, T>(micron::addr(stack[0]), o.begin());
  }

  template <class C> carray(const slice<T, C> &o)
  {
    if ( o.size() <= N )
      __impl_container::copy<N, T>(micron::addr(stack[0]), o.begin());
    else
      __impl_container::copy(micron::addr(stack[0]), o.begin(), o.size());
  }

  carray(const carray &o)
  {
    __impl_container::copy<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
  }     // micron::copy<N, T><N>(micron::addr(o.stack[0], micron::addr(stack[0]); }

  carray(carray &&o) { __impl_container::move<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0])); }

  iterator
  begin() noexcept
  {
    return micron::addr(stack[0]);
  }

  const_iterator
  cbegin() const noexcept
  {
    return micron::addr(stack[0]);
  }

  iterator
  end() noexcept
  {
    return micron::addr(stack[N]);
  }

  const_iterator
  cend() const noexcept
  {
    return micron::addr(stack[N]);
  }

  size_type
  size() const
  {
    return N;
  }

  size_type
  max_size() const
  {
    return N;
  }

  auto *
  addr()
  {
    return this;
  }

  const auto *
  addr() const
  {
    return this;
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

  T &
  at(const size_type i)
  {
    return stack[i];
  }

  const T &
  at(const size_type i) const
  {
    return stack[i];
  }

  inline iterator
  get(const size_type n)
  {
    return micron::addr(stack + n);
  }

  inline const_iterator
  get(const size_type n) const
  {
    return micron::addr(stack + n);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    return micron::addr(stack + n);
  }

  inline T &
  operator[](const size_type i)
  {
    return stack[i];
  }

  inline const T &
  operator[](const size_type i) const
  {
    return stack[i];
  }

  template <class C>
  inline slice<T, C>
  operator[]()
  {
    return slice<T, C>(begin(), end());
  }

  template <class C>
  inline const slice<T, C>
  operator[]() const
  {
    return slice<T, C>(begin(), end());
  }

  template <class C>
  inline __attribute__((always_inline)) const slice<T, C>
  operator[](size_type from, size_type to) const
  {
    return slice<T, C>(get(from), get(to));
  }

  template <class C>
  inline __attribute__((always_inline)) slice<T, C>
  operator[](size_type from, size_type to)
  {
    return slice<T, C>(get(from), get(to));
  }

  template <typename F, size_type M>
  carray &
  operator=(T (&o)[M])
    requires micron::is_array_v<F> && (M <= N)
  {
    __impl_container::copy<N, T>(micron::addr(&stack[0]), &o[0]);
    // micron::copy<N, T><N>(micron::addr(o.stack[0], &o[0]);
    return *this;
  }

  template <typename F>
  carray &
  operator=(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::ctypeset<N>(micron::addr(stack[0]), o);
    return *this;
  }

  template <is_constexpr_container A>
  carray &
  operator=(const A &o)
  {
    if constexpr ( N <= A::length ) {
      __impl_container::copy<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    } else {
      __impl_container::copy<A::length>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    }
    return *this;
  }

  template <is_container A>
    requires(!micron::is_same_v<A, carray>)
  carray &
  operator=(const A &o)
  {
    if ( N <= o.size() ) {
      __impl_container::copy<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    } else {
      __impl_container::copy(micron::addr(stack[0]), micron::addr(o.stack[0]), o.size());
    }
    return *this;
  }

  carray &
  operator=(const carray &o)
  {
    __impl_container::copy<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    return *this;
  }

  carray &
  operator=(carray &&o)
  {
    __impl_container::move<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  carray &
  operator+(const carray<T, M> &o)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] += o.stack[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  carray &
  operator-(const carray<T, M> &o)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] -= o.stack[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  carray &
  operator*(const carray<T, M> &o)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] *= o.stack[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  carray &
  operator/(const carray<T, M> &o)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] /= o.stack[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  carray &
  operator%(const carray<T, M> &o)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] %= o.stack[i];
    return *this;
  }

  // special functions - no idea why the stl doesn't have these
  size_type
  sum(void) const
  {
    size_type sm = 0;
    for ( size_type i = 0; i < N; i++ )
      sm += stack[i];
    return sm;
  }

  carray &
  operator*=(const T &o)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] *= o;
    return *this;
  }

  carray &
  operator/=(const T &o)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] /= o;
    return *this;
  }

  carray &
  operator-=(const T &o)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] -= o;
    return *this;
  }

  carray &
  operator+=(const T &o)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] += o;
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

  void
  mul(const size_type n)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] *= n;
  }

  void
  div(const size_type n)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] /= n;
  }

  void
  sub(const size_type n)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] -= n;
  }

  void
  add(const size_type n)
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] += n;
  }

  size_type
  mul(void) const
  {
    size_type mul_ = stack[0];
    for ( size_type i = 1; i < N; i++ )
      mul_ *= stack[i];
    return mul_;
  }

  bool
  all(const T &o) const
  {
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] != o )
        return false;
    return true;
  }

  bool
  any(const T &o) const
  {
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] == o )
        return true;
    return false;
  }

  void
  sqrt(void) const
  {
    for ( size_type i = 0; i < N; i++ )
      stack[i] = math::sqrt(static_cast<float>(stack[i]));
  }

  template <typename F>
  carray &
  fill(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::ctypeset<N>(micron::addr(stack[0]), o);
    return *this;
  }

  template <typename F>
  carray &
  fill(const F &o)
  {
    __impl_container::set<N>(micron::addr(stack[0]), o);
    return *this;
  }

  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class()
  {
    return micron::is_class_v<T>;
  }

  static constexpr bool
  is_trivial() noexcept
  {
    return micron::is_trivial_v<T>;
  }
};

};     // namespace micron
