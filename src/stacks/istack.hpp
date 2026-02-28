//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__container.hpp"

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../concepts.hpp"
#include "../memory/allocation/resources.hpp"
#include "../pointer.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../__special/initializer_list"
#include "../concepts.hpp"

#include "stack.hpp"

namespace micron
{
template <is_movable_object t, usize N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
class istack : public __immutable_memory_resource<t, Alloc>
{
  using __mem = __immutable_memory_resource<t, Alloc>;

  inline void
  _push(void)
  {
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    new (&__mem::memory[__mem::length++]) t();
  }

  inline void
  _push(const t &v)
  {
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    new (&__mem::memory[__mem::length++]) t(v);
  }

  inline void
  _push(t &&v)
  {
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    new (&__mem::memory[__mem::length++]) t(micron::move(v));
  }

  inline void
  _pop(void)
  {
    if constexpr ( micron::is_class<t>::value ) {
      __mem::memory[__mem::length - 1].~t();
    }
    czero<sizeof(t) / sizeof(byte)>((byte *)micron::voidify(&(__mem::memory)[__mem::length-- - 1]));
  }

public:
  using category_type = buffer_tag;
  using mutability_type = immutable_tag;
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

  ~istack()
  {
    if ( __mem::is_zero() )
      return;
    // clear(); CANT BE HERE DON'T WILL INF REC LOOP
  }

  istack() : __mem(N) {}

  istack(const umax_t n) : __mem(n)
  {
    for ( umax_t i = 0; i < n; i++ )
      _push();
  }

  istack(const std::initializer_list<t> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<t> ) {
      usize i = 0;
      for ( t &&value : lst ) {
        new (&__mem::memory[i++]) t(micron::move(value));
      }
      __mem::length = lst.size();
    } else {
      usize i = 0;
      for ( t value : lst ) {
        __mem::memory[i++] = value;
      }
      __mem::length = lst.size();
    }
  }

  template <typename K = t>
    requires(micron::is_convertible_v<K, t>)
  istack(const stack<K> &o) : __mem(o.size())
  {
    // TODO: optimize
    for ( usize i = o.size(); i > 0; --i )
      _push(o[i]);     // we want it to be inverse aka as it is in memory
    _push(o[0]);
  }

  istack(const istack &) = delete;

  istack(istack &&o) : __mem(micron::move(o)) {}

  istack(const istack *o) : __mem(o->capacity * sizeof(t))
  {
    for ( usize i = 0; i < o->length; i++ )
      __mem::memory[i] = o->memory[i];
    __mem::length = o->length;
  }

  istack &operator=(const istack &) = delete;

  inline const t &
  operator[](const umax_t n) const
  {
    umax_t c = __mem::length - n - 1;
    return __mem::memory[c];
  }

  const t &
  top(void) const
  {
    return __mem::memory[__mem::length - 1];
  }

  inline const t &
  operator()(void)
  {
    return __mem::memory[__mem::length - 1];
  }

  inline auto
  push(const t &v) -> istack
  {
    istack ret(this);
    ret._push(v);
    return ret;
  }

  inline auto
  push(t &&v) -> istack
  {
    istack ret(this);
    ret._push(v);
    return ret;
  }

  inline auto
  pop() -> istack
  {
    istack ret(this);
    ret._pop();
    return ret;
  }

  inline auto
  clear() -> istack
  {
    istack ret;
    ret.reserve(__mem::capacity);
    return ret;
  }

  // grow container
  inline void
  reserve(const usize n)
  {
    if ( n < __mem::capacity )
      return;
    __mem::expand(n);
  }

  inline void
  swap(istack &o)
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

  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<t>;
  }
};
};     // namespace micron
