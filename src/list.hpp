//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/algorithm.hpp"
#include "algorithm/memory.hpp"
#include "except.hpp"
#include "tags.hpp"
#include "types.hpp"

namespace micron
{
template<typename T> struct list_node {
  T data;
  list_node<T> *next;
};

// singly-linked list; traversal is node-pointer based
template<typename T> class list
{
  using list_node_t = list_node<T>;
  list_node_t *root;      // first node, or nullptr when empty

  inline void
  __impl_heap(T &&t, list_node_t **ptr)
  {
    *ptr = new list_node_t(micron::move(t), nullptr);
  }

  inline void
  __impl_heap(const T &t, list_node_t **ptr)
  {
    *ptr = new list_node_t(t, nullptr);
  }

  inline void
  __destroy()
  {
    list_node_t *ptr = root;
    while ( ptr != nullptr ) {
      list_node_t *nx = ptr->next;
      delete ptr;
      ptr = nx;
    }
    root = nullptr;
  }

  inline void
  __clone_from(const list_node_t *src)
  {
    list_node_t **tail = &root;
    for ( const list_node_t *p = src; p != nullptr; p = p->next ) {
      __impl_heap(p->data, tail);      // *tail = new node(p->data, nullptr)
      tail = &(*tail)->next;
    }
  }

public:
  using category_type = list_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef list_node_t *pointer;
  typedef const list_node_t *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  ~list() { __destroy(); }

  list() : root(nullptr) { }

  list(const size_t cnt) : root(nullptr)
  {
    if ( cnt == 0 ) return;
    __impl_heap(T(), &root);
    list_node_t *ptr = root;
    for ( size_t i = 1; i < cnt; i++ ) {
      __impl_heap(T(), &ptr->next);
      ptr = ptr->next;
    }
  }

  list(const size_t cnt, const T &t) : root(nullptr)
  {
    if ( cnt == 0 ) return;
    __impl_heap(t, &root);
    list_node_t *ptr = root;
    for ( size_t i = 1; i < cnt; i++ ) {
      __impl_heap(t, &ptr->next);
      ptr = ptr->next;
    }
  }

  list(const list &o) : root(nullptr) { __clone_from(o.root); }

  list(list &&o) : root(o.root) { o.root = nullptr; }

  list &
  operator=(const list &o)
  {
    if ( this == &o ) return *this;
    __destroy();
    __clone_from(o.root);
    return *this;
  }

  list &
  operator=(list &&o)
  {
    if ( this == &o ) return *this;
    __destroy();
    root = o.root;
    o.root = nullptr;
    return *this;
  }

  void
  clear()
  {
    __destroy();
  }

  const_pointer
  iend() const
  {
    if ( root == nullptr ) return nullptr;
    list_node_t *ptr = root;
    while ( ptr->next != nullptr ) ptr = ptr->next;
    return ptr;
  }

  const_pointer
  ibegin() const
  {
    return root;
  }

  const_iterator
  end() const
  {
    if ( root == nullptr ) return nullptr;
    list_node_t *ptr = root;
    while ( ptr->next != nullptr ) ptr = ptr->next;
    return &ptr->data;
  }

  const_iterator
  begin() const
  {
    if ( root == nullptr ) return nullptr;
    return &root->data;
  }

  void
  push_front(const T &v)
  {
    list_node_t *ptr;
    __impl_heap(v, &ptr);
    ptr->next = root;
    root = ptr;
  }

  void
  push_front(T &&v)
  {
    list_node_t *ptr;
    __impl_heap(micron::move(v), &ptr);
    ptr->next = root;
    root = ptr;
  }

  void
  push_back(const T &v)
  {
    list_node_t *ptr;
    __impl_heap(v, &ptr);
    if ( root == nullptr ) {
      root = ptr;
      return;
    }
    list_node_t *end_p = root;
    while ( end_p->next != nullptr ) end_p = end_p->next;
    end_p->next = ptr;
  }

  void
  push_back(T &&v)
  {
    list_node_t *ptr;
    __impl_heap(micron::move(v), &ptr);
    if ( root == nullptr ) {
      root = ptr;
      return;
    }
    list_node_t *end_p = root;
    while ( end_p->next != nullptr ) end_p = end_p->next;
    end_p->next = ptr;
  }

  const_iterator
  find(const T &srch) const
  {
    for ( const list_node_t *ptr = root; ptr != nullptr; ptr = ptr->next )
      if ( ptr->data == srch ) return &ptr->data;
    return nullptr;
  }

  const_pointer
  next(const_pointer itr, const size_t n = 0)
  {
    if ( itr == nullptr ) exc<except::runtime_error>("micron::next invalid iterator.");
    for ( size_t i = 0; i < (n + 1); i++ ) {
      if ( itr->next == nullptr ) break;
      itr = itr->next;
    }
    return itr;
  }

  void
  erase(const_pointer itr)
  {
    if ( itr == nullptr ) exc<except::runtime_error>("micron::erase invalid iterator.");
    if ( itr == root ) {
      root = root->next;
      delete const_cast<pointer>(itr);
      return;
    }
    list_node_t *ptr = root;
    while ( ptr != nullptr and ptr->next != itr ) ptr = ptr->next;
    if ( ptr == nullptr ) return;
    ptr->next = itr->next;
    delete const_cast<pointer>(itr);
  }

  void
  insert(const_pointer itr, const T &v)
  {
    if ( itr == nullptr ) exc<except::runtime_error>("micron::insert invalid iterator.");
    list_node_t *ptr;
    __impl_heap(v, &ptr);
    ptr->next = itr->next;
    const_cast<pointer>(itr)->next = ptr;
  }

  void
  insert(const_pointer itr, T &&v)
  {
    if ( itr == nullptr ) exc<except::runtime_error>("micron::insert invalid iterator.");
    list_node_t *ptr;
    __impl_heap(micron::move(v), &ptr);
    ptr->next = itr->next;
    const_cast<pointer>(itr)->next = ptr;
  }

  void
  emplace(const_pointer itr, T &&v)
  {
    if ( itr == nullptr ) exc<except::runtime_error>("micron::emplace invalid iterator.");
    list_node_t *ptr;
    __impl_heap(micron::move(v), &ptr);
    ptr->next = itr->next;
    const_cast<pointer>(itr)->next = ptr;
  }

  T
  front() const
  {
    if ( root == nullptr ) exc<except::runtime_error>("micron::list front() is empty");
    return root->data;
  }

  T
  back() const
  {
    if ( root == nullptr ) exc<except::runtime_error>("micron::list back() is empty");
    return *end();
  }

  void
  merge(list &o)
  {
    if ( &o == this ) return;
    if ( root == nullptr ) {
      root = o.root;
      o.root = nullptr;
      return;
    }
    auto *end_ptr = const_cast<pointer>(iend());
    end_ptr->next = o.root;
    o.root = nullptr;
  }

  void
  merge(list &&o)
  {
    if ( &o == this ) return;
    if ( root == nullptr ) {
      root = o.root;
      o.root = nullptr;
      return;
    }
    auto *end_ptr = const_cast<pointer>(iend());
    end_ptr->next = o.root;
    o.root = nullptr;
  }

  void
  splice(const_pointer pos, list &o)
  {
    if ( &o == this or o.root == nullptr ) return;
    if ( pos == nullptr ) exc<except::runtime_error>("micron::splice invalid position.");
    auto *p = const_cast<pointer>(pos);
    auto *otail = const_cast<pointer>(o.iend());
    otail->next = p->next;
    p->next = o.root;
    o.root = nullptr;
  }

  bool
  operator==(const list &o) const
  {
    const list_node_t *a = root;
    const list_node_t *b = o.root;
    while ( a != nullptr and b != nullptr ) {
      if ( !(a->data == b->data) ) return false;
      a = a->next;
      b = b->next;
    }
    return a == nullptr and b == nullptr;
  }

  bool
  operator!=(const list &o) const
  {
    return !(*this == o);
  }

  bool
  empty() const
  {
    return root == nullptr;
  }

  size_t
  size() const
  {
    size_t cnt = 0;
    for ( const list_node_t *ptr = root; ptr != nullptr; ptr = ptr->next ) cnt++;
    return cnt;
  }

  size_t
  max_size() const
  {
    return 0xFFFFFFFFFFFFFFF;
  }

  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    for ( list_node_t *ptr = root; ptr != nullptr; ptr = ptr->next ) fn(ptr->data);
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    for ( const list_node_t *ptr = root; ptr != nullptr; ptr = ptr->next ) fn(static_cast<const T &>(ptr->data));
  }
};
};      // namespace micron
