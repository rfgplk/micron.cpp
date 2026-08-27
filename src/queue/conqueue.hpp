//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../bits/__container.hpp"
#include "../memory/addr.hpp"
#include "../memory/allocation/resources.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{

template<typename T, usize N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
  requires micron::is_copy_constructible_v<T> and micron::is_move_constructible_v<T> and micron::is_copy_assignable_v<T>
           and micron::is_destructible_v<T>
class conqueue: public __mutable_memory_resource<T, Alloc>
{
  mutable micron::fast_mutex __mtx;
  using __mem = __mutable_memory_resource<T, Alloc>;
  usize head;

  using __defer = micron::unique_lock<micron::lock_starts::defer, micron::fast_mutex>;

  struct __hold {
    micron::fast_mutex &m;

    [[gnu::always_inline]] explicit __hold(micron::fast_mutex &mm) noexcept : m(mm) { m.lock(); }

    [[gnu::always_inline]] ~__hold() noexcept { m.unlock(); }

    __hold(const __hold &) = delete;
    __hold &operator=(const __hold &) = delete;
  };

  template<typename I>
  static constexpr usize
  __checked_elements(I count)
  {
    if ( static_cast<umax_t>(count) > static_cast<umax_t>(static_cast<usize>(-1) / sizeof(T)) ) [[unlikely]]
      exc<except::library_error>("micron::conqueue capacity overflow");
    return static_cast<usize>(count);
  }

  static inline void
  __lock_ordered(micron::fast_mutex &a, __defer &la, micron::fast_mutex &b, __defer &lb)
  {
    const uintptr_t aa = reinterpret_cast<uintptr_t>(micron::addr(a));
    const uintptr_t bb = reinterpret_cast<uintptr_t>(micron::addr(b));
    if ( aa == bb ) {
      la.lock();
    } else if ( aa < bb ) {
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
    if ( n <= __mem::capacity && head == 0 ) return;
    const usize count = __mem::length;
    const usize requested = n < count ? count : n;
    const usize elements = __checked_elements(requested ? requested : 1);
    chunk<byte> block = __allocator_create<Alloc, alignof(T)>(allocation_multiply_or_throw(elements, sizeof(T)));
    T *fresh = reinterpret_cast<T *>(block.ptr);
    const usize fresh_capacity = block.len / sizeof(T);

    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      if ( count ) micron::memcpy(fresh, micron::addressof(__mem::memory[head]), count);
    } else {
      usize made = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
      try {
#endif
        for ( ; made < count; ++made ) {
          if constexpr ( micron::is_nothrow_move_constructible_v<T> or !micron::is_copy_constructible_v<T> )
            new (micron::addr(fresh[made])) T(micron::move(__mem::memory[head + made]));
          else
            new (micron::addr(fresh[made])) T(__mem::memory[head + made]);
        }
#if !defined(__micron_freestanding) || defined(__micron_eh)
      } catch ( ... ) {
        __impl_container::destroy(fresh, made);
        __allocator_destroy<Alloc, alignof(T)>(block);
        throw;
      }
#endif
      if ( count ) __impl_container::destroy(micron::addr(__mem::memory[head]), count);
    }

    if ( __mem::memory ) __allocator_destroy<Alloc, alignof(T)>(__mem::data());
    __mem::memory = fresh;
    __mem::capacity = fresh_capacity;
    __mem::length = count;
    head = 0;
  }

