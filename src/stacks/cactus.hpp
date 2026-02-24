//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../bits/__container.hpp"
#include "../concepts.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

template <is_regular_object T, size_t N = 64>
  requires(N > 0 and N < (1 << 16))
class cactus_stack
{

  static constexpr i32 __none = -1;

  struct node_t {
    alignas(alignof(T)) byte val_storage[sizeof(T)];
    i32 parent;
  };

  alignas(alignof(node_t)) byte _raw[sizeof(node_t) * N];

  node_t *
  __slot(i32 i) noexcept
  {
    return reinterpret_cast<node_t *>(_raw) + i;
  }

  const node_t *
  __slot(i32 i) const noexcept
  {
    return reinterpret_cast<const node_t *>(_raw) + i;
  }

  T *
  __val(i32 i) noexcept
  {
    return reinterpret_cast<T *>(__slot(i)->val_storage);
  }

  const T *
  __val(i32 i) const noexcept
  {
    return reinterpret_cast<const T *>(__slot(i)->val_storage);
  }

  void
  _construct(i32 i, const T &v, i32 par)
  {
    new (__val(i)) T(v);
    __slot(i)->parent = par;
  }

  void
  _construct(i32 i, T &&v, i32 par)
  {
    new (__val(i)) T(micron::move(v));
    __slot(i)->parent = par;
  }

  void
  __destruct(i32 i) noexcept
  {
    if constexpr ( micron::is_class_v<T> )
      __val(i)->~T();
  }

  void
  __copy_spine(const cactus_stack &src) noexcept
  {

    i32 spine[N];
    size_type cnt = 0;
    for ( i32 c = src.__head; c != __none; c = src.__slot(c)->parent )
      spine[cnt++] = c;

    for ( size_type i = 0; i < cnt; ++i )
      _construct(static_cast<i32>(i), *src.__val(spine[cnt - 1 - i]), (i == 0) ? __none : static_cast<i32>(i - 1));

    __used = static_cast<i32>(cnt);
    __head = (cnt > 0) ? static_cast<i32>(cnt - 1) : __none;
    __depth = cnt;
  }

  void
  __move_spine(cactus_stack &src) noexcept
  {
    i32 spine[N];
    size_type cnt = 0;
    for ( i32 c = src.__head; c != __none; c = src.__slot(c)->parent )
      spine[cnt++] = c;

    for ( size_type i = 0; i < cnt; ++i ) {
      i32 s = spine[cnt - 1 - i];
      _construct(static_cast<i32>(i), micron::move(*src.__val(s)), (i == 0) ? __none : static_cast<i32>(i - 1));
      src.__destruct(s);
    }

    __used = static_cast<i32>(cnt);
    __head = (cnt > 0) ? static_cast<i32>(cnt - 1) : __none;
    __depth = cnt;

    for ( i32 i = 0; i < src.__used; ++i ) {
      bool on_spine = false;
      for ( size_type k = 0; k < cnt; ++k )
        if ( spine[k] == i ) {
          on_spine = true;
          break;
        }
      if ( !on_spine )
        src.__destruct(i);
    }

    src.__used = 0;
    src.__head = __none;
    src.__depth = 0;
  }

  i32
  _alloc() noexcept
  {
    if ( __used == static_cast<i32>(N) ) [[unlikely]]
      __builtin_trap();
    return __used++;
  }

  template <typename U>
  cactus_stack
  __push_impl(U &&v) const
  {
    cactus_stack next;
    next.__copy_spine(*this);
    i32 idx = next._alloc();
    next._construct(idx, micron::forward<U>(v), next.__head);
    next.__head = idx;
    next.__depth = __depth + 1;
    return next;
  }

  template <typename U, typename... Rest>
  static cactus_stack
  __push_all(cactus_stack s, U &&first, Rest &&...rest)
  {
    auto ns = s.push(micron::forward<U>(first));
    if constexpr ( sizeof...(Rest) > 0 )
      return __push_all(micron::move(ns), micron::forward<Rest>(rest)...);
    else
      return ns;
  }

  static cactus_stack
  __push_all(cactus_stack s)
  {
    return s;
  }

  i32 __used;
  i32 __head;
  size_t __depth;

public:
  using category_type = buffer_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef size_t size_type;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  ~cactus_stack()
  {
    for ( i32 i = 0; i < __used; ++i )
      __destruct(i);
  }

