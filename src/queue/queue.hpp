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

template<typename T, usize N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
  requires micron::is_copy_constructible_v<T> and micron::is_move_constructible_v<T> and micron::is_copy_assignable_v<T>
           and micron::is_destructible_v<T>
class queue: public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;
  usize head;

  template<typename I>
  static constexpr usize
  __checked_elements(I count)
  {
    if ( static_cast<umax_t>(count) > static_cast<umax_t>(static_cast<usize>(-1) / sizeof(T)) ) [[unlikely]]
      exc<except::library_error>("micron::queue capacity overflow");
    return static_cast<usize>(count);
  }

  inline void
  __relocate(const usize requested)
  {
    __checked_elements(requested);
    const usize count = __mem::length;
    chunk<byte> block = __allocator_create<Alloc, alignof(T)>(allocation_multiply_or_throw(requested, sizeof(T)));
    __mem replacement(micron::move(block));
    T *fresh = replacement.memory;

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
        throw;
      }
#endif
      if ( count ) __impl_container::destroy(micron::addr(__mem::memory[head]), count);
    }

    __mem::swap(replacement);
    __mem::length = count;
    head = 0;
  }

  [[gnu::noinline, gnu::cold]] void
  __make_tail_space()
  {
    if ( head ) {
      if constexpr ( micron::is_trivially_copyable_v<T> ) {
        micron::memmove(__mem::memory, micron::addressof(__mem::memory[head]), __mem::length);
        head = 0;
      } else {
        __relocate(__mem::capacity);
      }
      if ( __mem::length < __mem::capacity ) return;
    }
    const usize max_elements = static_cast<usize>(-1) / sizeof(T);
    if ( __mem::capacity == max_elements ) [[unlikely]]
      exc<except::library_error>("micron::queue capacity overflow");
    __relocate(__mem::recommended_capacity(__mem::capacity, __mem::capacity + 1));
  }

  [[gnu::always_inline]] inline void
  __ensure_tail_space()
  {
    if ( head <= __mem::capacity && __mem::length == __mem::capacity - head ) [[unlikely]]
      __make_tail_space();
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

  queue(void) : __mem(__checked_elements(N)), head(0) { }

  queue(const std::initializer_list<T> &lst) : __mem(lst.size()), head(0)
  {
    if constexpr ( !micron::is_trivially_copyable_v<T> ) {
      usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
      try {
#endif
        for ( const T &value : lst ) {
          new (micron::addr(__mem::memory[i])) T(value);
          ++i;
        }
#if !defined(__micron_freestanding) || defined(__micron_eh)
      } catch ( ... ) {
        __impl_container::destroy(__mem::memory, i);
        throw;
      }
#endif
      __mem::length = lst.size();
    } else {
      usize i = 0;
      for ( T value : lst ) {
        __mem::memory[i++] = value;
      }
      __mem::length = lst.size();
    }
  };

  queue(const umax_t n, const T val) : __mem(__checked_elements(n)), head(0)

  {
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      for ( umax_t i = 0; i < n; i++ ) push(val);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      clear();
      throw;
    }
#endif
  }

  queue(const umax_t n) : __mem(__checked_elements(n)), head(0)

  {
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      for ( umax_t i = 0; i < n; i++ ) push();
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      clear();
      throw;
    }
#endif
  }

  queue(const queue &o) = delete;

  queue(queue &&o) : __mem(micron::move(o)), head(o.head) { o.head = 0; }

  queue &operator=(const queue &o) = delete;

  queue &
  operator=(queue &&o)
  {
    if ( this == &o ) return *this;
    // release destination first: clear() runs element dtors (as ~queue does) and
    // __mem::free() releases the old buffer; the base move-assign does not free.
    if ( __mem::memory ) {
      clear();
      __mem::free();
    }
    __mem::operator=(micron::move(o));
    head = o.head;
    o.head = 0;
    return *this;
  }

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
    __relocate(__checked_elements(n));
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
    return __mem::memory ? __mem::memory + head : nullptr;
  }

  inline const T *
  begin() const
  {
    return __mem::memory ? __mem::memory + head : nullptr;
  }

  inline const T *
  cbegin() const
  {
    return __mem::memory ? __mem::memory + head : nullptr;
  }

  // one past
  inline T *
  end()
  {
    return __mem::memory ? __mem::memory + head + __mem::length : nullptr;
  }

  inline const T *
  end() const
  {
    return __mem::memory ? __mem::memory + head + __mem::length : nullptr;
  }

  inline const T *
  cend() const
  {
    return __mem::memory ? __mem::memory + head + __mem::length : nullptr;
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
    if constexpr ( !micron::is_trivially_destructible_v<T> ) {
      (__mem::memory)[head].~T();
    }
    head++;
    __mem::length--;
    if ( __mem::length == 0 ) head = 0;
    return *this;
  }
};
};      // namespace micron
