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

namespace micron
{
// general purpose fundamental array class, only allows fundamental types
// (int, char, etc) stack allocated, notthreadsafe, mutable. default to 64
template <is_fundamental_object T, usize N = 64>
  requires(N > 0 and ((N * sizeof(T)) < (1 << 22)))     // avoid weird stuff with N = 0
class farray
{
  alignas(64) T stack[N];

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_add(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ )
        dst[i] += v;
    } else {
      T *__restrict dst = stack;
      const auto *__restrict src = v.data();
      const size_type bound = v.size() < N ? v.size() : N;
#pragma GCC ivdep
      for ( size_type i = 0; i < bound; i++ )
        dst[i] += src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_sub(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ )
        dst[i] -= v;
    } else {
      T *__restrict dst = stack;
      const auto *__restrict src = v.data();
      const size_type bound = v.size() < N ? v.size() : N;
#pragma GCC ivdep
      for ( size_type i = 0; i < bound; i++ )
        dst[i] -= src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_mul(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ )
        dst[i] *= v;
    } else {
      T *__restrict dst = stack;
      const auto *__restrict src = v.data();
      const size_type bound = v.size() < N ? v.size() : N;
#pragma GCC ivdep
      for ( size_type i = 0; i < bound; i++ )
        dst[i] *= src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_div(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ )
        dst[i] /= v;
    } else {
      T *__restrict dst = stack;
      const auto *__restrict src = v.data();
      const size_type bound = v.size() < N ? v.size() : N;
#pragma GCC ivdep
      for ( size_type i = 0; i < bound; i++ )
        dst[i] /= src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_mod(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ )
        dst[i] %= v;
    } else {
      T *__restrict dst = stack;
      const auto *__restrict src = v.data();
      const size_type bound = v.size() < N ? v.size() : N;
#pragma GCC ivdep
      for ( size_type i = 0; i < bound; i++ )
        dst[i] %= src[i];
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
  static constexpr size_type static_size = length;

  // just pop off
  ~farray() = default;

  farray() { micron::czero_n<N>(&stack[0]); }

  template <typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  farray(Fn &&fn)
  {
    micron::czero_n<N>(&stack[0]);
    micron::generate(begin(), end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  farray(Fn &&fn)
  {
    micron::czero_n<N>(&stack[0]);
    micron::transform(begin(), end(), fn);
  }

  farray(const T &o) { micron::ctypeset<N>(&stack[0], o); }

  farray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N )
      exc<except::runtime_error>("micron::farray init_list too large.");
    size_type i = 0;
    for ( T value : lst )
      stack[i++] = micron::move(value);
    if ( lst.size() < N )
      micron::byteset(micron::addr(stack[lst.size()]), 0x0, (N - lst.size()) * sizeof(T));
  }

  template <is_container A>
    requires(!micron::is_same_v<A, farray>)
  farray(const A &o)
  {
    if ( o.size() < N )
      exc<except::runtime_error>("micron::farray farray(const A&) invalid size");
    __impl_container::copy<N, T>(&stack[0], &o[0]);
  }

  template <is_container A>
    requires(!micron::is_same_v<A, farray> and micron::is_fundamental_v<typename A::value_type>)
  farray(A &&o)
  {
    if ( o.size() < N )
      exc<except::runtime_error>("micron::farray farray(A&&) invalid size");
    __impl_container::copy<N, T>(&stack[0], &o[0]);
  }

  farray(const farray &o) { __impl_container::copy<N, T>(&stack[0], &o.stack[0]); }

  farray(farray &&o)
  {
    micron::copy<N, T>(&o.stack[0], &stack[0]);
    micron::ctypeset<N>(&o.stack[0], T{});
  }

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
    micron::czero_n<N>(&stack[0]);
  }

  inline T &
  at(const size_type i)
  {
    if ( i >= N )
      exc<except::runtime_error>("micron::farray at() out of range.");
    return stack[i];
  }

  inline const T &
  at(const size_type i) const
  {
    if ( i >= N )
      exc<except::runtime_error>("micron::farray at() out of range.");
    return stack[i];
  }

  inline iterator
  get(const size_type n)
  {
    if ( n >= N )
      exc<except::library_error>("micron::farray get() out of range");
    return (stack + n);
  }

  inline const_iterator
  get(const size_type n) const
  {
    if ( n >= N )
      exc<except::library_error>("micron::farray get() out of range");
    return (stack + n);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    if ( n >= N )
      exc<except::library_error>("micron::farray cget() out of range");
    return (stack + n);
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
    if ( from >= to or from > N or to > N )
      exc<except::library_error>("micron::farray operator[] out of allocated memory range.");
    return slice<T, C>(get(from), get(to));
  }

