//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/mem.hpp"
#include "allocation/chunks.hpp"
#include "allocator.hpp"
#include "pointer.hpp"
#include "tags.hpp"
#include "types.hpp"

#include <initializer_list>

namespace micron
{
// BUG: the autoformatter set t to lowercase and i don't have the willpower to actually change this :( (sed can't help)
template <typename t, size_t N = micron::alloc_auto_sz, class alloc = micron::allocator_serial<>>
  requires std::is_copy_constructible_v<t> && std::is_move_constructible_v<t>
class stack : private alloc, public contiguous_memory_no_copy<t>
{
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
    if ( contiguous_memory_no_copy<t>::memory == nullptr )
      return;
    if constexpr ( std::is_class_v<t> )
      clear();
    this->destroy(to_chunk(contiguous_memory_no_copy<t>::memory, contiguous_memory_no_copy<t>::capacity));
  }
  stack() : contiguous_memory_no_copy<t>(this->create(N)) {}
  stack(const umax_t n) : contiguous_memory_no_copy<t>(this->create(n * sizeof(t)))
  {
    for ( umax_t i = 0; i < n; i++ )
      push();
  }
  stack(const std::initializer_list<t> &lst) : contiguous_memory_no_copy<t>(this->create(sizeof(t) * lst.size()))
  {
    if constexpr ( std::is_class_v<t> ) {
      size_t i = 0;
      for ( t &&value : lst ) {
        new (&contiguous_memory_no_copy<t>::memory[i++]) t(micron::move(value));
      }
      contiguous_memory_no_copy<t>::length = lst.size();
    } else {
      size_t i = 0;
      for ( t value : lst ) {
        contiguous_memory_no_copy<t>::memory[i++] = value;
      }
      contiguous_memory_no_copy<t>::length = lst.size();
    }
  }
  stack(const stack &) = delete;
  stack(stack &&o) : contiguous_memory_no_copy<t>(micron::move(o)) {}
  stack &operator=(const stack &) = delete;
  stack &
  operator=(stack &&o)
  {
    if ( o.length >= contiguous_memory_no_copy<t>::capacity )
      reserve(o.length);
    micron::bytecpy(contiguous_memory_no_copy<t>::memory, o.memory, o.length);
    contiguous_memory_no_copy<t>::length = o.length;
    o.length = 0;
    return *this;
  };

  inline t &
  operator[](const umax_t n)
  {
    umax_t c = contiguous_memory_no_copy<t>::length - n - 1;
    return contiguous_memory_no_copy<t>::memory[c];
  }
  inline const t &
  operator[](const umax_t n) const
  {
    umax_t c = contiguous_memory_no_copy<t>::length - n - 1;
    return contiguous_memory_no_copy<t>::memory[c];
  }
  t &
  top(void)
  {
    return contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length - 1];
  }
  const t &
  top(void) const
  {
    return immutable_memory<t>::memory[immutable_memory<t>::length - 1];
  }
  // calling stack() is equivalent to top() and pop()
  inline t
  operator()(void)
  {
    auto n = contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length - 1];
    pop();
    return n;
  }
  template <typename... Args>
  inline void
  emplace(Args...args)
  {
    if ( contiguous_memory_no_copy<t>::length < contiguous_memory_no_copy<t>::capacity ) {
      new (&contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length++])
          t(micron::move(args...));
    } else {
      reserve(contiguous_memory_no_copy<t>::capacity + 1);
      new (&contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length++])
          t(micron::move(args...));
    }
  }
  template <typename... Args>
  requires (std::is_lvalue_reference_v<Args> && ...)
  inline void
  emplace(Args &&...args)
  {
    if ( contiguous_memory_no_copy<t>::length < contiguous_memory_no_copy<t>::capacity ) {
      new (&contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length++])
          t(micron::move(micron::forward<args>(args)...));
    } else {
      reserve(contiguous_memory_no_copy<t>::capacity + 1);
      new (&contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length++])
          t(micron::move(micron::forward<args>(args)...));
    }
  }
  inline void
  push(void)
  {
    if ( contiguous_memory_no_copy<t>::length >= contiguous_memory_no_copy<t>::capacity )
      reserve(contiguous_memory_no_copy<t>::capacity + 1);
    new (&contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length++]) t();
  }
  inline void
  push(t v)
    requires(std::is_arithmetic_v<t> && std::is_same_v<t, t>)
  {
    if ( contiguous_memory_no_copy<t>::length >= contiguous_memory_no_copy<t>::capacity )
      reserve(contiguous_memory_no_copy<t>::capacity + 1);
    contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length++] = (v);
  }
  inline void
  push(const t &v)
  requires (!std::is_same_v<t, t>)
  {
    if ( contiguous_memory_no_copy<t>::length >= contiguous_memory_no_copy<t>::capacity )
      reserve(contiguous_memory_no_copy<t>::capacity + 1);
    new (&contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length++]) t(v);
  }
  inline void
  push(t &&v)
  {
    if ( contiguous_memory_no_copy<t>::length >= contiguous_memory_no_copy<t>::capacity )
      reserve(contiguous_memory_no_copy<t>::capacity + 1);
    new (&contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length++]) t(micron::move(v));
  }
  inline void
  pop()
  {
    if constexpr ( std::is_class<t>::value ) {
      contiguous_memory_no_copy<t>::memory[contiguous_memory_no_copy<t>::length - 1].~t();
    }
    czero<sizeof(t) / sizeof(byte)>(
        (byte *)micron::voidify(&(contiguous_memory_no_copy<t>::memory)[contiguous_memory_no_copy<t>::length-- - 1]));
  }
  inline void
  clear()
  {
    if ( !contiguous_memory_no_copy<t>::length )
      return;
    if constexpr ( std::is_class<t>::value ) {
      for ( size_t i = 0; i < contiguous_memory_no_copy<t>::length; i++ )
        (contiguous_memory_no_copy<t>::memory)[i].~t();
    }
    micron::zero((byte *)micron::voidify(&(contiguous_memory_no_copy<t>::memory)[0]),
                 contiguous_memory_no_copy<t>::capacity * (sizeof(t) / sizeof(byte)));
    contiguous_memory_no_copy<t>::length = 0;
  }
  // grow container
  inline void
  reserve(const size_t n)
  {
    contiguous_memory_no_copy<t>::accept_new_memory(
        this->grow(reinterpret_cast<byte *>(contiguous_memory_no_copy<t>::memory),
                   contiguous_memory_no_copy<t>::capacity * sizeof(t), sizeof(t) * n));
  }
  inline void
  swap(stack &o)
  {
    micron::swap(contiguous_memory_no_copy<t>::memory);
    micron::swap(contiguous_memory_no_copy<t>::length);
    micron::swap(contiguous_memory_no_copy<t>::capacity);
  }
  inline bool
  empty() const
  {
    return !contiguous_memory_no_copy<t>::length;
  }
  inline size_t
  size() const
  {
    return contiguous_memory_no_copy<t>::length;
  }
  inline size_t
  max_size() const
  {
    return contiguous_memory_no_copy<t>::capacity;
  }
};

