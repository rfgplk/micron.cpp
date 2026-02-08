//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__container.hpp"

#include "algorithm/memory.hpp"
#include "allocation/resources.hpp"
#include "allocator.hpp"
#include "pointer.hpp"
#include "tags.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include "__special/initializer_list"

namespace micron
{
// BUG: the autoformatter set t to lowercase and i don't have the willpower to actually change this :( (sed can't help)
template <typename t, size_t N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
  requires micron::is_copy_constructible_v<t> && micron::is_move_constructible_v<t>
class stack : public __mutable_memory_resource<t, Alloc>
{
  using __mem = __mutable_memory_resource<t, Alloc>;

public:
  using category_type = buffer_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef t value_type;
  typedef t &reference;
  typedef t &ref;
  typedef const t &const_reference;
  typedef const t &const_ref;
  typedef t *pointer;
  typedef const t *const_pointer;
  typedef t *iterator;
  typedef const t *const_iterator;
  ~stack()
  {
    if ( __mem::is_zero() )
      return;
    clear();
  }
  stack() : __mem(N) {}
  stack(const umax_t n) : __mem(n)
  {
    for ( umax_t i = 0; i < n; i++ )
      push();
  }
  stack(const std::initializer_list<t> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<t> ) {
      size_t i = 0;
      for ( t &&value : lst ) {
        new (&__mem::memory[i++]) t(micron::move(value));
      }
      __mem::length = lst.size();
    } else {
      size_t i = 0;
      for ( t value : lst ) {
        __mem::memory[i++] = value;
      }
      __mem::length = lst.size();
    }
  }
  stack(const stack &o) : __mem(o.length) { __impl_container::copy(__mem::memory, o.memory, o.length); };
  stack(stack &&o) : __mem(micron::move(o)) {}
  stack &
  operator=(const stack &o)
  {
    if ( o.length >= __mem::capacity )
      reserve(o.length);
    __impl_container::copy(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
    return *this;
  };
  stack &
  operator=(stack &&o)
  {
    if ( __mem::memory ) {
      __mem::free();
    }
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    return *this;
  };
  inline t &
  operator[](const umax_t n)
  {
    umax_t c = __mem::length - n - 1;
    return __mem::memory[c];
  }
  inline const t &
  operator[](const umax_t n) const
  {
    umax_t c = __mem::length - n - 1;
    return __mem::memory[c];
  }
  t &
  top(void)
  {
    return __mem::memory[__mem::length - 1];
  }
  const t &
  top(void) const
  {
    return __mem::memory[__mem::length - 1];
  }
  // calling stack() is equivalent to top() and pop()
  inline t
  operator()(void)
  {
    auto n = __mem::memory[__mem::length - 1];
    pop();
    return n;
  }
  template <typename... Args>
    requires(micron::is_lvalue_reference_v<Args> && ...)
  inline void
  emplace(Args &&...args)
  {
    if ( __mem::length < __mem::capacity ) {
      new (&__mem::memory[__mem::length++]) t(micron::forward<Args>(args)...);
    } else {
      reserve(__mem::capacity * 2);
      new (&__mem::memory[__mem::length++]) t(micron::forward<Args>(args)...);
    }
  }
  inline void
  push(void)
  {
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    new (&__mem::memory[__mem::length++]) t();
  }
  inline void
  push(const t &v)
  {
    if constexpr ( micron::is_class_v<t> or !micron::is_trivially_constructible_v<t> ) {
      if ( __mem::length >= __mem::capacity )
        reserve(__mem::capacity * 2);
      new (&__mem::memory[__mem::length++]) t(v);
    } else {
      if ( __mem::length >= __mem::capacity )
        reserve(__mem::capacity * 2);
      __mem::memory[__mem::length++] = (v);
    }
  }
  inline void
  push(t &&v)
  {
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    new (&__mem::memory[__mem::length++]) t(micron::move(v));
  }
  inline void
  pop()
  {
    if constexpr ( micron::is_class<t>::value ) {
      __mem::memory[__mem::length - 1].~t();
    }
    czero<sizeof(t) / sizeof(byte)>((byte *)micron::voidify(&(__mem::memory)[__mem::length-- - 1]));
  }
  inline void
  clear()
  {
    if ( !__mem::length )
      return;
    if constexpr ( micron::is_class<t>::value ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        (__mem::memory)[i].~t();
    }
    micron::zero((byte *)micron::voidify(&(__mem::memory)[0]), __mem::capacity * (sizeof(t) / sizeof(byte)));
    __mem::length = 0;
  }
  // grow container
  inline void
  reserve(const size_t n)
  {
    if ( n < __mem::capacity )
      return;
    __mem::expand(n);
  }
  inline void
  swap(stack &o)
  {
    micron::swap(__mem::memory);
    micron::swap(__mem::length);
    micron::swap(__mem::capacity);
  }
  inline bool
  empty() const
  {
    return !__mem::length;
  }

  inline size_t
  size() const
  {
    return __mem::length;
  }
  inline size_t
  max_size() const
  {
    return __mem::capacity;
  }
  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<t>;
  }
};
};     // namespace micron
