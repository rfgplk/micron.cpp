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
#include "../sort/heap.hpp"
#include "../sort/quick.hpp"
#include "../tags.hpp"
#include "../types.hpp"

#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{

// concurrent vector class, always safe, mutable, thread safe via internal mutex
template <is_movable_object T, class Alloc = micron::allocator_serial<>, bool Sf = true>
class convector : public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;
  micron::mutex __mtx;

  inline __attribute__((always_inline)) bool
  __empty_check() const
  {
    return __mem::length == 0 || __mem::memory == nullptr;
  }

  inline __attribute__((always_inline)) bool
  __null_check() const
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

  template <auto Fn, typename E, typename... Args>
  inline __attribute__((always_inline)) void
  __safety_check(const char *msg, Args &&...args) const
  {
    if constexpr ( Sf == true ) {
      if ( (this->*Fn)(micron::forward<Args>(args)...) )
        exc<E>(msg);
    }
  }

  inline void
  __unlocked_reserve(const usize n)
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
  __unlocked_clear()
  {
    if ( !__mem::length )
      return;
    __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::length);
    __mem::length = 0;
  }

  inline void
  __unlocked_push_back(const T &v)
  {
    if ( __mem::length + 1 <= __mem::capacity ) {
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
        new (&__mem::memory[__mem::length++]) T(v);
      else
        __mem::memory[__mem::length++] = v;
    } else {
      __unlocked_reserve(__impl::grow(__mem::capacity));
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
        new (&__mem::memory[__mem::length++]) T(v);
      else
        __mem::memory[__mem::length++] = v;
    }
  }

  inline void
  __unlocked_push_back(T &&v)
  {
    if ( __mem::length + 1 <= __mem::capacity ) {
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
        new (&__mem::memory[__mem::length++]) T(micron::move(v));
      else
        __mem::memory[__mem::length++] = micron::move(v);
    } else {
      __unlocked_reserve(__impl::grow(__mem::capacity));
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
        new (&__mem::memory[__mem::length++]) T(micron::move(v));
      else
        __mem::memory[__mem::length++] = micron::move(v);
    }
  }

  template <typename... Args>
  inline void
  __unlocked_emplace_back(Args &&...v)
  {
    if ( __mem::length < __mem::capacity ) {
      new (&__mem::memory[__mem::length++]) T(micron::forward<Args>(v)...);
    } else {
      __unlocked_reserve(__impl::grow(__mem::capacity));
      new (&__mem::memory[__mem::length++]) T(micron::forward<Args>(v)...);
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

  ~convector()
  {
    if ( __mem::is_zero() )
      return;
    if ( __mem::length )
      __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::length);
    __mem::length = 0;
  }

  convector(void) : __mem() {}

  convector(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      size_type i = 0;
      for ( T &&value : lst )
        new (&__mem::memory[i++]) T(micron::move(value));
      __mem::length = lst.size();
    } else {
      size_type i = 0;
      for ( auto &&value : lst )
        __mem::memory[i++] = value;
      __mem::length = lst.size();
    }
  }

  convector(const size_type n) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
  }

  template <typename... Args>
    requires(sizeof...(Args) > 1 and micron::is_class_v<T>)
  convector(size_type n, Args &&...args) : __mem(n)
  {
    for ( size_type i = 0; i < n; i++ )
      new (&__mem::memory[i]) T(forward<Args>(args)...);
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

  convector(chunk<byte> &&m) : __mem(m) { m = nullptr; }

  template <typename C = T> convector(convector<C> &&o) : __mem(micron::move(o)) {}

  convector(convector &&o) : __mem(micron::move(o)) {}

  convector &
  operator=(const convector &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
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
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::memory ) {
      __unlocked_clear();
      __mem::free();
    }
    __mem::operator=(micron::move(o));
    return *this;
  }

  template <typename... Args>
  convector &
  operator+=(Args &&...args)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    (__unlocked_push_back(micron::forward<Args>(args)), ...);
    return *this;
  }

  const_pointer
  data() const
  {
    return __mem::memory;
  }

  pointer
  data()
  {
    return __mem::memory;
  }

  bool
  operator!() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return __mem::length == 0;
  }

  const byte *
  operator&() const
  {
    return reinterpret_cast<const byte *>(__mem::memory);
  }

  byte *
  operator&()
  {
    return reinterpret_cast<byte *>(__mem::memory);
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

  chunk<byte>
  operator*() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return __mem::data();
  }

  inline slice<T>
  operator[]()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return slice<T>(__mem::memory, __mem::memory + __mem::length);
  }

  inline const slice<T>
  operator[]() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return slice<T>(__mem::memory, __mem::memory + __mem::length);
  }

  inline __attribute__((always_inline)) const slice<T>
  operator[](size_type from, size_type to) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__cap_range_check, except::library_error>("micron::convector operator[] out of allocated memory range.",
                                                                         from, to);
    return slice<T>(__mem::memory + from, __mem::memory + to);
  }

  inline __attribute__((always_inline)) slice<T>
  operator[](size_type from, size_type to)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__cap_range_check, except::library_error>("micron::convector operator[] out of allocated memory range.",
                                                                         from, to);
    return slice<T>(__mem::memory + from, __mem::memory + to);
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  inline __attribute__((always_inline)) const T &
  operator[](R n) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__capacity_check, except::library_error>("micron::convector operator[] out of allocated memory range.",
                                                                        static_cast<size_type>(n));
    return (__mem::memory)[n];
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  inline __attribute__((always_inline)) T &
  operator[](R n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__capacity_check, except::library_error>("micron::convector operator[] out of allocated memory range.",
                                                                        static_cast<size_type>(n));
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) T &
  at(size_type n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__index_check, except::library_error>("micron::convector at() out of bounds", n);
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) const T &
  at(size_type n) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__index_check, except::library_error>("micron::convector at() out of bounds", n);
    return (__mem::memory)[n];
  }

  size_type
  at_n(iterator i) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__iterator_check, except::library_error>("micron::convector at_n() iterator out of range",
                                                                        static_cast<const T *>(i));
    return static_cast<size_type>(i - __mem::memory);
  }

  T *
  itr(size_type n)
  {
    __safety_check<&convector::__index_check, except::library_error>("micron::convector itr() out of bounds", n);
    return &(__mem::memory)[n];
  }

  const T *
  itr(size_type n) const
  {
    __safety_check<&convector::__index_check, except::library_error>("micron::convector itr() out of bounds", n);
    return &(__mem::memory)[n];
  }

  size_type
  max_size() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return __mem::capacity;
  }

  size_type
  size() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return __mem::length;
  }

  void
  set_size(const size_type n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __mem::length = n;
  }

  bool
  empty() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return __mem::length == 0;
  }

  inline void
  reserve(const size_type n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __unlocked_reserve(n);
  }

  inline void
  try_reserve(const size_type n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( n < __mem::capacity )
      exc<except::memory_error>("micron convector failed to reserve memory");
    __unlocked_reserve(n);
  }

  // NOTE: no lock
  inline iterator
  begin()
  {
    return __mem::memory;
  }

  inline const_iterator
  begin() const
  {
    return __mem::memory;
  }

  inline const_iterator
  cbegin() const
  {
    return __mem::memory;
  }

  inline iterator
  end()
  {
    return __mem::memory + __mem::length;
  }

  inline const_iterator
  end() const
  {
    return __mem::memory + __mem::length;
  }

  inline const_iterator
  cend() const
  {
    return __mem::memory + __mem::length;
  }

  inline iterator
  last()
  {
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector last() called on empty vector");
    return __mem::memory + (__mem::length - 1);
  }

  inline const_iterator
  last() const
  {
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector last() called on empty vector");
    return __mem::memory + (__mem::length - 1);
  }

  inline iterator
  get(const size_type n)
  {
    __safety_check<&convector::__get_check, except::library_error>("micron::convector get() out of range", n);
    return &(__mem::memory[n]);
  }

  inline const_iterator
  get(const size_type n) const
  {
    __safety_check<&convector::__get_check, except::library_error>("micron::convector get() out of range", n);
    return &(__mem::memory[n]);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    __safety_check<&convector::__get_check, except::library_error>("micron::convector cget() out of range", n);
    return &(__mem::memory[n]);
  }

  inline iterator
  find(const T &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    T *f_ptr = __mem::memory;
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }

  inline const_iterator
  find(const T &o) const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    const T *f_ptr = __mem::memory;
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }

  inline const T &
  front() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector front() called on empty vector");
    return (__mem::memory)[0];
  }

  inline const T &
  back() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector back() called on empty vector");
    return (__mem::memory)[__mem::length - 1];
  }

  inline T &
  front()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector front() called on empty vector");
    return (__mem::memory)[0];
  }

  inline T &
  back()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector back() called on empty vector");
    return (__mem::memory)[__mem::length - 1];
  }

  inline slice<byte>
  into_bytes()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::memory == nullptr || __mem::length == 0 )
      return slice<byte>(nullptr, nullptr);
    return slice<byte>(reinterpret_cast<byte *>(__mem::memory), reinterpret_cast<byte *>(__mem::memory + __mem::length));
  }

  inline convector<T>
  clone(void)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return convector<T>(*this);
  }

  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline convector &
  append(const convector<F> &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( o.empty() )
      return *this;
    if ( !__mem::has_space(o.length) )
      __unlocked_reserve(__mem::capacity + o.max_size());
    __impl_container::copy_assign(micron::addr(__mem::memory[__mem::length]), micron::addr(o.memory[0]), o.length);
    __mem::length += o.length;
    return *this;
  }

  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline convector &
  weld(convector<F> &&o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::has_space(o.length) )
      __unlocked_reserve(__mem::capacity + o.max_size());
    __impl_container::copy_assign(&(__mem::memory)[__mem::length], &o.memory[0], o.length);
    __mem::length += o.length;
    return *this;
  }

  template <typename C = T>
  void
  swap(convector<C> &o) noexcept
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
  }

  void
  fill(const T &v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __impl_container::set(micron::addr(__mem::memory[0]), v, __mem::length);
  }

  void
  resize(size_type n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !(n > __mem::length) )
      return;
    if ( n >= __mem::capacity )
      __unlocked_reserve(n);
    T *f_ptr = __mem::memory;
    for ( size_type i = __mem::length; i < n; i++ )
      new (&f_ptr[i]) T{};
    __mem::length = n;
  }

  void
  resize(size_type n, const T &v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !(n > __mem::length) )
      return;
    if ( n >= __mem::capacity )
      __unlocked_reserve(n);
    T *f_ptr = __mem::memory;
    for ( size_type i = __mem::length; i < n; i++ )
      new (&f_ptr[i]) T(v);
    __mem::length = n;
  }

  template <typename... Args>
  inline void
  emplace_back(Args &&...v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __unlocked_emplace_back(micron::forward<Args>(v)...);
  }

  inline void
  move_back(T &&t)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length < __mem::capacity ) {
      new (&__mem::memory[__mem::length++]) T(micron::move(t));
    } else {
      __unlocked_reserve(__impl::grow(__mem::capacity));
      new (&__mem::memory[__mem::length++]) T(micron::move(t));
    }
  }

  void
  push_back(const T &v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __unlocked_push_back(v);
  }

  void
  push_back(T &&v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __unlocked_push_back(micron::move(v));
  }

  inline void
  pop_back()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__empty_check, except::library_error>("micron::convector pop_back() called on empty vector");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      (__mem::memory)[__mem::length - 1].~T();
    else
      (__mem::memory)[__mem::length - 1] = T{};
    czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(&(__mem::memory)[--__mem::length]));
  }

  inline iterator
  insert(size_type n, const T &val, size_type cnt)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      for ( size_type i = 0; i < cnt; ++i )
        __unlocked_push_back(val);
      return __mem::memory;
    }
    if ( __mem::length + cnt > __mem::capacity )
      __unlocked_reserve(__impl::grow(__mem::capacity + cnt));
    __safety_check<&convector::__index_check, except::library_error>("micron::convector insert(): out of allocated memory range.", n);
    T *its = &(__mem::memory)[n];
    T *ite = &(__mem::memory)[__mem::length];
    micron::memmove(its + cnt, its, ite - its);
    for ( size_type i = 0; i < cnt; ++i )
      new (its + i) T(val);
    __mem::length += cnt;
    return its;
  }

  inline iterator
  insert(size_type n, const T &val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_push_back(val);
      return __mem::memory;
    }
    if ( __mem::length + 1 > __mem::capacity )
      __unlocked_reserve(__impl::grow(__mem::capacity));
    __safety_check<&convector::__index_check, except::library_error>("micron::convector insert(): out of allocated memory range.", n);
    T *its = &(__mem::memory)[n];
    T *ite = &(__mem::memory)[__mem::length];
    micron::memmove(its + 1, its, ite - its);
    new (its) T(val);
    __mem::length++;
    return its;
  }

  inline iterator
  insert(size_type n, T &&val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      __unlocked_emplace_back(micron::move(val));
      return __mem::memory;
    }
    if ( __mem::length + 1 > __mem::capacity )
      __unlocked_reserve(__impl::grow(__mem::capacity));
    __safety_check<&convector::__index_check, except::library_error>("micron::convector insert(): out of allocated memory range.", n);
    T *its = &(__mem::memory)[n];
    T *ite = &(__mem::memory)[__mem::length];
    micron::memmove(its + 1, its, (ite - its));
    new (its) T(micron::move(val));
    __mem::length++;
    return its;
  }

  inline iterator
  insert(iterator it, T &&val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
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
    T *ite = __mem::memory + __mem::length;
    micron::memmove(it + 1, it, ite - it);
    new (it) T(micron::move(val));
    __mem::length++;
    return it;
  }

  inline iterator
  insert(iterator it, const T &val, const size_type cnt)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length ) {
      for ( size_type i = 0; i < cnt; ++i )
        __unlocked_push_back(val);
      return __mem::memory;
    }
    if ( __mem::length + cnt > __mem::capacity ) {
      size_type dif = static_cast<size_type>(it - __mem::memory);
      __unlocked_reserve(__impl::grow(__mem::capacity + cnt));
      it = __mem::memory + dif;
    }
    if ( it > __mem::memory + __mem::length or it < __mem::memory )
      exc<except::library_error>("micron::convector insert(): iterator out of range.");
    T *ite = __mem::memory + __mem::length;
    micron::memmove(it + cnt, it, ite - it);
    for ( size_type i = 0; i < cnt; ++i )
      new (it + i) T(val);
    __mem::length += cnt;
    return it;
  }

  inline iterator
  insert(iterator it, const T &val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
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
    T *ite = __mem::memory + __mem::length;
    micron::memmove(it + 1, it, ite - it);
    new (it) T(val);
    __mem::length++;
    return it;
  }

  inline void
  sort(void)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length < 100000 )
      micron::sort::heap(*this);
    else
      micron::sort::quick(*this);
  }

  template <typename U = T>
  iterator
  insert_sort(U &&val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length == 0 ) {
      __unlocked_push_back(micron::move(val));
      return __mem::memory;
    }
    if ( __mem::length + 1 > __mem::capacity )
      __unlocked_reserve(__impl::grow(__mem::capacity));

    size_type pos = 0;
    for ( ; pos < __mem::length; ++pos )
      if ( __mem::memory[pos] > val )
        break;

    T *it = __mem::memory + pos;
    T *end_ = __mem::memory + __mem::length;
    micron::memmove(it + 1, it, (end_ - it) * sizeof(T));
    new (it) T(micron::move(val));
    ++__mem::length;
    return it;
  }

  inline convector &
  assign(const size_type cnt, const T &val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( cnt > __mem::capacity )
      __unlocked_reserve(cnt);
    __unlocked_clear();
    for ( size_type i = 0; i < cnt; i++ )
      new (&__mem::memory[i]) T(val);
    __mem::length = cnt;
    return *this;
  }

  inline void
  erase(iterator first, iterator last)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( first < __mem::memory or last > __mem::memory + __mem::length or first >= last )
      exc<except::library_error>("micron::convector erase(): invalid iterator range");

    size_type count = last - first;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      for ( T *p = first; p < last; ++p )
        p->~T();
    }

    T *it = first;
    T *src = last;
    T *end_ptr = __mem::memory + __mem::length;

    while ( src < end_ptr )
      *it++ = micron::move(*src++);

    for ( T *p = it; p < end_ptr; ++p )
      czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(p));

    __mem::length -= count;
  }

  inline void
  erase(size_type from, size_type to)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__range_check, except::library_error>("micron::convector erase(): invalid range", from, to);

    size_type count = to - from;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      for ( size_type i = from; i < to; ++i )
        (__mem::memory)[i].~T();
    }

    for ( size_type i = to; i < __mem::length; ++i )
      (__mem::memory)[i - count] = micron::move((__mem::memory)[i]);

    for ( size_type i = __mem::length - count; i < __mem::length; ++i )
      czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(&(__mem::memory)[i]));

    __mem::length -= count;
  }

  inline void
  erase(iterator it)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( it < __mem::memory or it >= __mem::memory + __mem::length )
      exc<except::library_error>("micron::convector erase(): out of allocated memory range.");

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      it->~T();

    for ( T *p = it; p < (__mem::memory + __mem::length - 1); ++p )
      *p = micron::move(*(p + 1));

    czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(&(__mem::memory)[__mem::length - 1]));
    --__mem::length;
  }

  inline void
  erase(const size_type n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __safety_check<&convector::__index_check, except::library_error>("micron::convector erase(): out of allocated memory range.", n);

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      (__mem::memory)[n].~T();

    for ( size_type i = n; i < __mem::length - 1; ++i )
      (__mem::memory)[i] = micron::move((__mem::memory)[i + 1]);

    czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(&(__mem::memory)[__mem::length - 1]));
    --__mem::length;
  }

  inline void
  __remove(const T &val)
  {
  remove_goto:
    auto itr = find(val);
    if ( itr == nullptr )
      return;
    erase(itr);
    goto remove_goto;
  }

  template <typename... Args>
    requires(micron::convertible_to<T, Args> && ...)
  inline void
  remove(const Args &...val)
  {
    (__remove(static_cast<T>(val)), ...);
  }

  inline void
  clear()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    __unlocked_clear();
  }

  void
  fast_clear()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if constexpr ( !micron::is_class_v<T> )
      __mem::length = 0;
    else
      __unlocked_clear();
  }

  static constexpr bool
  is_pod() noexcept
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class_type() noexcept
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