template <typename t, size_t N = micron::alloc_auto_sz, class alloc = micron::allocator_serial<>>
  requires std::is_copy_constructible_v<t> && std::is_move_constructible_v<t>
class istack : private alloc, public immutable_memory<t>
{
  inline void
  _push(void)
  {
    if ( immutable_memory<t>::length >= immutable_memory<t>::capacity )
      reserve(immutable_memory<t>::capacity + 1);
    new (&immutable_memory<t>::memory[immutable_memory<t>::length++]) t();
  }
  inline void
  _push(const t &v)
  {
    if ( immutable_memory<t>::length >= immutable_memory<t>::capacity )
      reserve(immutable_memory<t>::capacity + 1);
    new (&immutable_memory<t>::memory[immutable_memory<t>::length++]) t(v);
  }
  inline void
  _push(t &&v)
  {
    if ( immutable_memory<t>::length >= immutable_memory<t>::capacity )
      reserve(immutable_memory<t>::capacity + 1);
    new (&immutable_memory<t>::memory[immutable_memory<t>::length++]) t(micron::move(v));
  }
  inline void
  _pop(void)
  {
    if constexpr ( std::is_class<t>::value ) {
      immutable_memory<t>::memory[immutable_memory<t>::length - 1].~t();
    }
    czero<sizeof(t) / sizeof(byte)>(
        (byte *)micron::voidify(&(immutable_memory<t>::memory)[immutable_memory<t>::length-- - 1]));
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
    if ( immutable_memory<t>::memory == nullptr )
      return;
    // clear(); CANT BE HERE DON'T WILL INF REC LOOP
    this->destroy(to_chunk(immutable_memory<t>::memory, immutable_memory<t>::capacity));
  }
  istack() : immutable_memory<t>(this->create(N)) {}
  istack(const umax_t n) : immutable_memory<t>(this->create(n * sizeof(t)))
  {
    for ( umax_t i = 0; i < n; i++ )
      _push();
  }
  istack(const std::initializer_list<t> &lst) : immutable_memory<t>(this->create(sizeof(t) * lst.size()))
  {
    if constexpr ( std::is_class_v<t> ) {
      size_t i = 0;
      for ( t &&value : lst ) {
        new (&immutable_memory<t>::memory[i++]) t(micron::move(value));
      }
      immutable_memory<t>::length = lst.size();
    } else {
      size_t i = 0;
      for ( t value : lst ) {
        immutable_memory<t>::memory[i++] = value;
      }
      immutable_memory<t>::length = lst.size();
    }
  }

