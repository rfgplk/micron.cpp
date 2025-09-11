//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__container.hpp"

#include "algorithm/mem.hpp"
#include "allocation/resources.hpp"
#include "allocator.hpp"
#include "pointer.hpp"
#include "tags.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include <initializer_list>

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
  inline void
  emplace(Args... args)
  {
    if ( __mem::length < __mem::capacity ) {
      new (&__mem::memory[__mem::length++]) t(micron::move(args...));
    } else {
      reserve(__mem::capacity * 2);
      new (&__mem::memory[__mem::length++]) t(micron::move(args...));
    }
  }
  template <typename... Args>
    requires(micron::is_lvalue_reference_v<Args> && ...)
  inline void
  emplace(Args &&...args)
  {
    if ( __mem::length < __mem::capacity ) {
      new (&__mem::memory[__mem::length++]) t(micron::move(micron::forward<args>(args)...));
    } else {
      reserve(__mem::capacity * 2);
      new (&__mem::memory[__mem::length++]) t(micron::move(micron::forward<args>(args)...));
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
  push(t v)
    requires(micron::is_arithmetic_v<t> && micron::is_same_v<t, t>)
  {
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    __mem::memory[__mem::length++] = (v);
  }
  inline void
  push(const t &v)
  {
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    new (&__mem::memory[__mem::length++]) t(v);
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
};

template <typename t, size_t N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
  requires micron::is_copy_constructible_v<t> && micron::is_move_constructible_v<t>
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

  template <typename K = t>
    requires(micron::is_convertible_v<K, t>)
  istack(const stack<K> &o) : __mem(o.size())
  {
    // TODO: optimize
    for ( size_t i = o.size(); i > 0; --i )
      _push(o[i]);     // we want it to be inverse aka as it is in memory
    _push(o[0]);
  }
  istack(const istack &) = delete;
  istack(istack &&o) : __mem(micron::move(o)) {}
  istack(const istack *o) : __mem(o->capacity * sizeof(t))
  {
    for ( size_t i = 0; i < o->length; i++ )
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
  reserve(const size_t n)
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
};

template <typename t, size_t N = micron::alloc_auto_sz>
  requires micron::is_copy_constructible_v<t> && micron::is_move_constructible_v<t>
class sstack
{
  t stack[N];
  size_t length;

public:
  using category_type = buffer_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef t value_type;
  typedef t &reference;
  typedef t &ref;
  typedef const t &const_reference;
  typedef const t &const_ref;
  typedef t *pointer;
  typedef const t *const_pointer;
  typedef t *iterator;
  typedef const t *const_iterator;
  ~sstack() = default;
  sstack() : stack{}, length(0) {}
  sstack(const umax_t n) : length(n)
  {
    for ( umax_t i = 0; i < n; i++ )
      push();
  }
  sstack(const std::initializer_list<t> &lst) : length(lst.size())
  {
    if ( lst.size() > N )
      throw except::library_error("micron::sstack() initializer_list out of bounds");
    if constexpr ( micron::is_class_v<t> ) {
      size_t i = 0;
      for ( t &&value : lst )
        stack[i++] = value;
    } else {
      size_t i = 0;
      for ( t value : lst )
        stack[i++] = micron::move(value);
    }
  }
  sstack(const sstack &o)
  {
    __impl_container::copy(o.stack, stack, N);
    length = o.length;
  }
  template <typename C = t, size_t M> sstack(const sstack<C, M> &o)
  {
    if constexpr ( N < M ) {
      __impl_container::copy(o.stack, stack, M);
    } else if constexpr ( M >= N ) {
      __impl_container::copy(o.stack, stack, N);
    }
    length = o.length;
  }
  sstack(sstack &&o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    micron::zero<N>(&o.stack[0]);
    length = o.length;
    o.length = 0;
  }
  template <typename C = t, size_t M> sstack(sstack<C, M> &&o)
  {
    if constexpr ( N >= M ) {
      micron::copy<N>(&o.stack[0], &stack[0]);
      micron::zero<M>(&o.stack[0]);
      length = o.length;
      o.length = 0;
    } else {
      micron::copy<M>(&o.stack[0], &stack[0]);
      micron::zero<M>(&o.stack[0]);
      length = o.length;
      o.length = 0;
    }
  };
  sstack &
  operator=(const sstack &o)
  {
    __impl_container::copy(o.stack, stack, N);
    length = o.length;
    return *this;
  }
  template <typename C = t, size_t M>
  sstack &
  operator=(const sstack<C, M> &o)
  {
    if constexpr ( N >= M ) {
      __impl_container::copy(o.stack, stack, N);
    } else {
      __impl_container::copy(o.stack, stack, M);
    }
    length = o.length;
    return *this;
  }
  sstack &
  operator=(sstack &&o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    micron::zero<N>(&o.stack[0]);
    length = o.length;
    o.length = 0;
    return *this;
  };
  inline t &
  operator[](const umax_t n)
  {
    umax_t c = length - n - 1;
    return stack[c];
  }
  inline const t &
  operator[](const umax_t n) const
  {
    umax_t c = length - n - 1;
    return stack[c];
  }
  t &
  top(void)
  {
    return stack[length - 1];
  }
  const t &
  top(void) const
  {
    return stack[length - 1];
  }
  // calling stack() is equivalent to top() and pop()
  inline t
  operator()(void)
  {
    auto n = stack[length - 1];
    pop();
    return n;
  }
  template <typename... Args>
  inline void
  emplace(Args &&...args)
  {
    new (&stack[length++]) t(micron::move(micron::forward<args>(args)...));
  }
  inline void
  push(void)
  {
    new (&stack[length++]) t();
  }
  inline void
  push(t v)
    requires(micron::is_arithmetic_v<t>)
  {
    stack[length++] = (v);
  }
  inline void
  push(const t &v)
  {
    new (&stack[length++]) t(v);
  }
  inline void
  push(t &&v)
  {
    new (&stack[length++]) t(micron::move(v));
  }
  inline void
  pop()
  {
    if constexpr ( micron::is_class<t>::value ) {
      stack[length - 1].~t();
    }
    czero<sizeof(t) / sizeof(byte)>((byte *)micron::voidify(&(stack)[length-- - 1]));
  }
  inline void
  clear()
  {
    if ( !length )
      return;
    if constexpr ( micron::is_class<t>::value ) {
      for ( size_t i = 0; i < length; i++ )
        (stack)[i].~t();
    }
    micron::zero((byte *)micron::voidify(&(stack)[0]), N * (sizeof(t) / sizeof(byte)));
    length = 0;
  }
  // grow container
  inline void reserve(const size_t) = delete;
  inline void
  swap(sstack &o)
  {
  }
  inline bool
  empty() const
  {
    return !length;
  }
  inline size_t
  size() const
  {
    return length;
  }
  inline size_t
  max_size() const
  {
    return N;
  }
};

};     // namespace micron
