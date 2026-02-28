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
#include "../memory/allocation/resources.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

template <is_regular_object T, usize N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
class queue : public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;
  usize needle;

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
    if ( __mem::memory == nullptr )
      return;
    clear();
  }

  queue(void) : __mem(N), needle(__mem::capacity - 1) {}

  queue(const std::initializer_list<T> &lst) : __mem(lst.size()), needle(__mem::capacity - 1)
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      usize i = __mem::capacity - 1;
      for ( T &&value : lst ) {
        new (&__mem::memory[i--]) T(micron::move(value));
      }
      __mem::length = lst.size();
    } else {
      usize i = __mem::capacity - 1;
      for ( T value : lst ) {
        __mem::memory[i--] = value;
      }
      __mem::length = lst.size();
    }
  };

  queue(const umax_t n, const T val) : __mem(n), needle(__mem::capacity - 1)

  {
    for ( umax_t i = 0; i < n; i++ )
      push(val);
  }

  queue(const umax_t n) : __mem(n), needle(__mem::capacity - 1)

  {
    for ( umax_t i = 0; i < n; i++ )
      push();
  }

  queue(const queue &o) = delete;

  queue(queue &&o) : __mem(micron::move(o)), needle(o.needle) { o.needle = 0; }

  queue &operator=(const queue &o) = delete;

  inline void
  clear()
  {
    if ( !__mem::length )
      return;
    __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::capacity);
    needle = __mem::capacity - 1;
    __mem::length = 0;
  }

  inline void
  reserve(const usize n)
  {
    const usize old_capacity = __mem::capacity;
    const usize old_needle = needle;
    const usize len = __mem::length;

    if ( n <= old_capacity )
      return;

    __mem::expand(n);

    if ( len == 0 ) {
      needle = __mem::capacity - 1;
      return;
    }

    const usize new_capacity = __mem::capacity;
    const usize new_needle = new_capacity - 1;

    T *const old_begin = &__mem::memory[old_needle - len + 1];
    T *const new_begin = &__mem::memory[new_needle - len + 1];

    micron::memmove(new_begin, old_begin, len);

    needle = new_needle;
  }

  inline void
  swap(queue &o)
  {
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
    micron::swap(needle, o.needle);
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
    return __mem::memory[needle];
  }

  inline const T &
  last() const
  {
    return __mem::memory[needle];
  }

  inline T &
  front()
  {
    return __mem::memory[needle - __mem::length + 1];
  }

  inline const T &
  front() const
  {
    return __mem::memory[needle - __mem::length + 1];
  }

  inline T *
  begin()
  {
    return &__mem::memory[needle - __mem::length + 1];
  }

  inline const T *
  begin() const
  {
    return &__mem::memory[needle - __mem::length + 1];
  }

  inline const T *
  cbegin() const
  {
    return &__mem::memory[needle - __mem::length + 1];
  }

  // one past
  inline T *
  end()
  {
    return &__mem::memory[needle + 1];
  }

  inline const T *
  end() const
  {
    return &__mem::memory[needle + 1];
  }

  inline const T *
  cend() const
  {
    return &__mem::memory[needle + 1];
  }

  inline queue &
  push(void)
  {
    if ( needle == 0 or (__mem::length + 1) >= __mem::capacity or (needle - (__mem::length + 1)) == 0 )
      reserve(__mem::capacity + 1);
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (&__mem::memory[needle - __mem::length++]) T{};
    } else {
      __mem::memory[needle - __mem::length++] = T{};
    }
    return *this;
  }

  inline queue &
  push(T &&val)
  {
    if ( needle == 0 or (__mem::length + 1) >= __mem::capacity or (needle - (__mem::length + 1)) == 0 )
      reserve(__mem::capacity + 1);
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (&__mem::memory[needle - __mem::length++]) T{ micron::move(val) };
    } else {
      __mem::memory[needle - __mem::length++] = micron::move(val);
    }
    return *this;
  }

  inline queue &
  push(const T &val)
  {
    if ( needle == 0 or (__mem::length + 1) >= __mem::capacity or (needle - (__mem::length + 1)) == 0 )
      reserve(__mem::capacity + 1);
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (&__mem::memory[needle - __mem::length++]) T{ val };
    } else {
      __mem::memory[needle - __mem::length++] = val;
    }
    return *this;
  }

  inline queue &
  pop(void)
  {
    if ( __mem::length == 0 or needle == 0 )
      return *this;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_destructible_v<T> ) {
      (__mem::memory)[needle].~T();
    } else {
      (__mem::memory)[needle] = 0x0;
    }
    needle--;
    __mem::length--;
    return *this;
  }
};
};     // namespace micron
