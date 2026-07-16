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

#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{

// general purpose concurrent array class, stack allocated, thread-safe, mutable.
// default to 64
template<is_regular_object T, usize N = 64>
  requires(N > 0 and ((N * sizeof(T)) < (1 << 22)))      // avoid weird stuff with N = 0
class conarray
{
  mutable micron::fast_mutex __mtx;

  using __defer = micron::unique_lock<micron::lock_starts::defer, micron::fast_mutex>;

  struct __hold {
    micron::fast_mutex &m;

    [[gnu::always_inline]] explicit __hold(micron::fast_mutex &mm) noexcept : m(mm) { m.lock(); }

    [[gnu::always_inline]] ~__hold() noexcept { m.unlock(); }

    __hold(const __hold &) = delete;
    __hold &operator=(const __hold &) = delete;
  };

  static inline void
  __lock_ordered(micron::fast_mutex &a, __defer &la, micron::fast_mutex &b, __defer &lb)
  {
    if ( micron::addr(a) == micron::addr(b) ) {
      la.lock();
    } else if ( static_cast<const void *>(micron::addr(a)) < static_cast<const void *>(micron::addr(b)) ) {
      la.lock();
      lb.lock();
    } else {
      lb.lock();
      la.lock();
    }
  }

  // must be in an anonymous union
  union {
    alignas(64) T stack[N];
  } __attribute__((__may_alias__));

  template<typename U>
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

  template<typename U>
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

  template<typename U>
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

  template<typename U>
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

  template<typename U>
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
  static constexpr size_type static_size = length;

  ~conarray() { __impl_container::destroy<N, T>(micron::addr(stack[0])); }

  conarray() { __impl_container::construct<N, T>(micron::addr(stack[0]), T{}); }

  template<typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  conarray(Fn &&fn)
  {
    __impl_container::construct<N, T>(micron::addr(stack[0]), T{});
    micron::generate(begin_unsafe(), end_unsafe(), fn);
  }

  template<typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  conarray(Fn &&fn)
  {
    __impl_container::construct<N, T>(micron::addr(stack[0]), T{});
    micron::transform(begin_unsafe(), end_unsafe(), fn);
  }

  conarray(const T &o) { __impl_container::construct<N, T>(micron::addr(stack[0]), o); }

