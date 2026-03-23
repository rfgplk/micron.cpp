//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__container.hpp"

#include "../__special/initializer_list"
#include "../algorithm/algorithm.hpp"
#include "../algorithm/memory.hpp"
#include "../pointer.hpp"
#include "../slice_forward.hpp"
#include "../types.hpp"
#include "vector.hpp"

#include "../memory/addr.hpp"

namespace micron
{

// svector: vector on the stack, fixed capacity N, mutable.
template <is_regular_object T, usize N = 64, bool Sf = true> class svector
{
  alignas(T) T stack[N];
  usize length = 0;

  inline bool
  __empty_check(void) const
  {
    return length == 0;
  }

  inline bool
  __full_check(void) const
  {
    return length >= N;
  }

  inline bool
  __index_check(usize n) const
  {
    return n >= length;
  }

  inline bool
  __capacity_check(usize n) const
  {
    return n >= N;
  }

  inline bool
  __push_check(void) const
  {
    return length >= N;
  }

  inline bool
  __range_check(usize from, usize to) const
  {
    return from >= to || from >= N || to > N;
  }

  inline bool
  __iterator_check(const T *it) const
  {
    return it < stack || it > stack + length;
  }

  inline bool
  __count_check(usize cnt, usize ind) const
  {
    // checks that ind is valid and there is room for cnt more elements
    return ind >= length || length + cnt > N;
  }

  template <auto Fn, typename E, typename... Args>
  inline __attribute__((always_inline)) void
  __safety_check(const char *msg, Args &&...args) const
  {
    if constexpr ( Sf == true ) {
      if ( (this->*Fn)(micron::forward<Args>(args)...) )
        exc<E>(msg);
    }
  }

public:
  using category_type = vector_tag;
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

  ~svector(void)
  {
    __impl_container::destroy(micron::addr(stack[0]), length);
    length = 0;
  }

  svector(void) : length(0) {}

  template <typename First, typename... Rest>
    requires(micron::is_class_v<T> and !micron::is_integral_v<micron::remove_cvref_t<First>>)
  svector(First first, Rest... rest)
  {
    for ( size_type n = 0; n < N; n++ )
      new (&stack[n]) T{ first, rest... };
    length = N;
  }

  svector(const size_type cnt)
  {
    if ( cnt > N ) [[unlikely]]
      exc<except::library_error>("error micron::svector(): cnt too large");
    __impl_container::construct(micron::addr(stack[0]), T{}, cnt);
    length = cnt;
  }

  template <typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  svector(const size_type n, Fn &&fn)
  {
    if ( n > N ) [[unlikely]]
      exc<except::library_error>("error micron::svector(): cnt too large");
    __impl_container::construct(micron::addr(stack[0]), T{}, n);
    length = n;
    micron::generate(begin(), end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  svector(const size_type n, Fn &&fn)
  {
    if ( n > N ) [[unlikely]]
      exc<except::library_error>("error micron::svector(): cnt too large");
    __impl_container::construct(micron::addr(stack[0]), T{}, n);
    length = n;
    micron::transform(begin(), end(), fn);
  }

  svector(const size_type cnt, const T &v)
  {
    if ( cnt > N ) [[unlikely]]
      exc<except::library_error>("error micron::svector(): cnt too large");
    __impl_container::construct(micron::addr(stack[0]), v, cnt);
    length = cnt;
  }

  template <typename C = T>
    requires(sizeof(C) == sizeof(T))
  svector(const vector<C> &o)
  {
    const usize copy_n = (o.length >= N) ? N : o.length;
    __impl_container::copy(stack, o.memory, copy_n);
    length = copy_n;
  }

  svector(const svector &o)
  {
    __impl_container::copy(stack, o.stack, o.length);
    length = o.length;
  }

  template <typename C = T, size_type M = N> svector(const svector<C, M> &o)
  {
    __impl_container::copy(stack, o.stack, o.length);
    length = o.length < N ? o.length : N;
  }

  svector(const std::initializer_list<T> &lst)
  {
    if ( lst.size() > N )
      exc<except::runtime_error>("error micron::svector() initializer_list too large.");

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      size_type i = 0;
      for ( const T &value : lst )
        new (&stack[i++]) T(value);
    } else {
      size_type i = 0;
      for ( T value : lst )
        stack[i++] = value;
    }
    length = lst.size();
  }

  svector(svector &&o)
  {
    __impl_container::move(micron::real_addr_as<T>(stack[0]), micron::real_addr_as<T>(o.stack[0]), o.length);
    length = o.length;
    o.length = 0;
  }

  template <typename C = T, size_type M> svector(svector<C, M> &&o)
  {
    const usize mv_n = (M < N) ? M : N;
    __impl_container::move<N>(stack, o.stack);
    length = mv_n;
    o.length = 0;
  }

  svector &
  operator=(const svector &o)
  {
    __impl_container::copy_assign(stack, o.stack, o.length);
    length = o.length;
    return *this;
  }

  svector &
  operator=(svector &&o)
  {
    __impl_container::move_assign(stack, o.stack, o.length);
    length = o.length;
    o.length = 0;
    return *this;
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  T &
  operator[](const R n)
  {
    return stack[n];
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  const T &
  operator[](const R n) const
  {
    return stack[n];
  }

  T &
  at(const size_type n)
  {
    __safety_check<&svector::__index_check, except::runtime_error>("micron::svector at() out of range.", n);
    return stack[n];
  }

  const T &
  at(const size_type n) const
  {
    __safety_check<&svector::__index_check, except::runtime_error>("micron::svector at() out of range.", n);
    return stack[n];
  }

  T &
  front(void)
  {
    __safety_check<&svector::__empty_check, except::runtime_error>("micron::svector front() called on empty vector");
    return stack[0];
  }

  const T &
  front(void) const
  {
    __safety_check<&svector::__empty_check, except::runtime_error>("micron::svector front() called on empty vector");
    return stack[0];
  }

  T &
  back(void)
  {
    __safety_check<&svector::__empty_check, except::runtime_error>("micron::svector back() called on empty vector");
    return stack[length - 1];
  }

  const T &
  back(void) const
  {
    __safety_check<&svector::__empty_check, except::runtime_error>("micron::svector back() called on empty vector");
    return stack[length - 1];
  }

  iterator
  begin(void)
  {
    return micron::real_addr_as<T>(stack[0]);
  }

  const_iterator
  begin(void) const
  {
    return micron::real_addr_as<T>(stack[0]);
  }

  const_iterator
  cbegin(void) const
  {
    return micron::real_addr_as<T>(stack[0]);
  }

  iterator
  end(void)
  {
    return micron::real_addr_as<T>(stack[length]);
  }

  const_iterator
  end(void) const
  {
    return micron::real_addr_as<T>(stack[length]);
  }

  const_iterator
  cend(void) const
  {
    return micron::real_addr_as<T>(stack[length]);
  }

  // full when no more elements can be appended (length == N)
  inline bool
  full(void) const
  {
    return length >= N;
  }

  inline bool
  overflowed(void) const
  {
    return length > N;
  }

  inline bool
  full_or_overflowed(void) const
  {
    return length >= N;
  }

  size_type
  size(void) const
  {
    return length;
  }

  size_type
  max_size(void) const
  {
    return N;
  }

  void
  __set_size(size_type s)
  {
    length = s;
  }

  void
  fast_clear(void)
  {
    if constexpr ( !micron::is_class_v<T> )
      length = 0;
    else
      clear();
  }

  void
  clear(void)
  {
    __impl_container::destroy(micron::real_addr_as<T>(stack), length);
    length = 0;
  }

  iterator
  data(void)
  {
    return &stack[0];
  }

  const_iterator
  data(void) const
  {
    return &stack[0];
  }

  bool
  operator!(void) const
  {
    return empty();
  }

  bool
  empty(void) const noexcept
  {
    return length == 0;
  }

  byte *
  operator&(void)
  {
    return reinterpret_cast<byte *>(stack);
  }

  const byte *
  operator&(void) const
  {
    return reinterpret_cast<const byte *>(stack);
  }

  auto *
  addr(void)
  {
    return this;
  }

  const auto *
  addr(void) const
  {
    return this;
  }

  inline slice<T>
  operator[](void)
  {
    return slice<T>(begin(), end());
  }

  inline const slice<T>
  operator[](void) const
  {
    return slice<T>(begin(), end());
  }

  inline __attribute__((always_inline)) const slice<T>
  operator[](size_type from, size_type to) const
  {
    __safety_check<&svector::__range_check, except::library_error>("micron::svector operator[] out of allocated memory range.", from, to);
    return slice<T>(stack + from, stack + to);
  }

  inline __attribute__((always_inline)) slice<T>
  operator[](size_type from, size_type to)
  {
    __safety_check<&svector::__range_check, except::library_error>("micron::svector operator[] out of allocated memory range.", from, to);
    return slice<T>(stack + from, stack + to);
  }

  void resize(const size_type n) = delete;
  void reserve(const size_type n) = delete;

  svector &
  erase(size_type n)
  {
    __safety_check<&svector::__index_check, except::runtime_error>("micron::svector erase() out of range.", n);
    stack[n].~T();
    micron::memmove(micron::real_addr_as<T>(stack[n]), micron::real_addr_as<T>(stack[n + 1]), (length - n - 1));
    __impl_container::destroy(micron::real_addr_as<T>(stack[length - 1]), 1);
    --length;
    return *this;
  }

  template <typename C = T, size_type M>
    requires(sizeof(C) == sizeof(T))
  svector &
  append(const svector<C, M> &o)
  {
    if ( length + o.size() > N )
      exc<except::runtime_error>("micron::svector append() out of range.");
    for ( size_type i = length, j = 0; j < o.size(); i++, j++ )
      stack[i] = o[j];
    length += o.size();
    return *this;
  }

  template <typename C = T, size_type M>
    requires(sizeof(C) == sizeof(T))
  svector &
  operator+=(const svector<C, M> &o)
  {
    append(o);
    return *this;
  }

  template <typename... Args>
  svector &
  emplace_back(Args &&...args)
  {
    __safety_check<&svector::__push_check, except::runtime_error>("micron::svector emplace_back() out of range.");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> )
      new (&stack[length]) T(micron::forward<Args>(args)...);
    else
      stack[length] = T(micron::forward<Args>(args)...);
    ++length;
    return *this;
  }

  svector &
  move_back(T &&i)
  {
    __safety_check<&svector::__push_check, except::runtime_error>("micron::svector move_back() out of range.");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> )
      new (&stack[length]) T(micron::move(i));
    else
      stack[length] = micron::move(i);
    ++length;
    return *this;
  }

  template <typename C = T>
  svector &
  push_back(const C &i)
  {
    __safety_check<&svector::__push_check, except::runtime_error>("micron::svector push_back() out of range.");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> )
      new (&stack[length]) T(i);
    else
      stack[length] = i;
    ++length;
    return *this;
  }

  template <typename C = T>
  svector &
  append(const C &i)
  {
    return push_back(i);
  }

  template <typename C = T>
  svector &
  insert(size_type ind, const C &i)
  {
    __safety_check<&svector::__push_check, except::runtime_error>("micron::svector insert() out of range.");
    if ( ind > length )
      exc<except::runtime_error>("micron::svector insert() index past end.");
    micron::bytemove(&stack[ind + 1], &stack[ind], (length - ind) * sizeof(T));
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> )
      new (&stack[ind]) T(i);
    else
      stack[ind] = i;
    ++length;
    return *this;
  }

