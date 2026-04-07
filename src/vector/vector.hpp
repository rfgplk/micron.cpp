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
#include "../memory/new.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"
#include "../sort/heap.hpp"
#include "../sort/quick.hpp"
#include "../tags.hpp"
#include "../types.hpp"

namespace micron
{

namespace __impl
{
inline constexpr usize
grow(usize cap)
{
  return cap == 0 ? 8 : cap * 2;
}
};     // namespace __impl

// regular vector class, always safe, mutable, not thread safe, can be copied
template <typename T, class Alloc = micron::allocator_serial<>, bool Sf = true> class vector : public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;

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
    // used by operator[](from, to) which intentionally checks capacity
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

  // NOTE: by convention destructors are first
  ~vector(void)
  {
    if ( __mem::is_zero() )
      return;
    clear();
  }

  vector(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      size_type i = 0;
      for ( T &&value : lst )
        new (micron::addr(__mem::memory[i++])) T(micron::move(value));
      __mem::length = lst.size();
    } else {
      size_type i = 0;
      for ( auto &&value : lst )
        __mem::memory[i++] = value;
      __mem::length = lst.size();
    }
  }

  vector(void) : __mem() {}

  vector(const size_type n) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
  }

  template <typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  vector(const size_type n, Fn &&fn) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
    micron::generate(begin(), end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  vector(const size_type n, Fn &&fn) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
    micron::transform(begin(), end(), fn);
  }

  template <typename... Args>
    requires(sizeof...(Args) > 1 and micron::is_class_v<T>)
  vector(size_type n, Args &&...args) : __mem(n)
  {
    for ( size_type i = 0; i < n; i++ )
      new (micron::addr(__mem::memory[i])) T(forward<Args>(args)...);
    __mem::length = n;
  }

  vector(size_type n, const T &init_value) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), init_value, n);
    __mem::length = n;
  }

  vector(const vector &o) : __mem(o.length)
  {
    __impl_container::copy(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
  }

  vector(chunk<byte> &&m) : __mem(m) { m = nullptr; }

  template <typename C = T> vector(vector<C> &&o) : __mem(micron::move(o)) {}

  vector(vector &&o) : __mem(micron::move(o)) {}

  vector &
  operator=(const vector &o)
  {
    // NOTE: not guarding against self assignment ON PURPOSE
    resize(o.length);
    __impl_container::copy_assign(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
    return *this;
  }

  vector &
  operator=(vector &&o)
  {
    // NOTE: not guarding against self assignment ON PURPOSE
    if ( __mem::memory ) {
      clear();
      __mem::free();
    }
    __mem::operator=(micron::move(o));
    return *this;
  }

  template <typename... Args>
  vector &
  operator+=(Args &&...args)
  {
    push_back(micron::move(args)...);
    return *this;
  }

  chunk<byte>
  operator*(void) const
  {
    return __mem::data();
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
    return empty();
  }

  byte *
  operator&(void)
  {
    return reinterpret_cast<byte *>(__mem::memory);
  }

  const byte *
  operator&(void) const
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
    __safety_check<&vector::__cap_range_check, except::library_error>("micron::vector operator[] out of allocated memory range.", from, to);
    return slice<T>(__mem::memory + from, __mem::memory + to);
  }

  inline __attribute__((always_inline)) slice<T>
  operator[](size_type from, size_type to)
  {
    __safety_check<&vector::__cap_range_check, except::library_error>("micron::vector operator[] out of allocated memory range.", from, to);
    return slice<T>(__mem::memory + from, __mem::memory + to);
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  inline __attribute__((always_inline)) const T &
  operator[](R n) const
  {
    __safety_check<&vector::__capacity_check, except::library_error>("micron::vector operator[] out of allocated memory range.",
                                                                     static_cast<size_type>(n));
    return (__mem::memory)[n];
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  inline __attribute__((always_inline)) T &
  operator[](R n)
  {
    __safety_check<&vector::__capacity_check, except::library_error>("micron::vector operator[] out of allocated memory range.",
                                                                     static_cast<size_type>(n));
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) T &
  at(size_type n)
  {
    __safety_check<&vector::__index_check, except::library_error>("micron::vector at() out of bounds", n);
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) const T &
  at(size_type n) const
  {
    __safety_check<&vector::__index_check, except::library_error>("micron::vector at() out of bounds", n);
    return (__mem::memory)[n];
  }

  size_type
  at_n(iterator i) const
  {
    __safety_check<&vector::__iterator_check, except::library_error>("micron::vector at_n() iterator out of range",
                                                                     static_cast<const T *>(i));
    return static_cast<size_type>(i - begin());
  }

  T *
  itr(size_type n)
  {
    __safety_check<&vector::__index_check, except::library_error>("micron::vector itr() out of bounds", n);
    return &(__mem::memory)[n];
  }

  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline vector &
  append(const vector<F> &o)
  {
    if ( o.empty() )
      return *this;
    if ( !__mem::has_space(o.length) )
      reserve(__mem::capacity + o.max_size());
    __impl_container::copy_assign(micron::addr(__mem::memory[__mem::length]), micron::addr(o.memory[0]), o.length);
    __mem::length += o.length;
    return *this;
  }

  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline vector &
  weld(vector<F> &&o)
  {
    if ( !__mem::has_space(o.length) )
      reserve(__mem::capacity + o.max_size());
    __impl_container::copy_assign(&(__mem::memory)[__mem::length], &o.memory[0], o.length);
    __mem::length += o.length;
    return *this;
  }

  template <typename C = T>
  void
  swap(vector<C> &o) noexcept
  {
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
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

  void
  set_size(const size_type n)
  {
    __mem::length = n;
  }

  bool
  empty(void) const
  {
    return __mem::length == 0;
  }

  inline void
  reserve(const size_type n)
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
  try_reserve(const size_type n)
  {
    if ( n < __mem::capacity )
      exc<except::memory_error>("micron vector failed to reserve memory");
    if ( __mem::is_zero() ) {
      __mem::realloc(n);
      return;
    }
    __mem::expand(n);
  }

  inline slice<byte>
  into_bytes(void)
  {
    if ( __mem::memory == nullptr || __mem::length == 0 )
      return slice<byte>(nullptr, nullptr);
    return slice<byte>(reinterpret_cast<byte *>(__mem::memory), reinterpret_cast<byte *>(__mem::memory + __mem::length));
  }

  inline vector<T>
  clone(void)
  {
    return vector<T>(*this);
  }

  void
  fill(const T &v)
  {
    for ( size_type i = 0; i < __mem::length; i++ )
      __mem::memory[i] = v;
  }

  void
  resize(size_type n)
  {
    if ( !(n > __mem::length) )
      return;
    if ( n >= __mem::capacity )
      reserve(n);
    T *f_ptr = __mem::memory;
    for ( size_type i = __mem::length; i < n; i++ )
      new (micron::addr(f_ptr[i])) T{};
    __mem::length = n;
  }

  void
  resize(size_type n, const T &v)
  {
    if ( !(n > __mem::length) )
      return;
    if ( n >= __mem::capacity )
      reserve(n);
    T *f_ptr = __mem::memory;
    for ( size_type i = __mem::length; i < n; i++ )
      new (micron::addr(f_ptr[i])) T(v);
    __mem::length = n;
  }

  template <typename... Args>
  void
  emplace_back(Args &&...v)
  {
    if ( __mem::length < __mem::capacity ) {
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::forward<Args>(v)...);
    } else {
      reserve(__impl::grow(__mem::capacity));
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::forward<Args>(v)...);
    }
  }

  void
  move_back(T &&t)
  {
    if ( __mem::length < __mem::capacity ) {
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(t));
    } else {
      reserve(__impl::grow(__mem::capacity));
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(t));
    }
  }

  inline iterator
  get(const size_type n)
  {
    __safety_check<&vector::__get_check, except::library_error>("micron::vector get() out of range", n);
    return &(__mem::memory[n]);
  }

  inline const_iterator
  get(const size_type n) const
  {
    __safety_check<&vector::__get_check, except::library_error>("micron::vector get() out of range", n);
    return &(__mem::memory[n]);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    __safety_check<&vector::__get_check, except::library_error>("micron::vector cget() out of range", n);
    return &(__mem::memory[n]);
  }

  inline iterator
  find(const T &o)
  {
    T *f_ptr = __mem::memory;
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }

  inline const_iterator
  find(const T &o) const
  {
    T *f_ptr = __mem::memory;
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( f_ptr[i] == o )
        return &f_ptr[i];
    return nullptr;
  }

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

  inline iterator
  last(void)
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector last() called on empty vector");
    return __mem::memory + (__mem::length - 1);
  }

  inline const_iterator
  last(void) const
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector last() called on empty vector");
    return __mem::memory + (__mem::length - 1);
  }

  inline iterator
  insert(size_type n, const T &val, size_type cnt)
  {
    if ( !__mem::length ) {
      for ( size_type i = 0; i < cnt; ++i )
        push_back(val);
      return begin();
    }
    if ( __mem::length + cnt > __mem::capacity )
      reserve(__impl::grow(__mem::capacity));
    if ( n >= __mem::length )
      exc<except::library_error>("micron::vector insert(): out of allocated memory range.");
    T *its = &(__mem::memory)[n];
    T *ite = &(__mem::memory)[__mem::length - 1];
    micron::memmove(its + cnt, its, ite - its);
    for ( size_type i = 0; i < cnt; ++i )
      new (its + i) T(val);
    __mem::length += cnt;
    return its;
  }

  inline iterator
  insert(size_type n, const T &val)
  {
    if ( !__mem::length ) {
      push_back(val);
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity )
      reserve(__impl::grow(__mem::capacity));
    if ( n >= __mem::length )
      exc<except::library_error>("micron::vector insert(): out of allocated memory range.");
    T *its = &(__mem::memory)[n];
    T *ite = &(__mem::memory)[__mem::length - 1];
    micron::memmove(its + 1, its, ite - its);
    new (its) T(val);
    __mem::length++;
    return its;
  }

  inline iterator
  insert(size_type n, T &&val)
  {
    if ( !__mem::length ) {
      emplace_back(micron::move(val));
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity )
      reserve(__impl::grow(__mem::capacity));
    if ( n >= __mem::length )
      exc<except::library_error>("micron::vector insert(): out of allocated memory range.");
    T *its = itr(n);
    T *ite = end();
    micron::memmove(its + 1, its, (ite - its));
    new (its) T(micron::move(val));
    __mem::length++;
    return its;
  }

  inline iterator
  insert(iterator it, T &&val)
  {
    if ( !__mem::length ) {
      emplace_back(micron::move(val));
      return begin();
    }
    if ( (__mem::length) >= __mem::capacity ) {
      size_type dif = static_cast<size_type>(it - __mem::memory);
      reserve(__impl::grow(__mem::capacity));
      it = __mem::memory + dif;
    }
    if ( it > end() or it < begin() )
      exc<except::library_error>("micron::vector insert(): iterator out of range.");
    T *ite = end();
    micron::memmove(it + 1, it, ite - it);
    new (it) T(micron::move(val));
    __mem::length++;
    return it;
  }

  inline iterator
  insert(iterator it, const T &val, const size_type cnt)
  {
    if ( !__mem::length ) {
      for ( size_type i = 0; i < cnt; ++i )
        push_back(val);
      return begin();
    }
    if ( __mem::length >= __mem::capacity ) {
      size_type dif = it - __mem::memory;
      reserve(__impl::grow(__mem::capacity));
      it = __mem::memory + dif;
    }
    if ( it > end() or it < begin() )
      exc<except::library_error>("micron::vector insert(): iterator out of range.");
    T *ite = end();
    micron::memmove(it + cnt, it, ite - it);
    for ( size_type i = 0; i < cnt; ++i )
      new (it + i) T(val);
    __mem::length += cnt;
    return it;
  }

  inline iterator
  insert(iterator it, const T &val)
  {
    if ( !__mem::length ) {
      push_back(val);
      return begin();
    }
    if ( __mem::length >= __mem::capacity ) {
      size_type dif = it - __mem::memory;
      reserve(__impl::grow(__mem::capacity));
      it = __mem::memory + dif;
    }
    if ( it > end() or it < begin() )
      exc<except::library_error>("micron::vector insert(): iterator out of range.");
    T *ite = end();
    micron::memmove(it + 1, it, ite - it);
    new (it) T(val);
    __mem::length++;
    return it;
  }

  inline void
  sort(void)
  {
    if ( size() < 100000 )
      micron::sort::heap(*this);
    else
      micron::sort::quick(*this);
  }

  template <typename U = T>
  iterator
  insert_sort(U &&val)
  {
    if ( __mem::length == 0 ) {
      push_back(micron::move(val));
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity )
      reserve(__impl::grow(__mem::capacity));

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

  inline vector &
  assign(const size_type cnt, const T &val)
  {
    if ( (cnt * (sizeof(T) / sizeof(byte))) >= __mem::capacity )
      reserve(__mem::capacity + cnt * sizeof(T));
    clear();
    for ( size_type i = 0; i < cnt; i++ )
      new (micron::addr(__mem::memory[i])) T(val);
    __mem::length = cnt;
    return *this;
  }

  __attribute__((always_inline)) void
  inline_push_back(const T &v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( __mem::length + 1 <= __mem::capacity ) {
        new (micron::addr(__mem::memory[__mem::length++])) T(v);
      } else {
        reserve(__impl::grow(__mem::capacity));
        new (micron::addr(__mem::memory[__mem::length++])) T(v);
      }
    } else {
      if ( __mem::length + 1 <= __mem::capacity ) {
        __mem::memory[__mem::length++] = v;
      } else {
        reserve(__impl::grow(__mem::capacity));
        __mem::memory[__mem::length++] = v;
      }
    }
  }

  __attribute__((always_inline)) void
  inline_push_back(T &&v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( __mem::length + 1 <= __mem::capacity ) {
        new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
      } else {
        reserve(__impl::grow(__mem::capacity));
        new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
      }
    } else {
      if ( __mem::length + 1 <= __mem::capacity ) {
        __mem::memory[__mem::length++] = micron::move(v);
      } else {
        reserve(__impl::grow(__mem::capacity));
        __mem::memory[__mem::length++] = micron::move(v);
      }
    }
  }

  void
  push_back(const T &v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( __mem::length + 1 <= __mem::capacity ) {
        new (micron::addr(__mem::memory[__mem::length++])) T(v);
      } else {
        reserve(__impl::grow(__mem::capacity));
        new (micron::addr(__mem::memory[__mem::length++])) T(v);
      }
    } else {
      if ( __mem::length + 1 <= __mem::capacity ) {
        __mem::memory[__mem::length++] = v;
      } else {
        reserve(__impl::grow(__mem::capacity));
        __mem::memory[__mem::length++] = v;
      }
    }
  }

  void
  push_back(T &&v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( __mem::length + 1 <= __mem::capacity ) {
        new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
      } else {
        reserve(__impl::grow(__mem::capacity));
        new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
      }
    } else {
      if ( __mem::length + 1 <= __mem::capacity ) {
        __mem::memory[__mem::length++] = micron::move(v);
      } else {
        reserve(__impl::grow(__mem::capacity));
        __mem::memory[__mem::length++] = micron::move(v);
      }
    }
  }

  inline void
  pop_back(void)
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector pop_back() called on empty vector");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      (__mem::memory)[__mem::length - 1].~T();
    else
      (__mem::memory)[__mem::length - 1] = T{};
    czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(&(__mem::memory)[--__mem::length]));
  }

  inline void
  erase(iterator first, iterator last)
  {
    if ( first < begin() or last > end() or first >= last )
      exc<except::library_error>("micron::vector erase(): invalid iterator range");

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
    __safety_check<&vector::__range_check, except::library_error>("micron::vector erase(): invalid range", from, to);

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
  erase(iterator it)
  {
    if ( it < begin() or it >= end() )
      exc<except::library_error>("micron::vector erase(): out of allocated memory range.");

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
    __safety_check<&vector::__index_check, except::library_error>("micron::vector erase(): out of allocated memory range.", n);

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      (__mem::memory)[n].~T();

    for ( size_type i = n; i < __mem::length - 1; ++i )
      (__mem::memory)[i] = micron::move((__mem::memory)[i + 1]);

    czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(&(__mem::memory)[__mem::length - 1]));
    --__mem::length;
  }

  inline void
  clear(void)
  {
    if ( !__mem::length )
      return;
    __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::length);
    __mem::length = 0;
  }

  void
  fast_clear(void)
  {
    if constexpr ( !micron::is_class_v<T> )
      __mem::length = 0;
    else
      clear();
  }

  inline const T &
  front(void) const
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector front() called on empty vector");
    return (__mem::memory)[0];
  }

  inline const T &
  back(void) const
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector back() called on empty vector");
    return (__mem::memory)[__mem::length - 1];
  }

  inline T &
  front(void)
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector front() called on empty vector");
    return (__mem::memory)[0];
  }

  inline T &
  back(void)
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector back() called on empty vector");
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
};