  [[gnu::always_inline]] inline void
  __ensure_one_unsafe()
  {
    if ( head <= __mem::capacity && __mem::length < __mem::capacity - head ) return;
    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      if ( head ) {
        micron::memmove(__mem::memory, micron::addressof(__mem::memory[head]), __mem::length);
        head = 0;
        return;
      }
    }
    const usize max_elements = static_cast<usize>(-1) / sizeof(T);
    if ( __mem::length == __mem::capacity && __mem::capacity == max_elements ) [[unlikely]]
      exc<except::library_error>("micron::conqueue capacity overflow");
    const usize wanted
        = __mem::length < __mem::capacity ? __mem::capacity : __mem::recommended_capacity(__mem::capacity, __mem::capacity + 1);
    __reserve_unsafe(wanted);
  }

  inline void
  __clear_unsafe() noexcept
  {
    if ( __mem::length ) __impl_container::destroy(micron::addr(__mem::memory[head]), __mem::length);
    __mem::length = 0;
    head = 0;
  }

  template<typename U>
  inline void
  __push_unsafe(U &&val)
  {
    __ensure_one_unsafe();
    if constexpr ( micron::is_trivially_copyable_v<T> )
      __mem::memory[head + __mem::length] = micron::forward<U>(val);
    else
      new (micron::addr(__mem::memory[head + __mem::length])) T(micron::forward<U>(val));
    ++__mem::length;
  }

  inline bool
  __pop_unsafe(T *out)
  {
    if ( __mem::length == 0 ) return false;
    T &v = __mem::memory[head];
    if ( out ) {
      if constexpr ( micron::is_move_assignable_v<T> )
        *out = micron::move(v);
      else
        *out = v;
    }
    if constexpr ( !micron::is_trivially_destructible_v<T> ) v.~T();
    ++head;
    --__mem::length;
    if ( __mem::length == 0 ) head = 0;
    return true;
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
  typedef T *iterator;
  typedef const T *const_iterator;

  ~conqueue()
  {
    if ( __mem::memory == nullptr ) return;
    __clear_unsafe();
  }

  conqueue() : __mem(__checked_elements(N)), head(0) { }

  conqueue(const std::initializer_list<T> &lst) : __mem(lst.size()), head(0)
  {
    usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      for ( const T &value : lst ) {
        if constexpr ( micron::is_trivially_copyable_v<T> )
          __mem::memory[i] = value;
        else
          new (micron::addr(__mem::memory[i])) T(value);
        ++i;
      }
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      __impl_container::destroy(__mem::memory, i);
      throw;
    }
#endif
    __mem::length = i;
  };

  conqueue(const umax_t n, const T val) : __mem(__checked_elements(n)), head(0)

  {
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      for ( umax_t i = 0; i < n; i++ ) __push_unsafe(val);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      __clear_unsafe();
      throw;
    }
#endif
  }

  conqueue(const umax_t n) : __mem(__checked_elements(n)), head(0)

  {
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      for ( umax_t i = 0; i < n; i++ ) {
        if constexpr ( micron::is_trivially_copyable_v<T> )
          __mem::memory[__mem::length] = T{};
        else
          new (micron::addr(__mem::memory[__mem::length])) T{};
        ++__mem::length;
      }
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      __clear_unsafe();
      throw;
    }
#endif
  }

  conqueue(const conqueue &o) : __mem(nullptr), head(0)
  {
    __hold lock(o.__mtx);
    if ( o.length ) {
      __reserve_unsafe(o.length);
      __impl_container::copy(__mem::memory, micron::addressof(o.memory[o.head]), o.length);
      __mem::length = o.length;
    }
  }

  conqueue(conqueue &&o) : __mem(nullptr), head(0)
  {
    __hold lock(o.__mtx);
    __mem::operator=(micron::move(o));
    head = o.head;
    o.head = 0;
  }

  conqueue &
  operator=(const conqueue &o)
  {
    if ( this == micron::addr(o) ) return *this;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);

    chunk<byte> block
        = __allocator_create<Alloc, alignof(T)>(allocation_multiply_or_throw(o.length ? o.length : 1, sizeof(T)));
    T *fresh = reinterpret_cast<T *>(block.ptr);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      if ( o.length ) __impl_container::copy(fresh, micron::addressof(o.memory[o.head]), o.length);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      __allocator_destroy<Alloc, alignof(T)>(block);
      throw;
    }
