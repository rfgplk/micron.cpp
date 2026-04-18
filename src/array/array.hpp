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

// general purpose array class, stack allocated, notthreadsafe, mutable.
// default to 64
template <is_regular_object T, usize N = 64>
  requires(N > 0 and ((N * sizeof(T)) < (1 << 22)))     // avoid weird stuff with N = 0
class array
{
  alignas(64) T stack[N];

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_add(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ ) dst[i] += v;
    } else {
      T *__restrict dst = stack;
      const auto *__restrict src = v.data();
      const size_type bound = v.size() < N ? v.size() : N;
#pragma GCC ivdep
      for ( size_type i = 0; i < bound; i++ ) dst[i] += src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_sub(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ ) dst[i] -= v;
    } else {
      T *__restrict dst = stack;
      const auto *__restrict src = v.data();
      const size_type bound = v.size() < N ? v.size() : N;
#pragma GCC ivdep
      for ( size_type i = 0; i < bound; i++ ) dst[i] -= src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_mul(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ ) dst[i] *= v;
    } else {
      T *__restrict dst = stack;
      const auto *__restrict src = v.data();
      const size_type bound = v.size() < N ? v.size() : N;
#pragma GCC ivdep
      for ( size_type i = 0; i < bound; i++ ) dst[i] *= src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_div(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ ) dst[i] /= v;
    } else {
      T *__restrict dst = stack;
      const auto *__restrict src = v.data();
      const size_type bound = v.size() < N ? v.size() : N;
#pragma GCC ivdep
      for ( size_type i = 0; i < bound; i++ ) dst[i] /= src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_mod(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ ) dst[i] %= v;
    } else {
      T *__restrict dst = stack;
      const auto *__restrict src = v.data();
      const size_type bound = v.size() < N ? v.size() : N;
#pragma GCC ivdep
      for ( size_type i = 0; i < bound; i++ ) dst[i] %= src[i];
    }
  }

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
  static constexpr size_type static_size = N;

  // NOTE: zeroes out memory
  ~array() { __impl_container::destroy<N, T>(micron::addr(stack[0])); }

  array() { __impl_container::construct<N, T>(micron::addr(stack[0]), T{}); }

  template <typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  array(Fn &&fn)
  {
    __impl_container::construct<N, T>(micron::addr(stack[0]), T{});
    micron::generate(begin(), end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  array(Fn &&fn)
  {
    __impl_container::construct<N, T>(micron::addr(stack[0]), T{});
    micron::transform(begin(), end(), fn);
  }

  array(const T &o) { __impl_container::construct<N, T>(micron::addr(stack[0]), o); }

  array(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N ) exc<except::runtime_error>("micron::array array(init_list): init_list too large.");
    size_type i = 0;
    for ( auto &&value : lst ) stack[i++] = micron::move(value);
    if ( lst.size() < N ) __impl_container::construct(micron::addr(stack[lst.size()]), T{}, N - lst.size());
  }

  template <is_container A>
    requires(!micron::is_same_v<A, array>)
  array(A &&o)
  {
    if ( o.size() < N ) exc<except::runtime_error>("micron::array array(&&) invalid size");
    if constexpr ( micron::is_rvalue_reference_v<A &&> )
      __impl_container::move<N, T>(micron::addr(stack[0]), o.begin());
    else
      __impl_container::copy<N, T>(micron::addr(stack[0]), o.begin());
  }

  template <class C> array(const slice<T, C> &o)
  {
    const size_type bound = o.size() < N ? o.size() : N;
    __impl_container::copy(micron::addr(stack[0]), o.begin(), bound);
    if ( bound < N ) __impl_container::construct(micron::addr(stack[bound]), T{}, N - bound);
  }

  array(const array &o) { __impl_container::copy<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0])); }

  array(array &&o) { __impl_container::move<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0])); }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // iterators

  [[nodiscard]] iterator
  begin() noexcept
  {
    return micron::addr(stack[0]);
  }

  [[nodiscard]] const_iterator
  cbegin() const noexcept
  {
    return micron::addr(stack[0]);
  }

  [[nodiscard]] iterator
  end() noexcept
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

  auto *
  addr() noexcept
  {
    return this;
  }

  const auto *
  addr() const noexcept
  {
    return this;
  }

  [[nodiscard]] iterator
  data() noexcept
  {
    return stack;
  }

  [[nodiscard]] const_iterator
  data() const noexcept
  {
    return stack;
  }

  void
  clear()
  {
    // NOTE: destroy zeroes by default
    __impl_container::destroy<N, T>(micron::addr(stack[0]));
    if ( !micron::is_trivially_constructible_v<micron::remove_cv_t<T>> ) __impl_container::construct<N, T>(micron::addr(stack[0]), T{});
  }

  T &
  at(const size_type i)
  {
    if ( i >= N ) exc<except::runtime_error>("micron::array at() out of range.");
    return stack[i];
  }

  const T &
  at(const size_type i) const
  {
    if ( i >= N ) exc<except::runtime_error>("micron::array at() out of range.");
    return stack[i];
  }

  inline iterator
  get(const size_type n)
  {
    if ( n >= N ) exc<except::library_error>("micron::array get() out of range");
    return micron::addr(stack + n);
  }

  inline const_iterator
  get(const size_type n) const
  {
    if ( n >= N ) exc<except::library_error>("micron::array get() out of range");
    return micron::addr(stack + n);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    if ( n >= N ) exc<except::library_error>("micron::array cget() out of range");
    return micron::addr(stack + n);
  }

  inline T &
  operator[](const size_type i) noexcept
  {
    return stack[i];
  }

  inline const T &
  operator[](const size_type i) const noexcept
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
    return slice<T, C>(cbegin(), cend());
  }

  template <class C>
  inline __attribute__((always_inline)) const slice<T, C>
  operator[](size_type from, size_type to) const
  {
    if ( from >= to or from > N or to > N ) exc<except::library_error>("micron::array operator[] out of allocated memory range.");
    return slice<T, C>(get(from), get(to));
  }

  template <class C>
  inline __attribute__((always_inline)) slice<T, C>
  operator[](size_type from, size_type to)
  {
    if ( from >= to or from > N or to > N ) exc<except::library_error>("micron::array operator[] out of allocated memory range.");
    return slice<T, C>(get(from), get(to));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // assignment operators

  template <typename F, size_type M>
  array &
  operator=(T (&o)[M])
    requires micron::is_array_v<F> && (M <= N)
  {
    __impl_container::copy_assign<M, T>(micron::addr(stack[0]), micron::addr(o[0]));
    return *this;
  }

  template <typename F>
  array &
  operator=(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::ctypeset<N>(micron::addr(stack[0]), o);
    return *this;
  }

  template <is_constexpr_container A>
  array &
  operator=(const A &o)
  {
    if constexpr ( N <= A::length )
      __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o[0]));
    else
      __impl_container::copy_assign<A::length, T>(micron::addr(stack[0]), micron::addr(o[0]));
    return *this;
  }

  template <is_container A>
    requires(!micron::is_same_v<A, array>)
  array &
  operator=(const A &o)
  {
    if ( N <= o.size() )
      __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o[0]));
    else
      __impl_container::copy_assign(micron::addr(stack[0]), micron::addr(o[0]), o.size());
    return *this;
  }

  array &
  operator=(const array &o)
  {
    __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    return *this;
  }

  array &
  operator=(array &&o)
  {
    __impl_container::move_assign<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // binary arithmetic
  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) array
  operator+(const array<T, M> &o) const
  {
    array arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] += src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) array
  operator-(const array<T, M> &o) const
  {
    array arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] -= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) array
  operator*(const array<T, M> &o) const
  {
    array arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] *= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) array
  operator/(const array<T, M> &o) const
  {
    array arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] /= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N && micron::is_integral_v<T>)
  inline __attribute__((always_inline)) array
  operator%(const array<T, M> &o) const
  {
    array arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] %= src[i];
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
  // compound assignment

  inline __attribute__((always_inline)) array &
  operator+=(const T &o)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] += o;
    return *this;
  }

  inline __attribute__((always_inline)) array &
  operator-=(const T &o)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] -= o;
    return *this;
  }

  inline __attribute__((always_inline)) array &
  operator*=(const T &o)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] *= o;
    return *this;
  }

  inline __attribute__((always_inline)) array &
  operator/=(const T &o)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] /= o;
    return *this;
  }

  inline __attribute__((always_inline)) array &
  operator%=(const T &o)
    requires micron::is_integral_v<T>
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] %= o;
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // compound assignment

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) array &
  operator+=(const array<T, M> &o)
  {
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] += src[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) array &
  operator-=(const array<T, M> &o)
  {
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] -= src[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) array &
  operator*=(const array<T, M> &o)
  {
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] *= src[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) array &
  operator/=(const array<T, M> &o)
  {
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] /= src[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N && micron::is_integral_v<T>)
  inline __attribute__((always_inline)) array &
  operator%=(const array<T, M> &o)
  {
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] %= src[i];
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // variadic compound assignment

  template <typename... Rs>
    requires(sizeof...(Rs) >= 2)
  array &
  operator+=(const Rs &...rs)
  {
    (__apply_add(rs), ...);
    return *this;
  }

  template <typename... Rs>
    requires(sizeof...(Rs) >= 2)
  array &
  operator-=(const Rs &...rs)
  {
    (__apply_sub(rs), ...);
    return *this;
  }

  template <typename... Rs>
    requires(sizeof...(Rs) >= 2)
  array &
  operator*=(const Rs &...rs)
  {
    (__apply_mul(rs), ...);
    return *this;
  }

  template <typename... Rs>
    requires(sizeof...(Rs) >= 2)
  array &
  operator/=(const Rs &...rs)
  {
    (__apply_div(rs), ...);
    return *this;
  }

  template <typename... Rs>
    requires(sizeof...(Rs) >= 2 && micron::is_integral_v<T>)
  array &
  operator%=(const Rs &...rs)
  {
    (__apply_mod(rs), ...);
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  byte *
  operator&() noexcept
  {
    return reinterpret_cast<byte *>(stack);
  }

  const byte *
  operator&() const noexcept
  {
    return reinterpret_cast<const byte *>(stack);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // named in-place arithmetic

  void
  mul(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] *= n;
  }

  void
  div(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] /= n;
  }

  void
  sub(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] -= n;
  }

  void
  add(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] += n;
  }

  T
  mul(void)
  {
    T mul_ = stack[0];
    T *__restrict src = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) mul_ *= src[i];
    return mul_;
  }

  T
  div(void)
  {
    T mul_ = stack[0];
    T *__restrict src = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) mul_ *= src[i];
    return mul_;
  }

  T
  sub(void)
  {
    T mul_ = stack[0];
    T *__restrict src = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) mul_ *= src[i];
    return mul_;
  }

  T
  add(void)
  {
    T mul_ = stack[0];
    T *__restrict src = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) mul_ *= src[i];
    return mul_;
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

  void
  sqrt(void)
  {
    T *__restrict dst = stack;
    for ( size_type i = 0; i < N; i++ ) dst[i] = math::sqrt(static_cast<float>(dst[i]));
  }

  template <typename F>
  array &
  fill(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::ctypeset<N>(micron::addr(stack[0]), o);
    return *this;
  }

  template <typename F>
  array &
  fill(const F &o)
  {
    __impl_container::set<N, T>(micron::addr(stack[0]), o);
    return *this;
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
