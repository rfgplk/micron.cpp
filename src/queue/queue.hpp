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

namespace micron
{

template<is_regular_object T, usize N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
class queue: public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;
  // index of the oldest live element
  usize head;

  inline void
  __ensure_tail_space()
  {
    if ( head + __mem::length < __mem::capacity ) return;
    if ( head > 0 ) {
      micron::memmove(micron::addressof(__mem::memory[0]), micron::addressof(__mem::memory[head]), __mem::length);
      head = 0;
      if ( __mem::length < __mem::capacity ) return;
    }
    reserve(__mem::capacity + 1);
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

  ~queue()
  {
    if ( __mem::memory == nullptr ) return;
    clear();
  }

  queue(void) : __mem(N), head(0) { }

  queue(const std::initializer_list<T> &lst) : __mem(lst.size()), head(0)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      usize i = 0;
      for ( T &&value : lst ) {
        new (micron::addr(__mem::memory[i++])) T(micron::move(value));
      }
      __mem::length = lst.size();
    } else {
      usize i = 0;
      for ( T value : lst ) {
        __mem::memory[i++] = value;
      }
      __mem::length = lst.size();
    }
  };

  queue(const umax_t n, const T val) : __mem(n), head(0)

  {
    for ( umax_t i = 0; i < n; i++ ) push(val);
  }

  queue(const umax_t n) : __mem(n), head(0)

  {
    for ( umax_t i = 0; i < n; i++ ) push();
  }

  queue(const queue &o) = delete;

  queue(queue &&o) : __mem(micron::move(o)), head(o.head) { o.head = 0; }

  queue &operator=(const queue &o) = delete;

  inline void
  clear()
  {
    if ( !__mem::length ) return;
    __impl_container::destroy(micron::addr(__mem::memory[head]), __mem::length);
    head = 0;
    __mem::length = 0;
  }

  inline void
  reserve(const usize n)
  {
    if ( n <= __mem::capacity ) return;
    __mem::expand(n);
  }

  inline void
  swap(queue &o)
  {
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
    micron::swap(head, o.head);
  }

  inline bool
  empty() const
  {
    return __mem::length == 0;
  }

  inline usize
  size() const
  {
    return __mem::length;
  }

  inline usize
  max_size() const
  {
    return __mem::capacity;
  }

  inline T &
  last()
  {
    return __mem::memory[head];
  }

  inline const T &
  last() const
  {
    return __mem::memory[head];
  }

  inline T &
  front()
  {
    return __mem::memory[head + __mem::length - 1];
  }

  inline const T &
  front() const
  {
    return __mem::memory[head + __mem::length - 1];
  }

  inline T *
  begin()
  {
    return micron::addressof(__mem::memory[head]);
  }

  inline const T *
  begin() const
  {
    return micron::addressof(__mem::memory[head]);
  }

  inline const T *
  cbegin() const
  {
    return micron::addressof(__mem::memory[head]);
  }

  // one past
  inline T *
  end()
  {
    return micron::addressof(__mem::memory[head + __mem::length]);
  }

  inline const T *
  end() const
  {
    return micron::addressof(__mem::memory[head + __mem::length]);
  }

  inline const T *
  cend() const
  {
    return micron::addressof(__mem::memory[head + __mem::length]);
  }

  inline queue &
  push(void)
  {
    __ensure_tail_space();
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (micron::addr(__mem::memory[head + __mem::length])) T{};
    } else {
      __mem::memory[head + __mem::length] = T{};
    }
    __mem::length++;
    return *this;
  }

  inline queue &
  push(T &&val)
  {
    __ensure_tail_space();
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (micron::addr(__mem::memory[head + __mem::length])) T{ micron::move(val) };
    } else {
      __mem::memory[head + __mem::length] = micron::move(val);
    }
    __mem::length++;
    return *this;
  }

  inline queue &
  push(const T &val)
  {
    __ensure_tail_space();
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (micron::addr(__mem::memory[head + __mem::length])) T{ val };
    } else {
      __mem::memory[head + __mem::length] = val;
    }
    __mem::length++;
    return *this;
  }

  inline queue &
  pop(void)
  {
    if ( __mem::length == 0 ) return *this;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_destructible_v<T> ) {
      (__mem::memory)[head].~T();
    } else {
      (__mem::memory)[head] = 0x0;
    }
    head++;
    __mem::length--;
    if ( __mem::length == 0 ) head = 0;
    return *this;
  }
};
};      // namespace micron
