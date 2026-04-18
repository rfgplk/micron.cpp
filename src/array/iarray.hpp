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
#include "../concepts.hpp"
#include "../except.hpp"
#include "../math/sqrt.hpp"
#include "../math/trig.hpp"
#include "../memory/addr.hpp"
#include "../memory/memory.hpp"
#include "../slice_forward.hpp"
#include "../tags.hpp"
#include "../types.hpp"

namespace micron
{
// general purpose immutable iarray class, stack allocated, threadsafe, immutable.
// default to 64
template <is_movable_object T, usize N = 64>
  requires(N > 0 and ((N * sizeof(T)) < (1 << 22)))
class iarray
{
  alignas(alignof(T)) T stack[N];

public:
  using category_type = array_tag;
  using mutability_type = immutable_tag;
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
  static constexpr size_type static_size = length;

  ~iarray() { __impl_container::destroy<N, T>(micron::addr(stack[0])); }

  // NOTE: important
  // a void initialized array is strictly meant to be in a zeroed out state, regardless of contents
  // accessing is UB!
  iarray(void) { __impl_container::zero<N, T>(micron::addr(stack[0])); }

  template <typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  iarray(Fn &&fn)
  {
    __impl_container::construct<N, T>(micron::addr(stack[0]), T{});
    micron::generate(const_cast<iterator>(begin()), const_cast<iterator>(end()), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  iarray(Fn &&fn)
  {
    __impl_container::construct<N, T>(micron::addr(stack[0]), T{});
    micron::transform(const_cast<iterator>(begin()), const_cast<iterator>(end()), fn);
  }

  iarray(const T &o) { __impl_container::construct<N, T>(micron::addr(stack[0]), o); }

  iarray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N ) exc<except::runtime_error>("micron::iarray iarray(init_list): init_list too large.");
    size_type i = 0;
    for ( auto &&value : lst ) stack[i++] = micron::move(value);
    if ( lst.size() < N ) __impl_container::construct(micron::addr(stack[lst.size()]), T{}, N - lst.size());
  }

  template <is_container A>
    requires(!micron::is_same_v<A, iarray>)
  iarray(A &&o)
  {
    if ( o.size() < N ) exc<except::runtime_error>("micron::iarray iarray(A&&) invalid size");
    if constexpr ( micron::is_rvalue_reference_v<A &&> )
      __impl_container::move<N, T>(micron::addr(stack[0]), o.begin());
    else
      __impl_container::copy<N, T>(micron::addr(stack[0]), o.begin());
  }

  template <class C> iarray(const slice<T, C> &o)
  {
    const size_type bound = o.size() < N ? o.size() : N;
    __impl_container::copy(micron::addr(stack[0]), o.begin(), bound);
    if ( bound < N ) __impl_container::construct(micron::addr(stack[bound]), T{}, N - bound);
  }