  conarray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N ) exc<except::runtime_error>("micron::conarray conarray(init_list): init_list too large.");
    size_type i = 0;
    for ( auto &&value : lst ) new (micron::addr(stack[i++])) T(micron::move(value));
    if ( lst.size() < N ) __impl_container::construct(micron::addr(stack[lst.size()]), T{}, N - lst.size());
  }

  template<is_container A>
    requires(!micron::is_same_v<A, conarray>)
  conarray(A &&o)
  {
    if ( o.size() < N ) exc<except::runtime_error>("micron::conarray conarray(A&&) invalid size");
    if constexpr ( micron::is_rvalue_reference_v<A &&> )
      __impl_container::move_init<N, T>(micron::addr(stack[0]), o.begin());
    else
      __impl_container::copy<N, T>(micron::addr(stack[0]), o.begin());
  }

  conarray(const slice<T> &o)
  {
    const size_type bound = o.size() < N ? o.size() : N;
    __impl_container::copy(micron::addr(stack[0]), o.begin(), bound);
    if ( bound < N ) __impl_container::construct(micron::addr(stack[bound]), T{}, N - bound);
  }

  conarray(const conarray &o)
  {
    __hold __lock(o.__mtx);
    __impl_container::copy<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
  }

  conarray(conarray &&o)
  {
    __hold __lock(o.__mtx);
    __impl_container::move_init<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
  }

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

  [[nodiscard]] const_iterator
  data() const noexcept
  {
    return stack;
  }

  const_iterator
  view() const noexcept
  {
    return micron::addr(stack[0]);
  }

  // WARNING: exception-UNSAFE manual exclusive lock; release() must be called manually
  iterator
  get()
  {
    __mtx.lock();
    return micron::addr(stack[0]);
  }

  void
  release()
  {
    __mtx.unlock();
  }

  // RAII exclusive accessor
  struct scope {
    conarray &__c;

    [[gnu::always_inline]] explicit scope(conarray &c) noexcept : __c(c) { __c.__mtx.lock(); }

    [[gnu::always_inline]] ~scope() noexcept { __c.__mtx.unlock(); }

    scope(const scope &) = delete;
    scope &operator=(const scope &) = delete;

    [[nodiscard]] iterator
    data() noexcept
    {
      return micron::addr(__c.stack[0]);
    }

    [[nodiscard]] T &
    operator[](size_type i) noexcept
    {
      return __c.stack[i];
    }

    [[nodiscard]] size_type
    size() const noexcept
    {
      return N;
    }
  };

  [[nodiscard]] scope
  locked() noexcept
  {
    return scope(*this);
  }

  void
  clear()
  {
    __hold __lock(__mtx);
    __impl_container::destroy<N, T>(micron::addr(stack[0]));
    __impl_container::construct<N, T>(micron::addr(stack[0]), T{});
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // element access

  T
  at(const size_type i) const
  {
    if ( i >= N ) exc<except::library_error>("micron::conarray at() out of range.");
    __hold __lock(__mtx);
    return stack[i];
  }

  void
  at(const size_type i, const T &val)
  {
    if ( i >= N ) exc<except::library_error>("micron::conarray at() out of range.");
    __hold __lock(__mtx);
    stack[i] = val;
  }

  T
  front() const
  {
    __hold __lock(__mtx);
    return stack[0];
  }

  T
  back() const
  {
    __hold __lock(__mtx);
    return stack[N - 1];
  }

  inline const_iterator
  get(const size_type n) const
  {
    if ( n >= N ) exc<except::library_error>("micron::conarray get() out of range");
    return stack + n;
  }

  inline const_iterator
  cget(const size_type n) const
  {
    if ( n >= N ) exc<except::library_error>("micron::conarray cget() out of range");
    return stack + n;
  }

  // WARNING: UNSYNCHRONIZED raw access
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

  inline slice<T>
  operator[]()
  {
    return slice<T>(begin_unsafe(), end_unsafe());
  }

  inline const slice<T>
  operator[]() const
  {
    return slice<T>(begin(), end());
  }

  inline __attribute__((always_inline)) const slice<T>
  operator[](size_type from, size_type to) const
  {
    if ( from >= to or from > N or to > N ) exc<except::library_error>("micron::conarray operator[] out of allocated memory range.");
    return slice<T>(get(from), get(to));
  }

  inline __attribute__((always_inline)) slice<T>
  operator[](size_type from, size_type to)
  {
    if ( from >= to or from > N or to > N ) exc<except::library_error>("micron::conarray operator[] out of allocated memory range.");
    return slice<T>(get(from), get(to));
  }

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

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%5
  // assignment

  template<size_type M>
  conarray &
  operator=(T (&o)[M])
    requires(M <= N)
  {
    __hold __lock(__mtx);
    __impl_container::copy_assign<M, T>(micron::addr(stack[0]), micron::addr(o[0]));
    return *this;
  }

  template<typename F>
  conarray &
  operator=(const F &o)
    requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<T>)
  {
    __hold __lock(__mtx);
    micron::ctypeset<N>(micron::addr(stack[0]), static_cast<T>(o));
    return *this;
  }

  template<is_constexpr_container A>
    requires(micron::has_static_size<A>)
  conarray &
  operator=(const A &o)
  {
    __hold __lock(__mtx);
    if constexpr ( N <= A::static_size )
      __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o[0]));
    else
      __impl_container::copy_assign<A::static_size, T>(micron::addr(stack[0]), micron::addr(o[0]));
    return *this;
  }

  // source sized only at runtime
  template<is_container A>
    requires(!micron::is_same_v<A, conarray> and !micron::has_static_size<A>)
  conarray &
  operator=(const A &o)
  {
    __hold __lock(__mtx);
    if ( N <= o.size() )
      __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o[0]));
    else
      __impl_container::copy_assign(micron::addr(stack[0]), micron::addr(o[0]), o.size());
    return *this;
  }

  conarray &
  operator=(const conarray &o)
  {
    if ( this == &o ) [[unlikely]]
      return *this;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    return *this;
  }

  conarray &
  operator=(conarray &&o)
  {
    if ( this == &o ) [[unlikely]]
      return *this;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    __impl_container::move_assign<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // binary arithmetic

  template<size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) conarray
  operator+(const conarray<T, M> &o) const
  {
    conarray arr;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    T *__restrict dst = arr.stack;
    const T *__restrict me = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] = me[i] + src[i];
    return arr;
  }

  template<size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) conarray
  operator-(const conarray<T, M> &o) const
  {
    conarray arr;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    T *__restrict dst = arr.stack;
    const T *__restrict me = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] = me[i] - src[i];
    return arr;
  }

  template<size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) conarray
  operator*(const conarray<T, M> &o) const
  {
    conarray arr;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    T *__restrict dst = arr.stack;
    const T *__restrict me = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] = me[i] * src[i];
    return arr;
  }

  template<size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) conarray
  operator/(const conarray<T, M> &o) const
  {
    conarray arr;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    T *__restrict dst = arr.stack;
    const T *__restrict me = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] = me[i] / src[i];
    return arr;
  }

  template<size_type M>
    requires(M <= N && micron::is_integral_v<T>)
  inline __attribute__((always_inline)) conarray
  operator%(const conarray<T, M> &o) const
  {
    conarray arr;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    T *__restrict dst = arr.stack;
    const T *__restrict me = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] = me[i] % src[i];
    return arr;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // reductions

  T
  sum(void) const
  {
    __hold __lock(__mtx);
    const T *__restrict src = stack;
    T sm = T{};
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) sm += src[i];
    return sm;
  }

  T
  mul_reduce(void) const
  {
    __hold __lock(__mtx);
    const T *__restrict src = stack;
    T m = src[0];
#pragma GCC ivdep
    for ( size_type i = 1; i < N; i++ ) m *= src[i];
    return m;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // compound assignment

  inline __attribute__((always_inline)) conarray &
  operator+=(const T &o)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] += o;
    return *this;
  }

  inline __attribute__((always_inline)) conarray &
  operator-=(const T &o)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] -= o;
    return *this;
  }

  inline __attribute__((always_inline)) conarray &
  operator*=(const T &o)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] *= o;
    return *this;
  }

  inline __attribute__((always_inline)) conarray &
  operator/=(const T &o)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] /= o;
    return *this;
  }

  inline __attribute__((always_inline)) conarray &
  operator%=(const T &o)
    requires micron::is_integral_v<T>
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] %= o;
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // compound assignment

  template<size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) conarray &
  operator+=(const conarray<T, M> &o)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] += src[i];
    return *this;
  }

  template<size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) conarray &
  operator-=(const conarray<T, M> &o)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] -= src[i];
    return *this;
  }

  template<size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) conarray &
  operator*=(const conarray<T, M> &o)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] *= src[i];
    return *this;
  }

  template<size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) conarray &
  operator/=(const conarray<T, M> &o)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] /= src[i];
    return *this;
  }

  template<size_type M>
    requires(M <= N && micron::is_integral_v<T>)
  inline __attribute__((always_inline)) conarray &
  operator%=(const conarray<T, M> &o)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
    const T *__restrict src = o.stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < M; i++ ) dst[i] %= src[i];
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // variadic compound assignment

  template<typename... Rs>
    requires(sizeof...(Rs) >= 2)
  conarray &
  operator+=(const Rs &...rs)
  {
    __hold __lock(__mtx);
    (__apply_add(rs), ...);
    return *this;
  }

  template<typename... Rs>
    requires(sizeof...(Rs) >= 2)
  conarray &
  operator-=(const Rs &...rs)
  {
    __hold __lock(__mtx);
    (__apply_sub(rs), ...);
    return *this;
  }

  template<typename... Rs>
    requires(sizeof...(Rs) >= 2)
  conarray &
  operator*=(const Rs &...rs)
  {
    __hold __lock(__mtx);
    (__apply_mul(rs), ...);
    return *this;
  }

  template<typename... Rs>
    requires(sizeof...(Rs) >= 2)
  conarray &
  operator/=(const Rs &...rs)
  {
    __hold __lock(__mtx);
    (__apply_div(rs), ...);
    return *this;
  }

  template<typename... Rs>
    requires(sizeof...(Rs) >= 2 && micron::is_integral_v<T>)
  conarray &
  operator%=(const Rs &...rs)
  {
    __hold __lock(__mtx);
    (__apply_mod(rs), ...);
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // named in-place arithmetic

  void
  mul(const size_type n)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] *= n;
  }

  void
  div(const size_type n)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] /= n;
  }

  void
  sub(const size_type n)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] -= n;
  }

  void
  add(const size_type n)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ ) dst[i] += n;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // queries

  bool
  all(const T &o) const
  {
    __hold __lock(__mtx);
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] != o ) return false;
    return true;
  }

  bool
  any(const T &o) const
  {
    __hold __lock(__mtx);
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] == o ) return true;
    return false;
  }

  void
  sqrt(void)
  {
    __hold __lock(__mtx);
    T *__restrict dst = stack;
    for ( size_type i = 0; i < N; i++ ) {
      if constexpr ( micron::is_floating_point_v<T> )
        dst[i] = static_cast<T>(math::sqrt(dst[i]));      // no narrowing through float
      else
        dst[i] = static_cast<T>(math::sqrt(static_cast<double>(dst[i])));
    }
  }

  template<typename F>
  conarray &
  fill(const F &o)
    requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<T>)
  {
    __hold __lock(__mtx);
    micron::ctypeset<N>(micron::addr(stack[0]), static_cast<T>(o));
    return *this;
  }

  template<typename F>
  conarray &
  fill(const F &o)
  {
    __hold __lock(__mtx);
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

private:
  iterator
  begin_unsafe() noexcept
  {
    return micron::addr(stack[0]);
  }

  iterator
  end_unsafe() noexcept
  {
    return micron::addr(stack[N]);
  }
};
};      // namespace micron
