//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../bits/__container.hpp"

#include "vector.hpp"

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
#include "../sort/heap.hpp"
#include "../sort/quick.hpp"
#include "../tags.hpp"
#include "../types.hpp"

#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{

// concurrent vector class, always safe, mutable, thread safe via internal mutex
template<is_movable_object T, class Alloc = micron::allocator_serial<>, bool Sf = true>
class convector: public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;
  mutable micron::fast_mutex __mtx;

  // all convector instantiations are mutual friends so two-object ops may lock the other object's mutex even across differing element types
  // / Sf flags
  template<is_movable_object C2, class A2, bool S2> friend class convector;

  struct __hold {
    micron::fast_mutex &m;

    [[gnu::always_inline]] explicit __hold(micron::fast_mutex &mm) noexcept : m(mm) { m.lock(); }

    [[gnu::always_inline]] ~__hold() noexcept { m.unlock(); }

    __hold(const __hold &) = delete;
    __hold &operator=(const __hold &) = delete;
  };

  struct __hold2 {
    micron::fast_mutex *a;
    micron::fast_mutex *b;      // nullptr when both args alias the same mutex

    [[gnu::always_inline]] __hold2(micron::fast_mutex &m1, micron::fast_mutex &m2) noexcept
    {
      if ( micron::addressof(m1) == micron::addressof(m2) ) {
        a = micron::addressof(m1);
        b = nullptr;
        a->lock();
      } else if ( reinterpret_cast<usize>(micron::addressof(m1)) < reinterpret_cast<usize>(micron::addressof(m2)) ) {
        a = micron::addressof(m1);
        b = micron::addressof(m2);
        a->lock();
        b->lock();
      } else {
        a = micron::addressof(m2);
        b = micron::addressof(m1);
        a->lock();
        b->lock();
      }
    }

    [[gnu::always_inline]] ~__hold2() noexcept
    {
      if ( b ) b->unlock();
      a->unlock();
    }

    __hold2(const __hold2 &) = delete;
    __hold2 &operator=(const __hold2 &) = delete;
  };

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
  __get_check(size_t n) const
  {
    return n >= __mem::length;
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

  template<auto Fn, typename E, typename... Args>
  inline __attribute__((always_inline)) void
  __safety_check(const char *msg, Args &&...args) const
  {
    if constexpr ( Sf == true ) {
      if ( (this->*Fn)(micron::forward<Args>(args)...) ) exc<E>(msg);
    }
  }

  inline void
  __unlocked_reserve(const usize n)
  {
    if ( n < __mem::capacity ) return;
    if ( __mem::is_zero() ) {
      __mem::realloc(n);
      return;
    }
    __mem::expand(n);
  }

  inline void
  __unlocked_clear(void)
  {
    if ( !__mem::length ) return;
    __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::length);
    __mem::length = 0;
  }

  inline void
  __unlocked_push_back(const T &v)
  {
    if ( __mem::length + 1 <= __mem::capacity ) {
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
        new (micron::addr(__mem::memory[__mem::length++])) T(v);
      else
        __mem::memory[__mem::length++] = v;
    } else {
      __unlocked_reserve(__impl::grow(__mem::capacity));
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
        new (micron::addr(__mem::memory[__mem::length++])) T(v);
      else
        __mem::memory[__mem::length++] = v;
    }
  }

  inline void
  __unlocked_push_back(T &&v)
  {
    if ( __mem::length + 1 <= __mem::capacity ) {
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
        new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
      else
        __mem::memory[__mem::length++] = micron::move(v);
    } else {
      __unlocked_reserve(__impl::grow(__mem::capacity));
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
        new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
      else
        __mem::memory[__mem::length++] = micron::move(v);
    }
  }

  template<typename... Args>
  inline void
  __unlocked_emplace_back(Args &&...v)
  {
    if ( __mem::length < __mem::capacity ) {
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::forward<Args>(v)...);
    } else {
      __unlocked_reserve(__impl::grow(__mem::capacity));
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::forward<Args>(v)...);
    }
  }