  cactus_stack() noexcept : __used(0), __head(__none), __depth(0) {}

  cactus_stack(const cactus_stack &o) : __used(0), __head(__none), __depth(0) { __copy_spine(o); }

  cactus_stack(cactus_stack &&o) noexcept : __used(0), __head(__none), __depth(0) { __move_spine(o); }

  cactus_stack &
  operator=(const cactus_stack &o)
  {
    if ( this == &o )
      return *this;
    for ( i32 i = 0; i < __used; ++i )
      __destruct(i);
    __used = 0;
    __head = __none;
    __depth = 0;
    __copy_spine(o);
    return *this;
  }

  cactus_stack &
  operator=(cactus_stack &&o) noexcept
  {
    if ( this == &o )
      return *this;
    for ( i32 i = 0; i < __used; ++i )
      __destruct(i);
    __used = 0;
    __head = __none;
    __depth = 0;
    __move_spine(o);
    return *this;
  }

  cactus_stack
  push(const T &v) const
  {
    return __push_impl(v);
  }

  cactus_stack
  push(T &&v) const
  {
    return __push_impl(micron::move(v));
  }

  cactus_stack
  move(T &&v) const
  {
    return push(micron::move(v));
  }

  cactus_stack
  child(const T &v) const
  {
    return push(v);
  }

  cactus_stack
  child(T &&v) const
  {
    return push(micron::move(v));
  }

  template <typename... Args>
  cactus_stack
  push_range(Args &&...args) const
  {
    return __push_all(*this, micron::forward<Args>(args)...);
  }

  cactus_stack
  pop() const noexcept
  {
    cactus_stack next;
    next.__copy_spine(*this);
    next.__head = next.__slot(next.__head)->parent;
    next.__depth = (__depth > 0) ? __depth - 1 : 0;

    return next;
  }

  cactus_stack
  parent() const noexcept
  {
    return pop();
  }

  struct PopResult {
    T value;
    cactus_stack stack;
  };

  PopResult
  try_pop() const
  {
    return { *__val(__head), pop() };
  }

  template <typename... Args>
  cactus_stack
  pop_range(Args &...args) const
  {
    cactus_stack cur;
    cur.__copy_spine(*this);
    cur.__depth = __depth;
    ((args = *cur.__val(cur.__head), cur = cur.pop()), ...);
    return cur;
  }

  const T &
  val() const noexcept
  {
    return *__val(__head);
  }

  const T &
  top() const noexcept
  {
    return val();
  }

  bool
  is_empty() const noexcept
  {
    return __head == __none;
  }

  bool
  empty() const noexcept
  {
    return is_empty();
  }

  size_type
  len() const noexcept
  {
    return __depth;
  }

  size_type
  size() const noexcept
  {
    return __depth;
  }

  static constexpr size_type
  max_size() noexcept
  {
    return N;
  }

