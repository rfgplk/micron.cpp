//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__container.hpp"
#include "../memory/new.hpp"

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../memory/allocation/resources.hpp"
#include "../pointer.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../__special/initializer_list"
#include "../concepts.hpp"

#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{

// coarse-grained, mutex-synchronized LIFO stack
template<is_regular_object T, usize N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
class constack: public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;
  mutable micron::fast_mutex __mtx;

  using __defer = micron::unique_lock<micron::lock_starts::defer, micron::fast_mutex>;

  struct __hold {
    micron::fast_mutex &m;

    [[gnu::always_inline]] explicit __hold(micron::fast_mutex &mm) noexcept : m(mm) { m.lock(); }

    [[gnu::always_inline]] ~__hold() noexcept { m.unlock(); }

    __hold(const __hold &) = delete;
    __hold &operator=(const __hold &) = delete;
  };

  // NOTE: lock two deferred guards in a deadlock-free order
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

  inline void
  __reserve_unsafe(const usize n)
  {
    if ( n < __mem::capacity ) return;
    __mem::expand(n);
  }

  inline void
  __ensure_one_unsafe()
  {
    if ( __mem::length >= __mem::capacity ) __reserve_unsafe(__mem::capacity ? __mem::capacity * 2 : (N ? N : 1));
  }

  inline void
  __clear_unsafe()
  {
    if ( !__mem::length ) return;
    __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::length);
    __mem::length = 0;
  }

  inline void
  __push_unsafe(const T &v)
  {
    __ensure_one_unsafe();
    new (micron::addr(__mem::memory[__mem::length++])) T(v);
  }

  inline void
  __push_unsafe(T &&v)
  {
    __ensure_one_unsafe();
    new (micron::addr(__mem::memory[__mem::length++])) T(micron::move(v));
  }

  inline T
  __pop_unsafe()
  {
    T val = micron::move(__mem::memory[__mem::length - 1]);
    if constexpr ( micron::is_class_v<T> ) __mem::memory[__mem::length - 1].~T();
    czero<sizeof(T) / sizeof(byte)>((byte *)micron::voidify(&__mem::memory[__mem::length-- - 1]));
    return val;
  }

  static inline umax_t
  __checked_count(const umax_t n)
  {
    if constexpr ( sizeof(T) > 1 ) {
      if ( n > (static_cast<umax_t>(-1) / sizeof(T)) ) [[unlikely]]
        exc<except::library_error>("micron::constack size overflow");
    }
    return n;
  }

