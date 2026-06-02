//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../alloc.hpp"
#include "../concepts.hpp"
#include "../pointer.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../__special/initializer_list"

#include "stack.hpp"

namespace micron
{

// an immutable / persistent LIFO stack
// refcounted, immutable singly-linked cons-list
template<is_movable_object t> class istack
{
  struct node {
    t value;
    node *next;
    usize refs;      // number of incoming references (heads + successor .next)
  };

  node *head = nullptr;
  usize len = 0;

  template<class U>
  node *
  __cons(U &&v, node *tail)
  {
    node *m = micron::alloc<node>(sizeof(node));
    if ( !m ) [[unlikely]]
      exc<except::library_error>("micron::istack: node allocation failed");
#ifndef __micron_freestanding
    try {
      new (micron::addr(m->value)) t(micron::forward<U>(v));
    } catch ( ... ) {
      micron::free(m);
      throw;
    }
#else
    new (micron::addr(m->value)) t(micron::forward<U>(v));
#endif
    m->next = tail;
    m->refs = 1;
    return m;
  }

  node *
  __cons_default(node *tail)
  {
    node *m = micron::alloc<node>(sizeof(node));
    if ( !m ) [[unlikely]]
      exc<except::library_error>("micron::istack: node allocation failed");
#ifndef __micron_freestanding
    try {
      new (micron::addr(m->value)) t();
    } catch ( ... ) {
      micron::free(m);
      throw;
    }
#else
    new (micron::addr(m->value)) t();
#endif
    m->next = tail;
    m->refs = 1;
    return m;
  }

  static void
  __drop(node *p) noexcept
  {
    while ( p ) {
      if ( --p->refs != 0 ) break;
      node *nx = p->next;
      if constexpr ( micron::is_class<t>::value ) p->value.~t();
      micron::free(p);
      p = nx;
    }
  }

  static node *
  __share(node *p) noexcept
  {
    if ( p ) ++p->refs;
    return p;
  }

  istack(node *h, usize l) noexcept : head(h), len(l) { }

public:
  using category_type = buffer_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef t value_type;
  typedef const t &reference;
  typedef const t &ref;
  typedef const t &const_reference;
  typedef const t &const_ref;
  typedef const t *pointer;
  typedef const t *const_pointer;
  typedef const t *iterator;
  typedef const t *const_iterator;

  ~istack() { __drop(head); }

  istack() noexcept : head(nullptr), len(0) { }

  explicit istack(const umax_t n) : head(nullptr), len(0)
  {
    for ( umax_t i = 0; i < n; ++i ) {
      head = __cons_default(head);
      ++len;
    }
  }

  istack(const std::initializer_list<t> &lst) : head(nullptr), len(0)
  {
    for ( const t &v : lst ) {
      head = __cons(v, head);
      ++len;
    }
  }

  template<typename K = t, usize SN, class SA>
    requires(micron::is_convertible_v<K, t>)
  istack(const stack<K, SN, SA> &o) : head(nullptr), len(0)
  {
    for ( usize i = o.size(); i-- > 0; ) {
      head = __cons(o[i], head);      // o[size-1] (bottom) first ... o[0] (top) last
      ++len;
    }
  }

  istack(const istack &o) noexcept : head(__share(o.head)), len(o.len) { }

  istack(istack &&o) noexcept : head(o.head), len(o.len)
  {
    o.head = nullptr;
    o.len = 0;
  }

  istack &
  operator=(const istack &o) noexcept
  {
    if ( this == &o ) return *this;
    node *old = head;
    head = __share(o.head);
    len = o.len;
    __drop(old);
    return *this;
  }

  istack &
  operator=(istack &&o) noexcept
  {
    if ( this == &o ) return *this;
    __drop(head);
    head = o.head;
    len = o.len;
    o.head = nullptr;
    o.len = 0;
    return *this;
  }

  const t &
  top() const
  {
    if ( !head ) [[unlikely]]
      exc<except::library_error>("micron::istack top() called on empty stack");
    return head->value;
  }

  const t &
  operator()() const
  {
    return top();
  }

  const t &
  operator[](const umax_t n) const
  {
    if ( n >= len ) [[unlikely]]
      exc<except::library_error>("micron::istack operator[] out of range");
    const node *p = head;
    for ( umax_t i = 0; i < n; ++i ) p = p->next;
    return p->value;
  }

  [[nodiscard]] istack
  push(const t &v) const
  {
    node *m = const_cast<istack *>(this)->__cons(v, head);
    __share(head);      // *this keeps head; the new node also references it
    return istack(m, len + 1);
  }

  [[nodiscard]] istack
  push(t &&v) const
  {
    node *m = const_cast<istack *>(this)->__cons(micron::move(v), head);
    __share(head);
    return istack(m, len + 1);
  }

  [[nodiscard]] istack
  push() const
  {
    node *m = const_cast<istack *>(this)->__cons_default(head);
    __share(head);
    return istack(m, len + 1);
  }

  [[nodiscard]] istack
  pop() const
  {
    if ( !head ) [[unlikely]]
      exc<except::library_error>("micron::istack pop() called on empty stack");
    return istack(__share(head->next), len - 1);
  }

  [[nodiscard]] istack
  clear() const
  {
    return istack();
  }

  void
  swap(istack &o) noexcept
  {
    micron::swap(head, o.head);
    micron::swap(len, o.len);
  }

  [[nodiscard]] bool
  empty() const noexcept
  {
    return head == nullptr;
  }

  [[nodiscard]] usize
  size() const noexcept
  {
    return len;
  }

  [[nodiscard]] constexpr usize
  max_size() const noexcept
  {
    return static_cast<usize>(-1);
  }

  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<t>;
  }
};

};      // namespace micron