  static constexpr size_type
  capacity() noexcept
  {
    return N;
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

  const cactus_stack *
  addr() const noexcept
  {
    return this;
  }

  cactus_stack *
  addr() noexcept
  {
    return this;
  }

  struct __val_range_itr {
    const cactus_stack *owner;
    i32 cur;

    __val_range_itr(const cactus_stack *o, i32 c) noexcept : owner(o), cur(c) {}

    bool
    operator==(const __val_range_itr &o) const noexcept
    {
      return cur == o.cur;
    }

    bool
    operator!=(const __val_range_itr &o) const noexcept
    {
      return cur != o.cur;
    }

    const T &
    operator*() const noexcept
    {
      return *owner->__val(cur);
    }

    __val_range_itr &
    operator++() noexcept
    {
      cur = owner->__slot(cur)->parent;
      return *this;
    }

    __val_range_itr
    operator++(int) noexcept
    {
      __val_range_itr t(*this);
      ++(*this);
      return t;
    }
  };

  struct __val_range {
    __val_range_itr _b, _e;

    __val_range_itr
    begin() const noexcept
    {
      return _b;
    }

    __val_range_itr
    end() const noexcept
    {
      return _e;
    }
  };

  __val_range
  vals() const noexcept
  {
    return { __val_range_itr(this, __head), __val_range_itr(this, __none) };
  }

  struct __range_itr {
    const cactus_stack *owner;
    i32 cur;

    __range_itr(const cactus_stack *o, i32 c) noexcept : owner(o), cur(c) {}

    bool
    operator==(const __range_itr &o) const noexcept
    {
      return cur == o.cur;
    }

    bool
    operator!=(const __range_itr &o) const noexcept
    {
      return cur != o.cur;
    }

    __range_itr &
    operator++() noexcept
    {
      cur = owner->__slot(cur)->parent;
      return *this;
    }

    __range_itr
    operator++(int) noexcept
    {
      __range_itr t(*this);
      ++(*this);
      return t;
    }

    cactus_stack
    operator*() const noexcept
    {

      i32 spine[N];
      size_type cnt = 0;
      for ( i32 c = cur; c != __none; c = owner->__slot(c)->parent )
        spine[cnt++] = c;

      cactus_stack snap;
      for ( size_type i = 0; i < cnt; ++i )
        snap._construct(static_cast<i32>(i), *owner->__val(spine[cnt - 1 - i]), (i == 0) ? __none : static_cast<i32>(i - 1));
      snap.__used = static_cast<i32>(cnt);
      snap.__head = (cnt > 0) ? static_cast<i32>(cnt - 1) : __none;
      snap.__depth = cnt;
      return snap;
    }
  };

  struct __range {
    __range_itr _b, _e;

    __range_itr
    begin() const noexcept
    {
      return _b;
    }

    __range_itr
    end() const noexcept
    {
      return _e;
    }
  };

  __range
  nodes() const noexcept
  {
    return { __range_itr(this, __head), __range_itr(this, __none) };
  }

  bool
  operator==(const cactus_stack &o) const noexcept
  {
    if ( __depth != o.__depth )
      return false;
    i32 a = __head, b = o.__head;
    while ( a != __none && b != __none ) {
      if ( !(*__val(a) == *o.__val(b)) )
        return false;
      a = __slot(a)->parent;
      b = o.__slot(b)->parent;
    }
    return (a == __none) && (b == __none);
  }

  bool
  operator!=(const cactus_stack &o) const noexcept
  {
    return !(*this == o);
  }

  bool
  operator<(const cactus_stack &o) const noexcept
  {
    i32 as_[N], bs_[N];
    size_type ac = 0, bc = 0;
    for ( i32 c = __head; c != __none; c = __slot(c)->parent )
      as_[ac++] = c;
    for ( i32 c = o.__head; c != __none; c = o.__slot(c)->parent )
      bs_[bc++] = c;
    size_type n = (ac < bc) ? ac : bc;
    for ( size_type i = 0; i < n; ++i ) {
      const T &av = *__val(as_[ac - 1 - i]);
      const T &bv = *o.__val(bs_[bc - 1 - i]);
      if ( av < bv )
        return true;
      if ( bv < av )
        return false;
    }
    return ac < bc;
  }

  bool
  operator>(const cactus_stack &o) const noexcept
  {
    return o < *this;
  }

  bool
  operator<=(const cactus_stack &o) const noexcept
  {
    return !(o < *this);
  }

  bool
  operator>=(const cactus_stack &o) const noexcept
  {
    return !(*this < o);
  }
};

template <is_regular_object T, size_t N = 64>
  requires(N > 0 and N < (1 << 16))
class fixed_stack
{

  alignas(64) byte stack[sizeof(T) * N];

  size_t _depth;

  T *
  __at(size_t i) noexcept
  {
    return reinterpret_cast<T *>(stack) + i;
  }

  const T *
  __at(size_t i) const noexcept
  {
    return reinterpret_cast<const T *>(stack) + i;
  }

  void
  __destruct(size_t i) noexcept
  {
    if constexpr ( micron::is_class_v<T> )
      __at(i)->~T();
  }

  void
  __bulk_copy(const fixed_stack &src, size_t cnt) noexcept
  {
    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      if ( cnt )
        micron::memcpy(stack, src.stack, cnt * sizeof(T));
    } else {
      for ( size_t i = 0; i < cnt; ++i )
        new (__at(i)) T(*src.__at(i));
    }
    _depth = cnt;
  }