template <typename T, class Alloc, bool Sf>
  requires(!micron::is_regular_object<T> and micron::movable<T> and !micron::copyable<T>)
class vector<T, Alloc, Sf> : public __mutable_memory_resource_move_only<T, Alloc>
{
  using __mem = __mutable_memory_resource_move_only<T, Alloc>;

  inline bool
  __empty_check(void) const
  {
    return __mem::length == 0 || __mem::memory == nullptr;
  }

  inline bool
  __null_check(void) const
  {
    return __mem::memory == nullptr;
  }

  inline bool
  __index_check(size_t n) const
  {
    return n >= __mem::length;
  }

  inline bool
  __capacity_check(size_t n) const
  {
    return n >= __mem::capacity;
  }

  inline bool
  __get_check(size_t n) const
  {
    return n >= __mem::length;
  }

  inline bool
  __iterator_check(const T *it) const
  {
    return it < __mem::memory || it > __mem::memory + __mem::length;
  }

  inline bool
  __range_check(size_t from, size_t to) const
  {
    return from >= to || to > __mem::length;
  }

  inline bool
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

  ~vector(void)
  {
    if ( __mem::is_zero() )
      return;
    clear();
  }

  vector(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      size_type i = 0;
      for ( T &&value : lst )
        new (micron::addr(__mem::memory[i++])) T(micron::move(value));
      __mem::length = lst.size();
    } else {
      size_type i = 0;
      for ( auto &&value : lst )
        __mem::memory[i++] = value;
      __mem::length = lst.size();
    }
  }

  vector(void) : __mem() {}

  vector(const size_type n) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
  }

  template <typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  vector(const size_type n, Fn &&fn) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
    micron::generate(begin(), end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  vector(const size_type n, Fn &&fn) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), T{}, n);
    __mem::length = n;
    micron::transform(begin(), end(), fn);
  }

  template <typename... Args>
    requires(sizeof...(Args) > 1 and micron::is_class_v<T>)
  vector(size_type n, Args &&...args) : __mem(n)
  {
    for ( size_type i = 0; i < n; i++ )
      new (micron::addr(__mem::memory[i])) T(forward<Args>(args)...);
    __mem::length = n;
  }

  vector(size_type n, const T &init_value) : __mem(n)
  {
    __impl_container::construct(micron::addr(__mem::memory[0]), init_value, n);
    __mem::length = n;
  }

  vector(const vector &o) = delete;

  vector(chunk<byte> &&m) : __mem(m) { m = nullptr; }

  template <typename C = T> vector(vector<C> &&o) : __mem(micron::move(o)) {}

  vector(vector &&o) : __mem(micron::move(o)) {}

  vector &operator=(const vector &o) = delete;

  vector &
  operator=(vector &&o)
  {
    if ( this == &o )
      return *this;
    if ( __mem::memory ) {
      clear();
      __mem::free();
    }
    __mem::operator=(micron::move(o));
    return *this;
  }

  template <typename... Args>
  vector &
  operator+=(Args &&...args)
  {
    push_back(micron::move(args)...);
    return *this;
  }

  chunk<byte>
  operator*(void) const
  {
    return __mem::data();
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
    return empty();
  }

  byte *
  operator&(void)
  {
    return reinterpret_cast<byte *>(__mem::memory);
  }

  const byte *
  operator&(void) const
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

  inline __attribute__((always_inline)) const slice<T> operator[](size_type, size_type) const = delete;
  inline __attribute__((always_inline)) slice<T> operator[](size_type, size_type) = delete;

  template <typename R>
    requires(micron::is_integral_v<R>)
  inline __attribute__((always_inline)) const T &
  operator[](R n) const
  {
    __safety_check<&vector::__capacity_check, except::library_error>("micron::vector operator[] out of allocated memory range.",
                                                                     static_cast<size_type>(n));
    return (__mem::memory)[n];
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  inline __attribute__((always_inline)) T &
  operator[](R n)
  {
    __safety_check<&vector::__capacity_check, except::library_error>("micron::vector operator[] out of allocated memory range.",
                                                                     static_cast<size_type>(n));
    return (__mem::memory)[n];
  }

  inline __attribute__((always_inline)) T &
  at(size_type n)
  {
    __safety_check<&vector::__index_check, except::library_error>("micron::vector at() out of bounds", n);
    return (__mem::memory)[n];
  }

  size_type
  at_n(iterator i) const
  {
    __safety_check<&vector::__iterator_check, except::library_error>("micron::vector at_n() iterator out of range",
                                                                     static_cast<const T *>(i));
    return static_cast<size_type>(i - begin());
  }

  T *
  itr(size_type n)
  {
    __safety_check<&vector::__index_check, except::library_error>("micron::vector itr() out of bounds", n);
    return &(__mem::memory)[n];
  }

  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline vector &append(const vector<F> &o) = delete;

  template <typename F>
    requires(sizeof(T) == sizeof(F))
  inline vector &
  weld(vector<F> &&o)
  {
    if ( !__mem::has_space(o.length) )
      reserve(__mem::capacity + o.max_size());
    __impl_container::copy_assign(&(__mem::memory)[__mem::length], &o.memory[0], o.length);
    __mem::length += o.length;
    return *this;
  }

  template <typename C = T>
  void
  swap(vector<C> &o) noexcept
  {
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
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

  void
  set_size(const size_type n)
  {
    __mem::length = n;
  }

  bool
  empty(void) const
  {
    return __mem::length == 0;
  }

  inline void
  reserve(const size_type n)
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
  try_reserve(const size_type n)
  {
    if ( n < __mem::capacity )
      exc<except::memory_error>("micron vector failed to reserve memory");
    if ( __mem::is_zero() ) {
      __mem::realloc(n);
      return;
    }
    __mem::expand(n);
  }

  inline slice<byte>
  into_bytes(void)
  {
    if ( __mem::memory == nullptr || __mem::length == 0 )
      return slice<byte>(nullptr, nullptr);
    return slice<byte>(reinterpret_cast<byte *>(__mem::memory), reinterpret_cast<byte *>(__mem::memory + __mem::length));
  }

  void
  fill(const T &v)
  {
    for ( size_type i = 0; i < __mem::length; i++ )
      __mem::memory[i] = v;
  }

  void
  resize(size_type n)
  {
    if ( !(n > __mem::length) )
      return;
    if ( n >= __mem::capacity )
      reserve(n);
    for ( size_type i = __mem::length; i < n; i++ )
      new (micron::addr(__mem::memory[i])) T{};
    __mem::length = n;
  }

  void
  resize(size_type n, const T &v)
  {
    if ( !(n > __mem::length) )
      return;
    if ( n >= __mem::capacity )
      reserve(n);
    for ( size_type i = __mem::length; i < n; i++ )
      new (micron::addr(__mem::memory[i])) T(v);
    __mem::length = n;
  }

  template <typename... Args>
  void
  emplace_back(Args &&...v)
  {
    if ( __mem::length < __mem::capacity ) {
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::forward<Args>(v)...);
    } else {
      reserve(__impl::grow(__mem::capacity));
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::forward<Args>(v)...);
    }
  }

  void
  move_back(T &&t)
  {
    if ( __mem::length < __mem::capacity ) {
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(t));
    } else {
      reserve(__impl::grow(__mem::capacity));
      new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(t));
    }
  }

  inline iterator
  get(const size_type n)
  {
    __safety_check<&vector::__get_check, except::library_error>("micron::vector get() out of range", n);
    return &(__mem::memory[n]);
  }

  inline const_iterator
  get(const size_type n) const
  {
    __safety_check<&vector::__get_check, except::library_error>("micron::vector get() out of range", n);
    return &(__mem::memory[n]);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    __safety_check<&vector::__get_check, except::library_error>("micron::vector cget() out of range", n);
    return &(__mem::memory[n]);
  }

  inline iterator
  find(const T &o)
  {
    T *p = __mem::memory;
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( p[i] == o )
        return &p[i];
    return nullptr;
  }

  inline const_iterator
  find(const T &o) const
  {
    const T *p = __mem::memory;
    for ( size_type i = 0; i < __mem::length; i++ )
      if ( p[i] == o )
        return &p[i];
    return nullptr;
  }

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

  inline iterator
  last(void)
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector last() called on empty vector");
    return __mem::memory + (__mem::length - 1);
  }

  inline const_iterator
  last(void) const
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector last() called on empty vector");
    return __mem::memory + (__mem::length - 1);
  }

  inline iterator insert(size_type n, const T &val, size_type cnt) = delete;
  inline iterator insert(size_type n, const T &val) = delete;

  inline iterator
  insert(size_type n, T &&val)
  {
    if ( !__mem::length ) {
      emplace_back(micron::move(val));
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity )
      reserve(__impl::grow(__mem::capacity));
    if ( n >= __mem::length )
      exc<except::library_error>("micron::vector insert(): out of allocated memory range.");
    T *its = itr(n);
    T *ite = end();
    micron::memmove(its + 1, its, (ite - its));
    new (its) T(micron::move(val));
    __mem::length++;
    return its;
  }

  inline iterator
  insert(iterator it, T &&val)
  {
    if ( !__mem::length ) {
      emplace_back(micron::move(val));
      return begin();
    }
    if ( __mem::length >= __mem::capacity ) {
      size_type dif = static_cast<size_type>(it - __mem::memory);
      reserve(__impl::grow(__mem::capacity));
      it = __mem::memory + dif;
    }
    if ( it > end() or it < begin() )
      exc<except::library_error>("micron::vector insert(): iterator out of range.");
    T *ite = end();
    micron::memmove(it + 1, it, ite - it);
    new (it) T(micron::move(val));
    __mem::length++;
    return it;
  }

  inline iterator insert(iterator it, const T &val, const size_type cnt) = delete;
  inline iterator insert(iterator it, const T &val) = delete;

  inline void
  sort(void)
  {
    if ( size() < 100000 )
      micron::sort::heap(*this);
    else
      micron::sort::quick(*this);
  }

  template <typename U = T>
  iterator
  insert_sort(U &&val)
  {
    if ( __mem::length == 0 ) {
      push_back(micron::move(val));
      return begin();
    }
    if ( __mem::length + 1 > __mem::capacity )
      reserve(__impl::grow(__mem::capacity));
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

  inline vector &assign(const size_type cnt, const T &val) = delete;
  __attribute__((always_inline)) void inline_push_back(const T &v) = delete;

  __attribute__((always_inline)) void
  inline_push_back(T &&v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( __mem::length + 1 <= __mem::capacity ) {
        new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
      } else {
        reserve(__impl::grow(__mem::capacity));
        new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
      }
    } else {
      if ( __mem::length + 1 <= __mem::capacity ) {
        __mem::memory[__mem::length++] = micron::move(v);
      } else {
        reserve(__impl::grow(__mem::capacity));
        __mem::memory[__mem::length++] = micron::move(v);
      }
    }
  }

  void push_back(const T &v) = delete;

  void
  push_back(T &&v)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      if ( __mem::length + 1 <= __mem::capacity ) {
        new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
      } else {
        reserve(__impl::grow(__mem::capacity));
        new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
      }
    } else {
      if ( __mem::length + 1 <= __mem::capacity ) {
        __mem::memory[__mem::length++] = micron::move(v);
      } else {
        reserve(__impl::grow(__mem::capacity));
        __mem::memory[__mem::length++] = micron::move(v);
      }
    }
  }

  inline void
  pop_back(void)
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector pop_back() called on empty vector");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      (__mem::memory)[__mem::length - 1].~T();
    else
      (__mem::memory)[__mem::length - 1] = T{};
    czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(&(__mem::memory)[--__mem::length]));
  }

  inline void
  erase(iterator first, iterator last)
  {
    if ( first < begin() or last > end() or first >= last )
      exc<except::library_error>("micron::vector erase(): invalid iterator range");
    size_type count = last - first;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      for ( T *p = first; p < last; ++p )
        p->~T();
    T *it = first, *src = last, *end_ptr = __mem::memory + __mem::length;
    while ( src < end_ptr )
      *it++ = micron::move(*src++);
    for ( T *p = it; p < end_ptr; ++p )
      czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(p));
    __mem::length -= count;
  }

  inline void
  erase(size_type from, size_type to)
  {
    __safety_check<&vector::__range_check, except::library_error>("micron::vector erase(): invalid range", from, to);
    size_type count = to - from;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      for ( size_type i = from; i < to; ++i )
        (__mem::memory)[i].~T();
    for ( size_type i = to; i < __mem::length; ++i )
      (__mem::memory)[i - count] = micron::move((__mem::memory)[i]);
    for ( size_type i = __mem::length - count; i < __mem::length; ++i )
      czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(&(__mem::memory)[i]));
    __mem::length -= count;
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
  erase(iterator it)
  {
    if ( it < begin() or it >= end() )
      exc<except::library_error>("micron::vector erase(): out of allocated memory range.");
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
    __safety_check<&vector::__index_check, except::library_error>("micron::vector erase(): out of allocated memory range.", n);
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      (__mem::memory)[n].~T();
    for ( size_type i = n; i < __mem::length - 1; ++i )
      (__mem::memory)[i] = micron::move((__mem::memory)[i + 1]);
    czero<sizeof(T) / sizeof(byte)>(reinterpret_cast<byte *>(&(__mem::memory)[__mem::length - 1]));
    --__mem::length;
  }

  inline void
  clear(void)
  {
    if ( !__mem::length )
      return;
    __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::length);
    __mem::length = 0;
  }

  void
  fast_clear(void)
  {
    if constexpr ( !micron::is_class_v<T> )
      __mem::length = 0;
    else
      clear();
  }

  inline const T &
  front(void) const
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector front() called on empty vector");
    return (__mem::memory)[0];
  }

  inline const T &
  back(void) const
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector back() called on empty vector");
    return (__mem::memory)[__mem::length - 1];
  }

  inline T &
  front(void)
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector front() called on empty vector");
    return (__mem::memory)[0];
  }

  inline T &
  back(void)
  {
    __safety_check<&vector::__empty_check, except::library_error>("micron::vector back() called on empty vector");
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
};

};     // namespace micron
