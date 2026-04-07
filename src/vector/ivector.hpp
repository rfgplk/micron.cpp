//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../bits/__container.hpp"

#include "../algorithm/algorithm.hpp"
#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../container_safety.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/memory.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"
#include "../tags.hpp"
#include "../types.hpp"

#include "vector.hpp"

namespace micron
{
// immutable vector class, contiguous in memory, O(1) access, iterators never invalidated, always safe, immutable, always thread safe as
template <is_movable_object T, class Alloc = micron::allocator_serial<>, bool Sf = true>
class ivector : private Alloc, public __immutable_memory_resource<T, Alloc>
{
  using __mem = __immutable_memory_resource<T, Alloc>;

  inline __attribute__((always_inline)) bool
  __empty_check(void) const
  {
    return __mem::length == 0 || __mem::memory == nullptr;
  }

  inline __attribute__((always_inline)) bool
  __null_check(void) const
  {
    return __mem::memory == nullptr;
  }

  inline __attribute__((always_inline)) bool
  __index_check(size_t n) const
  {
    return n >= __mem::length;
  }

  inline __attribute__((always_inline)) bool
  __capacity_check(size_t n) const
  {
    return n >= __mem::capacity;
  }

  inline __attribute__((always_inline)) bool
  __iterator_check(const T *it) const
  {
    return it < __mem::memory || it > __mem::memory + __mem::length;
  }

  inline __attribute__((always_inline)) bool
  __iterator_strict(const T *it) const
  {
    return it < __mem::memory || it >= __mem::memory + __mem::length;
  }

  inline __attribute__((always_inline)) bool
  __range_check(size_t from, size_t to) const
  {
    return from >= to || to > __mem::length;
  }

  inline __attribute__((always_inline)) bool
  __cap_range_check(size_t from, size_t to) const
  {
    return from >= to || from >= __mem::capacity || to > __mem::capacity;
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

  struct __cap_tag {
  };

  ivector(__cap_tag, usize n) : __mem(n) { __mem::length = 0; }

  // grow container, private
  inline void
  reserve(usize n)
  {
    if ( n < __mem::capacity )
      return;
    if ( __mem::is_zero() ) {
      __mem::realloc(n);
      return;
    }
    __mem::expand(n);
  }

  inline void
  __clear(void)
  {
    if ( !__mem::length )
      return;
    __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::length);
    __mem::length = 0;
  }

  typedef T *iterator;

  // mutable iterators
  inline iterator
  __begin(void)
  {
    return __mem::memory;
  }

  inline iterator
  __end(void)
  {
    return __mem::memory + __mem::length;
  }

public:
  using category_type = vector_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef usize size_type;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;

  typedef const T *const_iterator;

  ivector() = delete;

  ~ivector(void)
  {
    if ( __mem::is_zero() )
      return;
    __clear();
  }

  ivector(usize n) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
  }

  template <typename... Args>
    requires(sizeof...(Args) > 1 and micron::is_class_v<T>)
  ivector(usize n, Args &&...args) : __mem(n)
  {
    for ( usize i = 0; i < n; i++ )
      new (micron::addr(__mem::memory[i])) T(forward<Args>(args)...);
    __mem::length = n;
  }