  void
  __bulk_move(fixed_stack &src, size_t cnt) noexcept
  {
    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      if ( cnt )
        micron::memcpy(stack, src.stack, cnt * sizeof(T));

    } else {
      for ( size_t i = 0; i < cnt; ++i ) {
        new (__at(i)) T(micron::move(*src.__at(i)));
        src.__destruct(i);
      }
    }
    _depth = cnt;
    src._depth = 0;
  }

  template <typename U>
  fixed_stack
  __push_impl(U &&v) const
  {
    if ( _depth == N ) [[unlikely]]
      __builtin_trap();

    fixed_stack next;
    next.__bulk_copy(*this, _depth);
    new (next.__at(_depth)) T(micron::forward<U>(v));
    next._depth = _depth + 1;
    return next;
  }

  template <typename... Args>
  static fixed_stack
  __append_args(fixed_stack &&acc, size_t base, Args &&...args) noexcept
  {
    size_t i = base;

    ((new (acc.__at(i)) T(micron::forward<Args>(args)), ++i), ...);
    acc._depth = i;
    return micron::move(acc);
  }

public:
  using category_type = buffer_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef size_t size_type;
  typedef T value_type;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef const T *const_pointer;
  typedef const T *const_iterator;

  ~fixed_stack()
  {

    for ( size_t i = 0; i < _depth; ++i )
      __destruct(i);
  }

  fixed_stack() noexcept : _depth(0) {}

  fixed_stack(const fixed_stack &o) : _depth(0) { __bulk_copy(o, o._depth); }

  fixed_stack(fixed_stack &&o) noexcept : _depth(0) { __bulk_move(o, o._depth); }

  fixed_stack &
  operator=(const fixed_stack &o)
  {
    if ( this == &o )
      return *this;
    for ( size_t i = 0; i < _depth; ++i )
      __destruct(i);
    _depth = 0;
    __bulk_copy(o, o._depth);
    return *this;
  }

  fixed_stack &
  operator=(fixed_stack &&o) noexcept
  {
    if ( this == &o )
      return *this;
    for ( size_t i = 0; i < _depth; ++i )
      __destruct(i);
    _depth = 0;
    __bulk_move(o, o._depth);
    return *this;
  }

  fixed_stack
  push(const T &v) const
  {
    return __push_impl(v);
  }

  fixed_stack
  push(T &&v) const
  {
    return __push_impl(micron::move(v));
  }

  fixed_stack
  move(T &&v) const
  {
    return push(micron::move(v));
  }

  fixed_stack
  child(const T &v) const
  {
    return push(v);
  }

  fixed_stack
  child(T &&v) const
  {
    return push(micron::move(v));
  }

  template <typename... Args>
  fixed_stack
  push_range(Args &&...args) const
  {
    constexpr size_t K = sizeof...(Args);
    if ( _depth + K > N ) [[unlikely]]
      __builtin_trap();

    fixed_stack acc;
    acc.__bulk_copy(*this, _depth);
    return __append_args(micron::move(acc), _depth, micron::forward<Args>(args)...);
  }

  fixed_stack
  pop() const noexcept
  {
    size_t cnt = _depth > 0 ? _depth - 1 : 0;
    fixed_stack next;
    next.__bulk_copy(*this, cnt);
    return next;
  }

  fixed_stack
  parent() const noexcept
  {
    return pop();
  }

  struct PopResult {
    T value;
    fixed_stack stack;
  };

  PopResult
  try_pop() const
  {
    return { *__at(_depth - 1), pop() };
  }

  template <typename... Args>
  fixed_stack
  pop_range(Args &...args) const
  {
    static_assert((micron::is_same_v<micron::remove_cv_t<micron::remove_reference_t<Args>>, T> && ...),
                  "fixed_stack::pop_range: all output types must match value_type");
    constexpr size_t K = sizeof...(Args);

    {
      size_t idx = _depth;
      ((args = *__at(--idx)), ...);
    }

    size_t remain = _depth >= K ? _depth - K : 0;
    fixed_stack next;
    next.__bulk_copy(*this, remain);
    return next;
  }

  const T &
  val() const noexcept
  {
    return *__at(_depth - 1);
  }

  const T &
  top() const noexcept
  {
    return val();
  }

  bool
  is_empty() const noexcept
  {
    return _depth == 0;
  }

  bool
  empty() const noexcept
  {
    return is_empty();
  }

  size_type
  len() const noexcept
  {
    return _depth;
  }

  size_type
  size() const noexcept
  {
    return _depth;
  }

  static constexpr size_type
  max_size() noexcept
  {
    return N;
  }

  static constexpr size_type
  capacity() noexcept
  {
    return N;
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

  const fixed_stack *
  addr() const noexcept
  {
    return this;
  }

  fixed_stack *
  addr() noexcept
  {
    return this;
  }

  struct __at_range_itr {
    const fixed_stack *owner;
    i32 cur;

    __at_range_itr(const fixed_stack *o, i32 c) noexcept : owner(o), cur(c) {}

    bool
    operator==(const __at_range_itr &o) const noexcept
    {
      return cur == o.cur;
    }

    bool
    operator!=(const __at_range_itr &o) const noexcept
    {
      return cur != o.cur;
    }

    const T &
    operator*() const noexcept
    {
      return *owner->__at((size_t)cur);
    }

    __at_range_itr &
    operator++() noexcept
    {
      --cur;
      return *this;
    }

    __at_range_itr
    operator++(int) noexcept
    {
      __at_range_itr t(*this);
      ++(*this);
      return t;
    }
  };

  struct __at_range {
    __at_range_itr _b, _e;

    __at_range_itr
    begin() const noexcept
    {
      return _b;
    }

    __at_range_itr
    end() const noexcept
    {
      return _e;
    }
  };

  __at_range
  vals() const noexcept
  {
    i32 head = _depth > 0 ? (i32)(_depth - 1) : -1;
    return { __at_range_itr(this, head), __at_range_itr(this, -1) };
  }

  struct __range_itr {
    const fixed_stack *owner;
    i32 cur;

    __range_itr(const fixed_stack *o, i32 c) noexcept : owner(o), cur(c) {}

    bool
    operator==(const __range_itr &o) const noexcept
    {
      return cur == o.cur;
    }

    bool
    operator!=(const __range_itr &o) const noexcept
    {
      return cur != o.cur;
    }

    __range_itr &
    operator++() noexcept
    {
      --cur;
      return *this;
    }

    __range_itr
    operator++(int) noexcept
    {
      __range_itr t(*this);
      ++(*this);
      return t;
    }

    fixed_stack
    operator*() const noexcept
    {
      fixed_stack snap;
      size_t cnt = (size_t)(cur + 1);
      if constexpr ( micron::is_trivially_copyable_v<T> ) {
        if ( cnt )
          micron::memcpy(snap.stack, owner->stack, cnt * sizeof(T));
      } else {
        for ( size_t i = 0; i < cnt; ++i )
          new (snap.__at(i)) T(*owner->__at(i));
      }
      snap._depth = cnt;
      return snap;
    }
  };

  struct __range {
    __range_itr _b, _e;

    __range_itr
    begin() const noexcept
    {
      return _b;
    }

    __range_itr
    end() const noexcept
    {
      return _e;
    }
  };

  __range
  nodes() const noexcept
  {
    i32 head = _depth > 0 ? (i32)(_depth - 1) : -1;
    return { __range_itr(this, head), __range_itr(this, -1) };
  }

  bool
  operator==(const fixed_stack &o) const noexcept
  {
    if ( _depth != o._depth )
      return false;
    for ( size_t i = 0; i < _depth; ++i )
      if ( !(*__at(i) == *o.__at(i)) )
        return false;
    return true;
  }

  bool
  operator!=(const fixed_stack &o) const noexcept
  {
    return !(*this == o);
  }

  bool
  operator<(const fixed_stack &o) const noexcept
  {
    size_t n = (_depth < o._depth) ? _depth : o._depth;
    for ( size_t i = 0; i < n; ++i ) {
      if ( *__at(i) < *o.__at(i) )
        return true;
      if ( *o.__at(i) < *__at(i) )
        return false;
    }
    return _depth < o._depth;
  }

  bool
  operator>(const fixed_stack &o) const noexcept
  {
    return o < *this;
  }

  bool
  operator<=(const fixed_stack &o) const noexcept
  {
    return !(o < *this);
  }

  bool
  operator>=(const fixed_stack &o) const noexcept
  {
    return !(*this < o);
  }
};

}     // namespace micron