  template <class C>
  inline __attribute__((always_inline)) slice<T, C>
  operator[](size_type from, size_type to)
  {
    if ( from >= to or from > N or to > N )
      exc<except::library_error>("micron::farray operator[] out of allocated memory range.");
    return slice<T, C>(get(from), get(to));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // assignment operators

  template <typename F, size_type M>
  farray &
  operator=(T (&o)[M])
    requires micron::is_array_v<F> && (M <= N)
  {
    micron::copy<M, T>(&o[0], &stack[0]);
    return *this;
  }

  farray &
  operator=(T o)
  {
    micron::ctypeset<N>(&stack[0], o);
    return *this;
  }

  farray &
  operator=(const farray &o)
  {
    __impl_container::copy_assign<N, T>(&stack[0], &o.stack[0]);
    return *this;
  }

  template <is_constexpr_container A>
  farray &
  operator=(const A &o)
  {
    if constexpr ( N <= A::length )
      __impl_container::copy_assign<N, T>(&stack[0], micron::addr(o[0]));
    else
      __impl_container::copy_assign<A::length, T>(&stack[0], micron::addr(o[0]));
    return *this;
  }

  template <is_container A>
    requires(!micron::is_same_v<A, farray>)
  farray &
  operator=(const A &o)
  {
    if ( N <= o.size() )
      __impl_container::copy_assign<N, T>(&stack[0], micron::addr(o[0]));
    else
      micron::copy_n(&o[0], &stack[0], o.size() * sizeof(T));
    return *this;
  }

  farray &
  operator=(farray &&o)
  {
    micron::copy<N, T>(&o.stack[0], &stack[0]);
    micron::ctypeset<N>(&o.stack[0], T{});
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // binary arithmetic

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) farray
  operator+(const farray<T, M> &o) const
  {
    farray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ )
      dst[i] += src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) farray
  operator-(const farray<T, M> &o) const
  {
    farray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ )
      dst[i] -= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) farray
  operator*(const farray<T, M> &o) const
  {
    farray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ )
      dst[i] *= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) farray
  operator/(const farray<T, M> &o) const
  {
    farray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ )
      dst[i] /= src[i];
    return arr;
  }

  template <size_type M>
    requires(M <= N && micron::is_integral_v<T>)
  inline __attribute__((always_inline)) farray
  operator%(const farray<T, M> &o) const
  {
    farray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ )
      dst[i] %= src[i];
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
    for ( size_type i = 0; i < N; i++ )
      sm += src[i];
    return sm;
  }

  T
  mul_reduce(void) const noexcept
  {
    const T *__restrict src = stack;
    T m = src[0];
#pragma GCC ivdep
    for ( size_type i = 1; i < N; i++ )
      m *= src[i];
    return m;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // compound assignment

  inline __attribute__((always_inline)) farray &
  operator+=(const T &o)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] += o;
    return *this;
  }

  inline __attribute__((always_inline)) farray &
  operator-=(const T &o)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] -= o;
    return *this;
  }

  inline __attribute__((always_inline)) farray &
  operator*=(const T &o)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] *= o;
    return *this;
  }

  inline __attribute__((always_inline)) farray &
  operator/=(const T &o)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] /= o;
    return *this;
  }

  inline __attribute__((always_inline)) farray &
  operator%=(const T &o)
    requires micron::is_integral_v<T>
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] %= o;
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // compound assignment

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) farray &
  operator+=(const farray<T, M> &o)
  {
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ )
      dst[i] += src[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) farray &
  operator-=(const farray<T, M> &o)
  {
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ )
      dst[i] -= src[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) farray &
  operator*=(const farray<T, M> &o)
  {
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ )
      dst[i] *= src[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) farray &
  operator/=(const farray<T, M> &o)
  {
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ )
      dst[i] /= src[i];
    return *this;
  }

  template <size_type M>
    requires(M <= N && micron::is_integral_v<T>)
  inline __attribute__((always_inline)) farray &
  operator%=(const farray<T, M> &o)
  {
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ )
      dst[i] %= src[i];
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // variadic compound assignment

  template <typename... Rs>
    requires(sizeof...(Rs) >= 2)
  farray &
  operator+=(const Rs &...rs)
  {
    (__apply_add(rs), ...);
    return *this;
  }

  template <typename... Rs>
    requires(sizeof...(Rs) >= 2)
  farray &
  operator-=(const Rs &...rs)
  {
    (__apply_sub(rs), ...);
    return *this;
  }

  template <typename... Rs>
    requires(sizeof...(Rs) >= 2)
  farray &
  operator*=(const Rs &...rs)
  {
    (__apply_mul(rs), ...);
    return *this;
  }

  template <typename... Rs>
    requires(sizeof...(Rs) >= 2)
  farray &
  operator/=(const Rs &...rs)
  {
    (__apply_div(rs), ...);
    return *this;
  }

  template <typename... Rs>
    requires(sizeof...(Rs) >= 2 && micron::is_integral_v<T>)
  farray &
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
    for ( size_type i = 0; i < N; i++ )
      dst[i] *= n;
  }

  void
  div(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] /= n;
  }

  void
  sub(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] -= n;
  }

  void
  add(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] += n;
  }

  T
  mul(void)
  {
    T mul_ = stack[0];
    T *__restrict src = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      mul_ *= src[i];
    return mul_;
  }

  T
  div(void)
  {
    T mul_ = stack[0];
    T *__restrict src = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      mul_ *= src[i];
    return mul_;
  }

  T
  sub(void)
  {
    T mul_ = stack[0];
    T *__restrict src = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      mul_ *= src[i];
    return mul_;
  }

  T
  add(void)
  {
    T mul_ = stack[0];
    T *__restrict src = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      mul_ *= src[i];
    return mul_;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // queries

  bool
  all(const T &o) const noexcept
  {
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] != o )
        return false;
    return true;
  }

  bool
  any(const T &o) const noexcept
  {
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] == o )
        return true;
    return false;
  }

  void
  sqrt(void)
  {
    T *__restrict dst = stack;
    for ( size_type i = 0; i < N; i++ )
      dst[i] = math::sqrt(static_cast<float>(dst[i]));
  }

  template <typename F>
  farray &
  fill(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::ctypeset<N>(micron::addr(stack[0]), o);
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