  ivector(usize n, const T &init_value) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), init_value, n);
    __mem::length = n;
  }

  ivector(usize n, T &&init_value) : __mem(n)
  {
    T tmp = micron::move(init_value);
    __impl_container::construct(micron::addr(__mem::memory[0]), tmp, n);
    __mem::length = n;
  }

  template <typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  ivector(usize n, Fn &&fn) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
    micron::generate(__begin(), __end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  ivector(usize n, Fn &&fn) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
    micron::transform(__begin(), __end(), fn);
  }

  // initializer list
  ivector(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      usize i = 0;
      for ( T &&value : lst )
        new (micron::addr(__mem::memory[i++])) T(micron::move(value));
      __mem::length = lst.size();
    } else {
      usize i = 0;
      for ( auto &&value : lst )
        __mem::memory[i++] = value;
      __mem::length = lst.size();
    }
  }

  // copy
  ivector(const ivector &o) : __mem(o.capacity())
  {
    __impl_container::copy(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
  }

  ivector &
  operator=(const ivector &o)
  {
    if ( o.capacity() >= __mem::capacity )
      reserve(o.capacity());
    __impl_container::copy_assign(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
    return *this;
  }

  // from mutable vector
  ivector(const vector<T> &o) : __mem(o.size())
  {
    __impl_container::copy(__mem::memory, o.data(), o.size());
    __mem::length = o.size();
  }

  // move
  ivector(chunk<byte> &&m) : __mem(m) { m = nullptr; }

  template <typename C = T> ivector(ivector<C> &&o) : __mem(micron::move(o)) {}

  ivector(ivector &&o) : __mem(micron::move(o)) {}

  ivector &
  operator=(ivector &&o)
  {
    if ( __mem::memory ) {
      __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::length);
    }
    __mem::operator=(micron::move(o));
    return *this;
  }

  chunk<byte>
  operator*(void) const
  {
    return { reinterpret_cast<byte *>(__mem::memory), __mem::capacity };
  }

  const_pointer
  data(void) const
  {
    return __mem::memory;
  }

  bool
  operator!(void) const
  {
    return empty();
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  inline __attribute__((always_inline)) const T &
  operator[](R n) const
  {
    __safety_check<&ivector::__capacity_check, except::library_error>("micron::ivector operator[] out of allocated memory range.",
                                                                      static_cast<size_type>(n));
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) const slice<T>
  operator[](size_type from, size_type to) const
  {
    __safety_check<&ivector::__cap_range_check, except::library_error>("micron::ivector operator[] out of allocated memory range.", from,
                                                                       to);
    return slice<T>(__mem::memory + from, __mem::memory + to);
  }

  inline __attribute__((always_inline)) const T &
  at(size_type n) const
  {
    __safety_check<&ivector::__index_check, except::library_error>("micron::ivector at() out of bounds", n);
    return (__mem::memory)[n];
  }

  size_type
  at_n(const_iterator i) const
  {
    __safety_check<&ivector::__iterator_check, except::library_error>("micron::ivector at_n() iterator out of range",
                                                                      static_cast<const T *>(i));
    return static_cast<size_type>(i - begin());
  }

  const_iterator
  itr(size_type n) const
  {
    __safety_check<&ivector::__index_check, except::library_error>("micron::ivector itr() out of bounds", n);
    return &(__mem::memory)[n];
  }

  inline const_iterator
  get(const size_type n) const
  {
    __safety_check<&ivector::__index_check, except::library_error>("micron::ivector get() out of range", n);
    return &(__mem::memory[n]);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    __safety_check<&ivector::__index_check, except::library_error>("micron::ivector cget() out of range", n);
    return &(__mem::memory[n]);
  }

  inline const_iterator
  find(const T &o) const
  {
    const T *f_ptr = __mem::memory;
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }

  size_type
  capacity(void) const
  {
    return __mem::capacity;
  }

  size_type
  max_size(void) const
  {
    return __mem::capacity;
  }

  size_type
  size(void) const
  {
    return __mem::length;
  }

  bool
  empty(void) const
  {
    return __mem::length == 0;
  }

  inline const T &
  front(void) const
  {
    __safety_check<&ivector::__empty_check, except::library_error>("micron::ivector front() called on empty vector");
    return (__mem::memory)[0];
  }

  inline const T &
  back(void) const
  {
    __safety_check<&ivector::__empty_check, except::library_error>("micron::ivector back() called on empty vector");
    return (__mem::memory)[__mem::length - 1];
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

  inline const_iterator
  begin(void) const
  {
    return __mem::memory;
  }

  inline const_iterator
  cbegin(void) const
  {
    return __mem::memory;
  }

  inline const_iterator
  end(void) const
  {
    return __mem::memory + __mem::length;
  }

  inline const_iterator
  cend(void) const
  {
    return __mem::memory + __mem::length;
  }

  inline const_iterator
  last(void) const
  {
    __safety_check<&ivector::__empty_check, except::library_error>("micron::ivector last() called on empty vector");
    return __mem::memory + (__mem::length - 1);
  }

  inline slice<byte>
  into_bytes(void) const
  {
    if ( __mem::memory == nullptr || __mem::length == 0 )
      return slice<byte>(nullptr, nullptr);
    return slice<byte>(reinterpret_cast<const byte *>(&__mem::memory[0]), reinterpret_cast<const byte *>(&__mem::memory[__mem::length]));
  }

  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline ivector<T, Alloc, Sf>
  append(const ivector<F, Alloc, Sf> &o) const
  {
    ivector<T, Alloc, Sf> buf(__cap_tag{}, __mem::length + o.length);
    __impl_container::copy(&buf.memory[0], __mem::memory, __mem::length);
    __impl_container::copy(&buf.memory[__mem::length], o.memory, o.length);
    buf.length = __mem::length + o.length;
    return buf;
  }

  ivector
  operator+(const ivector &o) const
  {
    return append(o);
  }

  template <typename C = T>
  void
  swap(ivector<C, Alloc, Sf> &&o)
  {
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
  }

  inline ivector
  push_back(const T &v) const
  {
    ivector buf(__cap_tag{}, __mem::length + 1);
    __impl_container::copy(&buf.memory[0], __mem::memory, __mem::length);
    new (micron::addr(buf.memory[__mem::length])) T(v);
    buf.length = __mem::length + 1;
    return buf;
  }

  inline ivector
  push_back(T &&v) const
  {
    ivector buf(__cap_tag{}, __mem::length + 1);
    __impl_container::copy(&buf.memory[0], __mem::memory, __mem::length);
    new (micron::addr(buf.memory[__mem::length])) T(micron::move(v));
    buf.length = __mem::length + 1;
    return buf;
  }

  inline ivector
  push_front(const T &v) const
  {
    ivector buf(__cap_tag{}, __mem::length + 1);
    new (micron::addr(buf.memory[0])) T(v);
    __impl_container::copy(&buf.memory[1], __mem::memory, __mem::length);
    buf.length = __mem::length + 1;
    return buf;
  }

  inline ivector
  push_front(T &&v) const
  {
    ivector buf(__cap_tag{}, __mem::length + 1);
    new (micron::addr(buf.memory[0])) T(micron::move(v));
    __impl_container::copy(&buf.memory[1], __mem::memory, __mem::length);
    buf.length = __mem::length + 1;
    return buf;
  }

  template <typename... Args>
  inline ivector
  emplace_back(Args &&...v) const
  {
    ivector buf(__cap_tag{}, __mem::length + 1);
    __impl_container::copy(&buf.memory[0], __mem::memory, __mem::length);
    new (micron::addr(buf.memory[__mem::length])) T(micron::forward<Args>(v)...);
    buf.length = __mem::length + 1;
    return buf;
  }

  inline ivector
  insert(size_type n, const T &val) const
  {
    __safety_check<&ivector::__index_check, except::library_error>("micron::ivector insert() out of bounds", n);
    ivector buf(__cap_tag{}, __mem::length + 1);
    // copy [0, n)
    if ( n > 0 )
      __impl_container::copy(&buf.memory[0], __mem::memory, n);
    // place new element
    new (micron::addr(buf.memory[n])) T(val);
    // copy [n, length)
    if ( n < __mem::length )
      __impl_container::copy(&buf.memory[n + 1], &__mem::memory[n], __mem::length - n);
    buf.length = __mem::length + 1;
    return buf;
  }

  inline ivector
  insert(size_type n, T &&val) const
  {
    __safety_check<&ivector::__index_check, except::library_error>("micron::ivector insert() out of bounds", n);
    ivector buf(__cap_tag{}, __mem::length + 1);
    if ( n > 0 )
      __impl_container::copy(&buf.memory[0], __mem::memory, n);
    new (micron::addr(buf.memory[n])) T(micron::move(val));
    if ( n < __mem::length )
      __impl_container::copy(&buf.memory[n + 1], &__mem::memory[n], __mem::length - n);
    buf.length = __mem::length + 1;
    return buf;
  }

  inline ivector
  insert(const_iterator it, const T &val) const
  {
    size_type n = static_cast<size_type>(it - begin());
    return insert(n, val);
  }

  inline ivector
  insert(const_iterator it, T &&val) const
  {
    size_type n = static_cast<size_type>(it - begin());
    return insert(n, micron::move(val));
  }

  inline ivector
  insert(size_type n, const T &val, size_type cnt) const
  {
    __safety_check<&ivector::__index_check, except::library_error>("micron::ivector insert() out of bounds", n);
    ivector buf(__cap_tag{}, __mem::length + cnt);
    if ( n > 0 )
      __impl_container::copy(&buf.memory[0], __mem::memory, n);
    for ( size_type i = 0; i < cnt; ++i )
      new (micron::addr(buf.memory[n + i])) T(val);
    if ( n < __mem::length )
      __impl_container::copy(&buf.memory[n + cnt], &__mem::memory[n], __mem::length - n);
    buf.length = __mem::length + cnt;
    return buf;
  }

  inline ivector
  assign(const size_type cnt, const T &val) const
  {
    ivector buf(__cap_tag{}, cnt);
    __impl_container::construct(micron::addr(buf.memory[0]), val, cnt);
    buf.length = cnt;
    return buf;
  }

  inline ivector
  erase(size_type n) const
  {
    __safety_check<&ivector::__index_check, except::library_error>("micron::ivector erase() out of bounds", n);
    ivector buf(__cap_tag{}, __mem::length - 1);
    // copy [0, n)
    if ( n > 0 )
      __impl_container::copy(&buf.memory[0], __mem::memory, n);
    // copy [n+1, length)
    if ( n + 1 < __mem::length )
      __impl_container::copy(&buf.memory[n], &__mem::memory[n + 1], __mem::length - n - 1);
    buf.length = __mem::length - 1;
    return buf;
  }

  inline ivector
  erase(const_iterator it) const
  {
    size_type n = static_cast<size_type>(it - begin());
    return erase(n);
  }

  inline ivector
  erase(size_type from, size_type to) const
  {
    __safety_check<&ivector::__range_check, except::library_error>("micron::ivector erase() invalid range", from, to);
    size_type count = to - from;
    ivector buf(__cap_tag{}, __mem::length - count);
    // copy [0, from)
    if ( from > 0 )
      __impl_container::copy(&buf.memory[0], __mem::memory, from);
    // copy [to, length)
    if ( to < __mem::length )
      __impl_container::copy(&buf.memory[from], &__mem::memory[to], __mem::length - to);
    buf.length = __mem::length - count;
    return buf;
  }

  inline ivector
  erase(const_iterator first, const_iterator last) const
  {
    size_type from = static_cast<size_type>(first - begin());
    size_type to = static_cast<size_type>(last - begin());
    return erase(from, to);
  }

  inline ivector
  pop_back(void) const
  {
    __safety_check<&ivector::__empty_check, except::library_error>("micron::ivector pop_back() called on empty vector");
    ivector buf(__cap_tag{}, __mem::length - 1);
    __impl_container::copy(&buf.memory[0], __mem::memory, __mem::length - 1);
    buf.length = __mem::length - 1;
    return buf;
  }

  inline ivector
  clear(void) const
  {
    return ivector(__cap_tag{}, __mem::capacity);
  }
};

template <typename T>
auto
to_persist(const micron::vector<T> &vec)
{
  return ivector<T>(vec);
}

};     // namespace micron