public:
  using category_type = vector_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
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

  ~convector(void)
  {
    if ( __mem::is_zero() ) return;
    if ( __mem::length ) __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::length);
    __mem::length = 0;
  }

  convector(void) : __mem() { }

  convector(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      size_type i = 0;
#ifndef __micron_freestanding
      try {
        for ( const T &value : lst ) {
          new (micron::addr(__mem::memory[i])) T(value);
          ++i;
        }
      } catch ( ... ) {
        for ( size_type j = 0; j < i; ++j ) __mem::memory[j].~T();
        throw;
      }
#else
      for ( const T &value : lst ) {
        new (micron::addr(__mem::memory[i])) T(value);
        ++i;
      }
#endif
      __mem::length = lst.size();
    } else {
      size_type i = 0;
      for ( auto &&value : lst ) __mem::memory[i++] = value;
      __mem::length = lst.size();
    }
  }

  convector(const size_type n) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
  }

  template<typename... Args>
    requires(sizeof...(Args) > 1 and micron::is_class_v<T>)
  convector(size_type n, Args &&...args) : __mem(n)
  {
    size_type i = 0;
#ifndef __micron_freestanding
    try {
      for ( ; i < n; i++ ) new (micron::addr(__mem::memory[i])) T(forward<Args>(args)...);
    } catch ( ... ) {
      for ( size_type j = 0; j < i; ++j ) __mem::memory[j].~T();
      throw;
    }
#else
    for ( ; i < n; i++ ) new (micron::addr(__mem::memory[i])) T(forward<Args>(args)...);
#endif
    __mem::length = n;
  }

  convector(size_type n, const T &init_value) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), init_value, n);
    __mem::length = n;
  }

  convector(size_type n, T &&init_value) : __mem(n)
  {
    T tmp = micron::move(init_value);
    __impl_container::construct(micron::addr(__mem::memory[0]), tmp, n);
    __mem::length = n;
  }

  convector(const convector &o) : __mem(o.length)
  {
    __impl_container::copy(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
  }

  convector(chunk<byte> &&m) : __mem(micron::move(m)) { }

  template<typename C = T, bool Sf2 = Sf>
    requires(micron::is_same_v<C, T>)
  convector(convector<C, Alloc, Sf2> &&o) : __mem(micron::move(o))
  {
  }

  convector(convector &&o) : __mem(micron::move(o)) { }

  convector &
  operator=(const convector &o)
  {
    if constexpr ( Sf == true ) {
      if ( this == micron::addressof(const_cast<convector &>(o)) ) return *this;
    }
    __hold __lock(__mtx);
    if ( __mem::memory ) {
      __unlocked_clear();
      __mem::free();
    }
    __mem::realloc(o.length);
    __impl_container::copy(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
    return *this;
  }

  convector &
  operator=(convector &&o)
  {
    if constexpr ( Sf == true ) {
      if ( this == micron::addressof(o) ) return *this;
    }
    __hold __lock(__mtx);
    if ( __mem::memory ) {
      __unlocked_clear();
      __mem::free();
    }
    __mem::operator=(micron::move(o));
    return *this;
  }

  template<typename... Args>
  convector &
  operator+=(Args &&...args)
  {
    __hold __lock(__mtx);
    (__unlocked_push_back(micron::forward<Args>(args)), ...);
    return *this;
  }

  const_pointer
  data(void) const
  {
    return __mem::memory;
  }

  pointer
  data(void)
  {
    return __mem::memory;
  }

  bool
  operator!(void) const
  {
    __hold __lock(__mtx);
    return __mem::length == 0;
  }

  const byte *
  operator&(void) const
  {
    return reinterpret_cast<const byte *>(__mem::memory);
  }

  byte *
  operator&(void)
  {
    return reinterpret_cast<byte *>(__mem::memory);
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

  chunk<byte>
  operator*(void) const
  {
    __hold __lock(__mtx);
    return __mem::data();
  }

  inline slice<T>
  operator[](void)
  {
    __hold __lock(__mtx);
    return slice<T>(__mem::memory, __mem::memory + __mem::length);
  }

  inline const slice<T>
  operator[](void) const
  {
    __hold __lock(__mtx);
    return slice<T>(__mem::memory, __mem::memory + __mem::length);
  }

  inline __attribute__((always_inline)) const slice<T>
  operator[](size_type from, size_type to) const
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__cap_range_check, except::library_error>("micron::convector operator[] out of allocated memory range.",
                                                                         from, to);
    return slice<T>(__mem::memory + from, __mem::memory + to);
  }

  inline __attribute__((always_inline)) slice<T>
  operator[](size_type from, size_type to)
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__cap_range_check, except::library_error>("micron::convector operator[] out of allocated memory range.",
                                                                         from, to);
    return slice<T>(__mem::memory + from, __mem::memory + to);
  }

  template<typename R>
    requires(micron::is_integral_v<R>)
  inline __attribute__((always_inline)) T
  operator[](R n) const
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__capacity_check, except::library_error>("micron::convector operator[] out of allocated memory range.",
                                                                        static_cast<size_type>(n));
    return (__mem::memory)[n];      // value snapshot — a reference would dangle once the lock releases
  }

  template<typename R>
    requires(micron::is_integral_v<R>)
  inline __attribute__((always_inline)) T &
  operator[](R n)
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__capacity_check, except::library_error>("micron::convector operator[] out of allocated memory range.",
                                                                        static_cast<size_type>(n));
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) T &
  at(size_type n)
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__index_check, except::library_error>("micron::convector at() out of bounds", n);
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) T
  at(size_type n) const
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__index_check, except::library_error>("micron::convector at() out of bounds", n);
    return (__mem::memory)[n];      // value snapshot
  }

  size_type
  at_n(iterator i) const
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__iterator_check, except::library_error>("micron::convector at_n() iterator out of range",
                                                                        static_cast<const T *>(i));
    return static_cast<size_type>(i - __mem::memory);
  }

  T *
  itr(size_type n)
  {
    __safety_check<&convector::__index_check, except::library_error>("micron::convector itr() out of bounds", n);
    return micron::addr((__mem::memory)[n]);
  }

  const T *
  itr(size_type n) const
  {
    __safety_check<&convector::__index_check, except::library_error>("micron::convector itr() out of bounds", n);
    return micron::addr((__mem::memory)[n]);
  }

  size_type
  max_size(void) const
  {
    __hold __lock(__mtx);
    return __mem::capacity;
  }

  size_type
  size(void) const
  {
    __hold __lock(__mtx);
    return __mem::length;
  }

  void
  set_size(const size_type n)
  {
    __hold __lock(__mtx);
    __mem::length = (n <= __mem::capacity) ? n : __mem::capacity;      // never let length exceed capacity (OOB)
  }

  bool
  empty(void) const
  {
    __hold __lock(__mtx);
    return __mem::length == 0;
  }

  inline void
  reserve(const size_type n)
  {
    __hold __lock(__mtx);
    __unlocked_reserve(n);
  }

  inline void
  try_reserve(const size_type n)
  {
    __hold __lock(__mtx);
    if ( n <= __mem::capacity ) return;
    __unlocked_reserve(n);
  }

  // NOTE: begin/end/cbegin/cend/data/itr/get/cget/last and the non-const element accessors are
  // NOT internally synchronized for the lifetime of the returned iterator/pointer/reference
  inline iterator
  begin(void)
  {
    return __mem::memory;
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

  inline iterator
  end(void)
  {
    return __mem::memory + __mem::length;
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

  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    __hold __lock(__mtx);
    for ( size_type i = 0; i < __mem::length; ++i ) fn(__mem::memory[i]);
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    __hold __lock(__mtx);
    for ( size_type i = 0; i < __mem::length; ++i ) fn(__mem::memory[i]);
  }

  inline iterator
  last(void)
  {
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector last() called on empty vector");
    return __mem::memory + (__mem::length - 1);
  }

  inline const_iterator
  last(void) const
  {
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector last() called on empty vector");
    return __mem::memory + (__mem::length - 1);
  }

  inline iterator
  get(const size_type n)
  {
    __safety_check<&convector::__get_check, except::library_error>("micron::convector get() out of range", n);
    return micron::addr(__mem::memory[n]);
  }

  inline const_iterator
  get(const size_type n) const
  {
    __safety_check<&convector::__get_check, except::library_error>("micron::convector get() out of range", n);
    return micron::addr(__mem::memory[n]);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    __safety_check<&convector::__get_check, except::library_error>("micron::convector cget() out of range", n);
    return micron::addr(__mem::memory[n]);
  }

  inline iterator
  find(const T &o)
  {
    __hold __lock(__mtx);
    T *f_ptr = __mem::memory;
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o ) return micron::addr(f_ptr[i]);
    return nullptr;
  }

  inline const_iterator
  find(const T &o) const
  {
    __hold __lock(__mtx);
    const T *f_ptr = __mem::memory;
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o ) return micron::addr(f_ptr[i]);
    return nullptr;
  }

  inline T
  front(void) const
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector front() called on empty vector");
    return (__mem::memory)[0];      // value snapshot
  }

  inline T
  back(void) const
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector back() called on empty vector");
    return (__mem::memory)[__mem::length - 1];      // value snapshot
  }

  inline T &
  front(void)
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector front() called on empty vector");
    return (__mem::memory)[0];
  }

  inline T &
  back(void)
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector back() called on empty vector");
    return (__mem::memory)[__mem::length - 1];
  }

  inline slice<byte>
  into_bytes(void)
  {
    __hold __lock(__mtx);
    if ( __mem::memory == nullptr || __mem::length == 0 ) return slice<byte>(nullptr, nullptr);
    return slice<byte>(reinterpret_cast<byte *>(__mem::memory), reinterpret_cast<byte *>(__mem::memory + __mem::length));
  }

  inline convector<T, Alloc, Sf>
  clone(void)
  {
    __hold __lock(__mtx);
    return convector<T, Alloc, Sf>(*this);
  }

  template<typename F, bool Sf2 = Sf>
    requires(sizeof(T) == sizeof(F))
  inline convector &
  append(const convector<F, Alloc, Sf2> &o)
  {
    __hold2 __lock(__mtx, o.__mtx);
    const size_type on = o.length;
    if ( on == 0 ) return *this;
    if ( !__mem::has_space(on) ) __unlocked_reserve(__mem::length + on);
    __impl_container::copy(micron::addr(__mem::memory[__mem::length]), micron::addr(o.memory[0]), on);
    __mem::length += on;
    return *this;
  }

  template<typename F, bool Sf2 = Sf>
    requires(sizeof(T) == sizeof(F))
  inline convector &
  weld(convector<F, Alloc, Sf2> &&o)
  {
    if ( static_cast<const void *>(this) == static_cast<const void *>(micron::addressof(o)) ) return *this;
    __hold2 __lock(__mtx, o.__mtx);
    const size_type on = o.length;
    if ( on == 0 ) return *this;
    if ( !__mem::has_space(on) ) __unlocked_reserve(__mem::length + on);
    __impl_container::move(micron::addr(__mem::memory[__mem::length]), micron::addr(o.memory[0]), on);
    __mem::length += on;
    o.length = 0;
    return *this;
  }

  template<typename C = T, bool Sf2 = Sf>
  void
  swap(convector<C, Alloc, Sf2> &o) noexcept
  {
    if ( static_cast<const void *>(this) == static_cast<const void *>(micron::addressof(o)) ) return;
    __hold2 __lock(__mtx, o.__mtx);      // lock BOTH (ordered) — a one-sided lock races the other object
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
  }

  void
  fill(const T &v)
  {
    __hold __lock(__mtx);
    __impl_container::set(micron::addr(__mem::memory[0]), v, __mem::length);
  }

  void
  resize(size_type n)
  {
    __hold __lock(__mtx);

    if ( n == __mem::length ) return;
    if ( n < __mem::length ) {
      if constexpr ( !micron::is_trivially_destructible_v<T> ) {
        for ( size_type i = n; i < __mem::length; ++i ) __mem::memory[i].~T();
      }
      __mem::length = n;
      return;
    }
    if ( n >= __mem::capacity ) __unlocked_reserve(n);
    T *f_ptr = __mem::memory;
    for ( size_type i = __mem::length; i < n; i++ ) new (micron::addr(f_ptr[i])) T{};
    __mem::length = n;
  }

  void
  resize(size_type n, const T &v)
  {
    __hold __lock(__mtx);
    if ( n == __mem::length ) return;
    if ( n < __mem::length ) {
      if constexpr ( !micron::is_trivially_destructible_v<T> ) {
        for ( size_type i = n; i < __mem::length; ++i ) __mem::memory[i].~T();
      }
      __mem::length = n;
      return;
    }
    if ( n >= __mem::capacity ) __unlocked_reserve(n);
    T *f_ptr = __mem::memory;
    for ( size_type i = __mem::length; i < n; i++ ) new (micron::addr(f_ptr[i])) T(v);
    __mem::length = n;
  }

  template<typename... Args>
  inline void
  emplace_back(Args &&...v)
  {
    __hold __lock(__mtx);
    __unlocked_emplace_back(micron::forward<Args>(v)...);
  }

  inline void
  move_back(T &&t)
  {
    __hold __lock(__mtx);
    if ( __mem::length < __mem::capacity ) {
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(t));
    } else {
      __unlocked_reserve(__impl::grow(__mem::capacity));
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(t));
    }
  }

  void
  push_back(const T &v)
  {
    __hold __lock(__mtx);
    __unlocked_push_back(v);
  }

  void
  push_back(T &&v)
  {
    __hold __lock(__mtx);
    __unlocked_push_back(micron::move(v));
  }

  inline void
  pop_back(void)
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector pop_back() called on empty vector");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      (__mem::memory)[__mem::length - 1].~T();
    else
      (__mem::memory)[__mem::length - 1] = T{};
    czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(micron::addr(__mem::memory[--__mem::length])));
  }

  inline iterator
  insert(size_type n, const T &val, size_type cnt)
  {
    __hold __lock(__mtx);
    if ( !__mem::length ) {
      for ( size_type i = 0; i < cnt; ++i ) __unlocked_push_back(val);
      return __mem::memory;
    }
    if ( __mem::length + cnt > __mem::capacity ) __unlocked_reserve(__impl::grow(__mem::capacity + cnt));
    __safety_check<&convector::__index_check, except::library_error>("micron::convector insert(): out of allocated memory range.", n);
    __impl_container::open_gap(__mem::memory, __mem::length, n, cnt);
    __impl_container::fill_gap_copy(__mem::memory, __mem::length, n, cnt, val);      // rollback-safe gap fill
    __mem::length += cnt;
    return micron::addr(__mem::memory[n]);
  }

  inline iterator
  insert(size_type n, const T &val)
  {
    __hold __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_push_back(val);
      return __mem::memory;
    }
    if ( __mem::length + 1 > __mem::capacity ) __unlocked_reserve(__impl::grow(__mem::capacity));
    __safety_check<&convector::__index_check, except::library_error>("micron::convector insert(): out of allocated memory range.", n);
    __impl_container::open_gap(__mem::memory, __mem::length, n, 1);
    __impl_container::fill_gap_copy(__mem::memory, __mem::length, n, 1, val);      // rollback-safe gap fill
    __mem::length++;
    return micron::addr(__mem::memory[n]);
  }

  inline iterator
  insert(size_type n, T &&val)
  {
    __hold __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_emplace_back(micron::move(val));
      return __mem::memory;
    }
    if ( __mem::length + 1 > __mem::capacity ) __unlocked_reserve(__impl::grow(__mem::capacity));
    __safety_check<&convector::__index_check, except::library_error>("micron::convector insert(): out of allocated memory range.", n);
    __impl_container::open_gap(__mem::memory, __mem::length, n, 1);
    __impl_container::fill_gap(__mem::memory, __mem::length, n, 1,
                              [&](size_type i) { new (micron::addr(__mem::memory[i])) T(micron::move(val)); });
    __mem::length++;
    return micron::addr(__mem::memory[n]);
  }

  inline iterator
  insert(iterator it, T &&val)
  {
    __hold __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_emplace_back(micron::move(val));
      return __mem::memory;
    }
    if ( __mem::length >= __mem::capacity ) {
      size_type dif = static_cast<size_type>(it - __mem::memory);
      __unlocked_reserve(__impl::grow(__mem::capacity));
      it = __mem::memory + dif;
    }
    if ( it > __mem::memory + __mem::length or it < __mem::memory )
      exc<except::library_error>("micron::convector insert(): iterator out of range.");
    const size_type __p = static_cast<size_type>(it - __mem::memory);
    __impl_container::open_gap(__mem::memory, __mem::length, __p, 1);
    __impl_container::fill_gap(__mem::memory, __mem::length, __p, 1,
                              [&](size_type i) { new (micron::addr(__mem::memory[i])) T(micron::move(val)); });
    __mem::length++;
    return micron::addr(__mem::memory[__p]);
  }

  inline iterator
  insert(iterator it, const T &val, const size_type cnt)
  {
    __hold __lock(__mtx);
    if ( !__mem::length ) {
      for ( size_type i = 0; i < cnt; ++i ) __unlocked_push_back(val);
      return __mem::memory;
    }
    if ( __mem::length + cnt > __mem::capacity ) {
      size_type dif = static_cast<size_type>(it - __mem::memory);
      __unlocked_reserve(__impl::grow(__mem::capacity + cnt));
      it = __mem::memory + dif;
    }
    if ( it > __mem::memory + __mem::length or it < __mem::memory )
      exc<except::library_error>("micron::convector insert(): iterator out of range.");
    const size_type __p = static_cast<size_type>(it - __mem::memory);
    __impl_container::open_gap(__mem::memory, __mem::length, __p, cnt);
    __impl_container::fill_gap_copy(__mem::memory, __mem::length, __p, cnt, val);      // rollback-safe gap fill
    __mem::length += cnt;
    return micron::addr(__mem::memory[__p]);
  }

  inline iterator
  insert(iterator it, const T &val)
  {
    __hold __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_push_back(val);
      return __mem::memory;
    }
    if ( __mem::length >= __mem::capacity ) {
      size_type dif = static_cast<size_type>(it - __mem::memory);
      __unlocked_reserve(__impl::grow(__mem::capacity));
      it = __mem::memory + dif;
    }
    if ( it > __mem::memory + __mem::length or it < __mem::memory )
      exc<except::library_error>("micron::convector insert(): iterator out of range.");
    const size_type __p = static_cast<size_type>(it - __mem::memory);
    __impl_container::open_gap(__mem::memory, __mem::length, __p, 1);
    __impl_container::fill_gap_copy(__mem::memory, __mem::length, __p, 1, val);      // rollback-safe gap fill
    __mem::length++;
    return micron::addr(__mem::memory[__p]);
  }

  inline void
  sort(void)
  {
    __hold __lock(__mtx);

    if ( __mem::length < 2 ) return;
    micron::vector<T> tmp;
    tmp.reserve(__mem::length);
    for ( size_type i = 0; i < __mem::length; ++i ) tmp.push_back(__mem::memory[i]);
    tmp.sort();
    for ( size_type i = 0; i < __mem::length; ++i ) __mem::memory[i] = tmp[i];
  }

  template<typename U = T>
  iterator
  insert_sort(U &&val)
  {
    __hold __lock(__mtx);
    if ( __mem::length == 0 ) {
      __unlocked_push_back(micron::move(val));
      return __mem::memory;
    }
    if ( __mem::length + 1 > __mem::capacity ) __unlocked_reserve(__impl::grow(__mem::capacity));

    size_type pos = 0;
    for ( ; pos < __mem::length; ++pos )
      if ( __mem::memory[pos] > val ) break;

    __impl_container::open_gap(__mem::memory, __mem::length, pos, 1);
    __impl_container::fill_gap(__mem::memory, __mem::length, pos, 1,
                              [&](size_type i) { new (micron::addr(__mem::memory[i])) T(micron::move(val)); });
    ++__mem::length;
    return micron::addr(__mem::memory[pos]);
  }

  inline convector &
  assign(const size_type cnt, const T &val)
  {
    __hold __lock(__mtx);
    if ( cnt > __mem::capacity ) __unlocked_reserve(cnt);
    __unlocked_clear();
    for ( size_type i = 0; i < cnt; i++ ) new (micron::addr(__mem::memory[i])) T(val);
    __mem::length = cnt;
    return *this;
  }

  inline void
  erase(iterator first, iterator last)
  {
    __hold __lock(__mtx);
    if ( first < __mem::memory or last > __mem::memory + __mem::length or first >= last )
      exc<except::library_error>("micron::convector erase(): invalid iterator range");

    const size_type __p = static_cast<size_type>(first - __mem::memory);
    const size_type count = static_cast<size_type>(last - first);
    __impl_container::close_gap(__mem::memory, __mem::length, __p, count);
    __mem::length -= count;
  }

  inline void
  erase(size_type from, size_type to)
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__range_check, except::library_error>("micron::convector erase(): invalid range", from, to);

    const size_type count = to - from;
    __impl_container::close_gap(__mem::memory, __mem::length, from, count);
    __mem::length -= count;
  }

  inline void
  erase(iterator it)
  {
    __hold __lock(__mtx);
    if ( it < __mem::memory or it >= __mem::memory + __mem::length )
      exc<except::library_error>("micron::convector erase(): out of allocated memory range.");

    const size_type __p = static_cast<size_type>(it - __mem::memory);
    __impl_container::close_gap(__mem::memory, __mem::length, __p, 1);
    --__mem::length;
  }

  inline void
  erase(const size_type n)
  {
    __hold __lock(__mtx);
    __safety_check<&convector::__index_check, except::library_error>("micron::convector erase(): out of allocated memory range.", n);

    __impl_container::close_gap(__mem::memory, __mem::length, n, 1);
    --__mem::length;
  }

  inline void
  __remove(const T &val)
  {
  remove_goto:
    auto itr = find(val);
    if ( itr == nullptr ) return;
    erase(itr);
    goto remove_goto;
  }

  template<typename... Args>
    requires(micron::convertible_to<T, Args> && ...)
  inline void
  remove(const Args &...val)
  {
    (__remove(static_cast<T>(val)), ...);
  }

  inline void
  clear(void)
  {
    __hold __lock(__mtx);
    __unlocked_clear();
  }

  void
  fast_clear(void)
  {
    __hold __lock(__mtx);
    if constexpr ( !micron::is_class_v<T> )
      __mem::length = 0;
    else
      __unlocked_clear();
  }

  static constexpr bool
  is_pod(void) noexcept
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

};      // namespace micron