  template <typename C = T>
  svector &
  insert(iterator itr, const C &i)
  {
    __safety_check<&svector::__push_check, except::runtime_error>("micron::svector insert() out of range.");
    __safety_check<&svector::__iterator_check, except::runtime_error>("micron::svector insert() iterator out of range.",
                                                                      static_cast<const T *>(itr));
    const usize tail_bytes = static_cast<usize>(end() - itr) * sizeof(T);
    micron::bytemove(itr + 1, itr, tail_bytes);
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> )
      new (itr) T(i);
    else
      *itr = i;
    ++length;
    return *this;
  }

  inline iterator
  get(const size_type n)
  {
    __safety_check<&svector::__capacity_check, except::library_error>("micron::svector get() out of range", n);
    return micron::addr(stack[n]);
  }

  inline const_iterator
  get(const size_type n) const
  {
    __safety_check<&svector::__capacity_check, except::library_error>("micron::svector get() out of range", n);
    return micron::addr(stack[n]);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    __safety_check<&svector::__capacity_check, except::library_error>("micron::svector cget() out of range", n);
    return micron::addr(stack[n]);
  }

  static constexpr bool
  is_pod(void)
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class_type(void) noexcept
  {
    return micron::is_class_v<T>;
  }

  static constexpr bool
  is_trivial(void) noexcept
  {
    return micron::is_trivial_v<T>;
  }
};

}     // namespace micron