  iarray(const iarray &o) { __impl_container::copy<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0])); }

  iarray(iarray &&o) { __impl_container::move<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0])); }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // iterators

  [[nodiscard]] const_iterator
  begin() const noexcept
  {
    return micron::addr(stack[0]);
  }

  [[nodiscard]] const_iterator
  cbegin() const noexcept
  {
    return micron::addr(stack[0]);
  }

  [[nodiscard]] const_iterator
  end() const noexcept
  {
    return micron::addr(stack[N]);
  }

  [[nodiscard]] const_iterator
  cend() const noexcept
  {
    return micron::addr(stack[N]);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // capacity

  size_type
  size() const noexcept
  {
    return N;
  }

  size_type
  max_size() const noexcept
  {
    return N;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // observers

  const auto *
  addr() const noexcept
  {
    return this;
  }

  [[nodiscard]] const T *
  data() const noexcept
  {
    return stack;
  }

  const T &
  at(const size_type i) const
  {
    if ( i >= N ) exc<except::runtime_error>("micron::iarray at() out of range.");
    return stack[i];
  }

  inline const_iterator
  get(const size_type n) const
  {
    if ( n >= N ) exc<except::library_error>("micron::iarray get() out of range");
    return (stack + n);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    if ( n >= N ) exc<except::library_error>("micron::iarray cget() out of range");
    return (stack + n);
  }

  inline const T &
  operator[](const size_type i) const noexcept
  {
    return stack[i];
  }

  template <class C>
  inline const slice<T, C>
  operator[]() const
  {
    return slice<T, C>(data(), data() + N);
  }

  template <class C>
  inline __attribute__((always_inline)) const slice<T, C>
  operator[](size_type from, size_type to) const
  {
    if ( from >= to or from > N or to > N ) exc<except::library_error>("micron::iarray operator[] out of allocated memory range.");
    return slice<T, C>(get(from), get(to));
  }

  inline T &
  mut(const size_type i)
  {
    return stack[i];
  }

  const byte *
  operator&() const noexcept
  {
    return reinterpret_cast<const byte *>(stack);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // assignment

  iarray &
  operator=(const iarray &o)
  {
    __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    return *this;
  }

  iarray &
  operator=(iarray &&o)
  {
    __impl_container::move_assign<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    return *this;
  }

  // scalar fill
  iarray &
  operator=(const T &o)
  {
    __impl_container::set<N, T>(micron::addr(stack[0]), o);
    return *this;
  }

  template <typename F>
    requires(micron::is_convertible_v<F, T>)
  iarray &
  operator=(F o)
  {
    __impl_container::set<N, T>(micron::addr(stack[0]), o);
    return *this;
  }

  template <typename F, size_type M>
  iarray &
  operator=(T (&o)[M])
    requires(M <= N)
  {
    __impl_container::copy_assign<M, T>(micron::addr(stack[0]), micron::addr(o[0]));
    return *this;
  }

  /*
  template <is_constexpr_container A>
    requires(!micron::is_arithmetic_v<A>)
  iarray &
  operator=(const A &o)
  {
    if constexpr ( N <= A::length )
      __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o[0]));
    else
      __impl_container::copy_assign<A::length, T>(micron::addr(stack[0]), micron::addr(o[0]));
    return *this;
  }
*/
  template <is_container A>
    requires(!micron::is_same_v<A, iarray> && !micron::is_arithmetic_v<A>)
  iarray &
  operator=(const A &o)
  {
    if ( N <= o.size() )
      __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o[0]));
    else
      __impl_container::copy_assign(micron::addr(stack[0]), micron::addr(o[0]), o.size());
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // binary arithmetic

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) iarray
  operator+(const iarray<T, M> &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] += src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) iarray
  operator-(const iarray<T, M> &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] -= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) iarray
  operator*(const iarray<T, M> &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] *= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) iarray
  operator/(const iarray<T, M> &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] /= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N && micron::is_integral_v<T>)
  inline __attribute__((always_inline)) iarray
  operator%(const iarray<T, M> &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] %= src[i];
    return arr;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // compound assignment

  inline __attribute__((always_inline)) iarray
  operator+=(const T &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] += o;
    return arr;
  }

  inline __attribute__((always_inline)) iarray
  operator-=(const T &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] -= o;
    return arr;
  }

  inline __attribute__((always_inline)) iarray
  operator*=(const T &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] *= o;
    return arr;
  }

  inline __attribute__((always_inline)) iarray
  operator/=(const T &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] /= o;
    return arr;
  }

  inline __attribute__((always_inline)) iarray
  operator%=(const T &o) const
    requires micron::is_integral_v<T>
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] %= o;
    return arr;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // compound assignment

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) iarray
  operator+=(const iarray<T, M> &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] += src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) iarray
  operator-=(const iarray<T, M> &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] -= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) iarray
  operator*=(const iarray<T, M> &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] *= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) iarray
  operator/=(const iarray<T, M> &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] /= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N && micron::is_integral_v<T>)
  inline __attribute__((always_inline)) iarray
  operator%=(const iarray<T, M> &o) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] %= src[i];
    return arr;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // named arithmetic

  [[nodiscard]] iarray
  mul(const size_type n) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] *= n;
    return arr;
  }

  [[nodiscard]] iarray
  div(const size_type n) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] /= n;
    return arr;
  }

  [[nodiscard]] iarray
  sub(const size_type n) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] -= n;
    return arr;
  }

  [[nodiscard]] iarray
  add(const size_type n) const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] += n;
    return arr;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // reductions

  T
  sum(void) const noexcept
  {
    const T *__restrict src = stack;
    T sm = T{};
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) sm += src[i];
    return sm;
  }

  T
  mul_reduce(void) const noexcept
  {
    const T *__restrict src = stack;
    T m = src[0];
#pragma GCC ivdep
    for ( size_type i = 1; i < N; i++ ) m *= src[i];
    return m;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // queries

  bool
  all(const T &o) const noexcept
  {
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] != o ) return false;
    return true;
  }

  bool
  any(const T &o) const noexcept
  {
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] == o ) return true;
    return false;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // transforms

  [[nodiscard]] iarray
  sqrt() const
  {
    iarray arr(*this);
    T *__restrict dst = arr.stack;
    for ( size_type i = 0; i < N; i++ ) dst[i] = math::sqrt(static_cast<float>(dst[i]));
    return arr;
  }

  [[nodiscard]] iarray
  clear() const
  {
    return iarray();
  }

  template <typename F>
  [[nodiscard]] iarray
  fill(const F &o) const
    requires micron::is_fundamental_v<F>
  {
    return iarray(o);
  }

  template <typename F>
  [[nodiscard]] iarray
  fill(const F &o) const
  {
    return iarray(o);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // type queries

  static constexpr bool
  is_pod() noexcept
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class() noexcept
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