#endif

    __clear_unsafe();
    if ( __mem::memory ) __allocator_destroy<Alloc, alignof(T)>(__mem::data());
    __mem::memory = fresh;
    __mem::capacity = block.len / sizeof(T);
    __mem::length = o.length;
    head = 0;
    return *this;
  }

  conqueue &
  operator=(conqueue &&o)
  {
    if ( this == micron::addr(o) ) return *this;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    __clear_unsafe();
    if ( __mem::memory ) __mem::free();
    __mem::operator=(micron::move(o));
    head = o.head;
    o.head = 0;
    return *this;
  }

  inline void
  clear()
  {
    __hold __lock(__mtx);
    __clear_unsafe();
  }

  inline void
  reserve(const usize n)
  {
    __hold __lock(__mtx);
    if ( n <= __mem::capacity ) return;
    __reserve_unsafe(n);
  }

  inline void
  swap(conqueue &o)
  {
    if ( this == micron::addr(o) ) return;
    __defer la(__mtx), lb(o.__mtx);
    __lock_ordered(__mtx, la, o.__mtx, lb);
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
    micron::swap(head, o.head);
  }

  inline bool
  empty() const
  {
    __hold __lock(__mtx);
    return __mem::length == 0;
  }

  inline usize
  size() const
  {
    __hold __lock(__mtx);
    return __mem::length;
  }

  inline usize
  max_size() const
  {
    __hold __lock(__mtx);
    return __mem::capacity;
  }

  // Raw references and iterators outlive the internal lock. They are quiescent-only.
  inline T &
  last()
  {
    __hold __lock(__mtx);
    return __mem::memory[head];
  }

  inline const T &
  last() const
  {
    __hold __lock(__mtx);
    return __mem::memory[head];
  }

  inline T &
  front()
  {
    __hold __lock(__mtx);
    return __mem::memory[head + __mem::length - 1];
  }

  inline const T &
  front() const
  {
    __hold __lock(__mtx);
    return __mem::memory[head + __mem::length - 1];
  }

  inline T *
  begin()
  {
    __hold __lock(__mtx);
    return __mem::memory ? __mem::memory + head : nullptr;
  }

  inline const T *
  begin() const
  {
    __hold __lock(__mtx);
    return __mem::memory ? __mem::memory + head : nullptr;
  }

  inline const T *
  cbegin() const
  {
    __hold __lock(__mtx);
    return __mem::memory ? __mem::memory + head : nullptr;
  }

  // one past
  inline T *
  end()
  {
    __hold __lock(__mtx);
    return __mem::memory ? __mem::memory + head + __mem::length : nullptr;
  }

  inline const T *
  end() const
  {
    __hold __lock(__mtx);
    return __mem::memory ? __mem::memory + head + __mem::length : nullptr;
  }

  inline const T *
  cend() const
  {
    __hold __lock(__mtx);
    return __mem::memory ? __mem::memory + head + __mem::length : nullptr;
  }

  inline conqueue &
  push(void)
  {
    __hold __lock(__mtx);
    __ensure_one_unsafe();
    if constexpr ( micron::is_trivially_copyable_v<T> )
      __mem::memory[head + __mem::length] = T{};
    else
      new (micron::addr(__mem::memory[head + __mem::length])) T{};
    ++__mem::length;
    return *this;
  }

  inline conqueue &
  push(T &&val)
  {
    __hold __lock(__mtx);
    __push_unsafe(micron::move(val));
    return *this;
  }

  inline conqueue &
  push(const T &val)
  {
    __hold __lock(__mtx);
    __push_unsafe(val);
    return *this;
  }

  inline usize
  push_batch(const T *items, usize count)
  {
    __hold __lock(__mtx);
    if ( count == 0 ) return 0;
    if ( count > static_cast<usize>(-1) - __mem::length ) [[unlikely]]
      exc<except::library_error>("micron::conqueue capacity overflow");
    const usize needed = __mem::length + count;
    if ( head > __mem::capacity || needed > __mem::capacity - head ) {
      if constexpr ( micron::is_trivially_copyable_v<T> ) {
        if ( needed <= __mem::capacity ) {
          micron::memmove(__mem::memory, micron::addressof(__mem::memory[head]), __mem::length);
          head = 0;
        } else {
          __reserve_unsafe(__mem::recommended_capacity(__mem::capacity, needed));
        }
      } else {
        __reserve_unsafe(needed <= __mem::capacity ? __mem::capacity : __mem::recommended_capacity(__mem::capacity, needed));
      }
    }
    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      micron::memcpy(micron::addressof(__mem::memory[head + __mem::length]), items, count);
      __mem::length += count;
      return count;
    } else {
      usize pushed = 0;
      for ( ; pushed < count; ++pushed ) {
        new (micron::addr(__mem::memory[head + __mem::length])) T(items[pushed]);
        ++__mem::length;
      }
      return pushed;
    }
  }

  inline usize
  pop_batch(T *items, usize count)
  {
    __hold __lock(__mtx);
    const usize n = count < __mem::length ? count : __mem::length;
    usize popped = 0;
    for ( ; popped < n; ++popped ) __pop_unsafe(micron::addressof(items[popped]));
    return popped;
  }

  template<typename Fn>
  inline void
  for_each_locked(Fn &&fn)
  {
    __hold __lock(__mtx);
    for ( usize i = 0; i < __mem::length; ++i ) fn(__mem::memory[head + i]);
  }

  template<typename Fn>
  inline void
  for_each_locked(Fn &&fn) const
  {
    __hold __lock(__mtx);
    for ( usize i = 0; i < __mem::length; ++i ) fn(static_cast<const T &>(__mem::memory[head + i]));
  }

  inline conqueue &
  pop(void)
  {
    __hold __lock(__mtx);
    __pop_unsafe(nullptr);
    return *this;
  }

  inline bool
  pop(T &out)
  {
    __hold __lock(__mtx);
    return __pop_unsafe(micron::addressof(out));
  }
};
};      // namespace micron