  template <typename K = t>
    requires(std::is_convertible_v<K, t>)
  istack(const stack<K> &o) : immutable_memory<t>(this->create(o.size()))
  {
    // TODO: optimize
    for ( size_t i = o.size(); i > 0; --i )
      _push(o[i]);     // we want it to be inverse aka as it is in memory
    _push(o[0]);
  }
  istack(const istack &) = delete;
  istack(istack &&o) : immutable_memory<t>(micron::move(o)) {}
  istack(const istack *o) : immutable_memory<t>(this->create((o->capacity * sizeof(t)) + sizeof(t)))
  {
    for ( size_t i = 0; i < o->length; i++ )
      immutable_memory<t>::memory[i] = o->memory[i];
    immutable_memory<t>::length = o->length;
  }
  istack &operator=(const istack &) = delete;
  inline const t &
  operator[](const umax_t n) const
  {
    umax_t c = immutable_memory<t>::length - n - 1;
    return immutable_memory<t>::memory[c];
  }
  const t &
  top(void) const
  {
    return immutable_memory<t>::memory[immutable_memory<t>::length - 1];
  }
  inline const t &
  operator()(void)
  {
    return immutable_memory<t>::memory[immutable_memory<t>::length - 1];
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
    ret.reserve(immutable_memory<t>::capacity);
    return ret;
  }
  // grow container
  inline void
  reserve(const size_t n)
  {
    immutable_memory<t>::accept_new_memory(this->grow(reinterpret_cast<byte *>(immutable_memory<t>::memory),
                                                      immutable_memory<t>::capacity * sizeof(t), sizeof(t) * n));
  }
  inline void
  swap(istack &o)
  {
    micron::swap(immutable_memory<t>::memory);
    micron::swap(immutable_memory<t>::length);
    micron::swap(immutable_memory<t>::capacity);
  }
  inline bool
  empty() const
  {
    return !immutable_memory<t>::length;
  }
  inline size_t
  size() const
  {
    return immutable_memory<t>::length;
  }
  inline size_t
  max_size() const
  {
    return immutable_memory<t>::capacity;
  }
};

template <typename t, size_t N = micron::alloc_auto_sz>
  requires std::is_copy_constructible_v<t> && std::is_move_constructible_v<t>
class sstack
{
  t stack[N];
  size_t length;

  // shallow copy routine
  inline void
  shallow_copy(t *dest, t *src, size_t cnt)
  {
    micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src), cnt);
  };
  // deep copy routine, nec. if obj. has const/dest (can be ignored but WILL
  // cause segfaulting if underlying doesn't account for double deletes)
  inline void
  deep_copy(t *dest, t *src, size_t cnt)
  {
    for ( size_t i = 0; i < cnt; i++ )
      dest[i] = src[i];
  };
  inline void
  __impl_copy(t *dest, t *src, size_t cnt)
  {
    if constexpr ( std::is_class<t>::value ) {
      deep_copy(dest, src, cnt);
    } else {
      shallow_copy(dest, src, cnt);
    }
  }

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
  ~sstack() {}
  sstack() : stack{}, length(0) {
  }
  sstack(const umax_t n) : length(n)
  {
    for ( umax_t i = 0; i < n; i++ )
      push();
  }
  sstack(const std::initializer_list<t> &lst) : length(lst.size())
  {
    if ( lst.size() > N )
      throw except::library_error("micron::sstack() initializer_list out of bounds");
    if constexpr ( std::is_class_v<t> ) {
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
    __impl_copy(o.stack, stack, N);
    length = o.length;
  }
  template <typename C = t, size_t M> sstack(const sstack<C, M> &o)
  {
    if constexpr ( N < M ) {
      __impl_copy(o.stack, stack, M);
    } else if constexpr ( M >= N ) {
      __impl_copy(o.stack, stack, N);
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
    __impl_copy(o.stack, stack, N);
    length = o.length;
    return *this;
  }
  template <typename C = t, size_t M>
  sstack &
  operator=(const sstack<C, M> &o)
  {
    if constexpr ( N >= M ) {
      __impl_copy(o.stack, stack, N);
    } else {
      __impl_copy(o.stack, stack, M);
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
    requires(std::is_arithmetic_v<t>)
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
    if constexpr ( std::is_class<t>::value ) {
      stack[length - 1].~t();
    }
    czero<sizeof(t) / sizeof(byte)>((byte *)micron::voidify(&(stack)[length-- - 1]));
  }
  inline void
  clear()
  {
    if ( !length )
      return;
    if constexpr ( std::is_class<t>::value ) {
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