public:
  using category_type = buffer_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;

  ~constack()
  {
    if ( __mem::is_zero() ) return;
    __clear_unsafe();      // no lock: an object being destroyed must not be shared
  }

  constack() : __mem(N) { }

  explicit constack(const umax_t n) : __mem(__checked_count(n))
  {
    for ( umax_t i = 0; i < n; i++ ) push();
  }

  constack(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> ) {
      usize i = 0;
      for ( const T &value : lst ) new (micron::addr(__mem::memory[i++])) T(value);
      __mem::length = lst.size();
    } else {
      usize i = 0;
      for ( T value : lst ) __mem::memory[i++] = value;
      __mem::length = lst.size();
    }
  }

  constack(const constack &o) : __mem(nullptr)
  {
    __hold lo(o.__mtx);
    if ( o.length ) {
      __reserve_unsafe(o.length);
      __impl_container::copy(__mem::memory, o.memory, o.length);
      __mem::length = o.length;
    }
  }

  constack(constack &&o) : __mem(nullptr)
  {
    __hold lo(o.__mtx);
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  }

  constack &
  operator=(const constack &o)
  {
    if ( this == micron::addr(o) ) return *this;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    __clear_unsafe();      // destroy our current elements; slots become raw storage
    if ( o.length > __mem::capacity ) __reserve_unsafe(o.length);
    __impl_container::copy(__mem::memory, o.memory, o.length);      // placement-construct
    __mem::length = o.length;
    return *this;
  }

  constack &
  operator=(constack &&o)
  {
    if ( this == micron::addr(o) ) return *this;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    __clear_unsafe();      // destroy our live elements before releasing the buffer (free() would leak them)
    if ( __mem::memory ) __mem::free();
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
    return *this;
  }

  T
  operator[](const umax_t n) const
  {
    __hold lk(__mtx);
    if ( n >= __mem::length ) [[unlikely]]
      exc<except::library_error>("micron::constack operator[] out of range");
    return __mem::memory[__mem::length - n - 1];
  }

  T
  top() const
  {
    __hold lk(__mtx);
    if ( __mem::length == 0 ) [[unlikely]]
      exc<except::library_error>("micron::constack top() called on empty stack");
    return __mem::memory[__mem::length - 1];
  }

  inline T
  operator()()
  {
    __hold lk(__mtx);
    if ( __mem::length == 0 ) [[unlikely]]
      exc<except::library_error>("micron::constack operator()() called on empty stack");
    return __pop_unsafe();
  }

  inline void
  push()
  {
    __hold lk(__mtx);
    __ensure_one_unsafe();
    new (micron::addr(__mem::memory[__mem::length++])) T{};
  }

  inline void
  push(const T &v)
  {
    __hold lk(__mtx);
    __push_unsafe(v);
  }

  inline void
  push(T &&v)
  {
    __hold lk(__mtx);
    __push_unsafe(micron::move(v));
  }

  inline void
  move(T &&v)
  {
    push(micron::move(v));
  }

  template<typename... Args>
  inline void
  push_range(Args &&...args)
  {
    __hold lk(__mtx);
    (__push_unsafe(micron::forward<Args>(args)), ...);
  }

  template<typename... Args>
  inline void
  emplace(Args &&...args)
  {
    __hold lk(__mtx);
    __ensure_one_unsafe();
    new (micron::addr(__mem::memory[__mem::length++])) T(micron::forward<Args>(args)...);
  }

  inline T
  pop()
  {
    __hold lk(__mtx);
    if ( __mem::length == 0 ) [[unlikely]]
      exc<except::library_error>("micron::constack pop() called on empty stack");
    return __pop_unsafe();
  }

  template<typename... Args>
  inline void
  pop_range(Args &...args)
  {
    static_assert((micron::is_same_v<micron::remove_cvref_t<Args>, T> && ...),
                  "micron::constack pop_range(): all output types must match value_type");
    __hold lk(__mtx);
    if ( sizeof...(Args) > __mem::length ) [[unlikely]]
      exc<except::library_error>("micron::constack pop_range(): not enough elements");
    ((args = __pop_unsafe()), ...);
  }

  inline void
  reserve(const usize n)
  {
    __hold lk(__mtx);
    __reserve_unsafe(n);
  }

  inline void
  clear()
  {
    __hold lk(__mtx);
    __clear_unsafe();
  }

  inline void
  swap(constack &o) noexcept
  {
    if ( this == micron::addr(o) ) return;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
  }

  [[nodiscard]] inline bool
  empty() const noexcept
  {
    __hold lk(__mtx);
    return __mem::length == 0;
  }

  [[nodiscard]] inline usize
  size() const noexcept
  {
    __hold lk(__mtx);
    return __mem::length;
  }

  [[nodiscard]] inline usize
  max_size() const noexcept
  {
    __hold lk(__mtx);
    return __mem::capacity;
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

  bool
  operator==(const constack &o) const noexcept
  {
    if ( this == micron::addr(o) ) return true;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    if ( __mem::length != o.length ) return false;
    for ( usize i = 0; i < __mem::length; ++i )
      if ( __mem::memory[i] != o.memory[i] ) return false;
    return true;
  }

  bool
  operator!=(const constack &o) const noexcept
  {
    return !(*this == o);
  }

  bool
  operator<(const constack &o) const noexcept
  {
    if ( this == micron::addr(o) ) return false;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    usize n = __mem::length < o.length ? __mem::length : o.length;
    for ( usize i = 0; i < n; ++i ) {
      if ( __mem::memory[i] < o.memory[i] ) return true;
      if ( o.memory[i] < __mem::memory[i] ) return false;
    }
    return __mem::length < o.length;
  }

  bool
  operator>(const constack &o) const noexcept
  {
    return o < *this;
  }

  bool
  operator<=(const constack &o) const noexcept
  {
    return !(o < *this);
  }

  bool
  operator>=(const constack &o) const noexcept
  {
    return !(*this < o);
  }
};

}      // namespace micron
