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
template<typename T> struct double_list_node {
  T data;
  double_list_node<T> *prev;
  double_list_node<T> *next;
};

template<typename T> class double_list
{
  using double_list_node_t = double_list_node<T>;
  double_list_node_t *root;      // first node, or nullptr when empty

  inline void
  __impl_heap(T &&t, double_list_node_t **ptr)
  {
    *ptr = new double_list_node_t(micron::move(t), nullptr, nullptr);
  }

  inline void
  __impl_heap(const T &t, double_list_node_t **ptr)
  {
    *ptr = new double_list_node_t(t, nullptr, nullptr);
  }

  // tear the whole chain down, destroying each node exactly once
  inline void
  __destroy()
  {
    double_list_node_t *ptr = root;
    while ( ptr != nullptr ) {
      double_list_node_t *nx = ptr->next;
      delete ptr;      // ~double_list_node destroys data once
      ptr = nx;
    }
    root = nullptr;
  }

  // deep-copy src's chain into a freshly empty (root == nullptr) list
  inline void
  __clone_from(const double_list_node_t *src)
  {
    double_list_node_t **tail = &root;
    double_list_node_t *prev = nullptr;
    for ( const double_list_node_t *p = src; p != nullptr; p = p->next ) {
      __impl_heap(p->data, tail);      // *tail = new node(p->data, nullptr, nullptr)
      (*tail)->prev = prev;
      prev = *tail;
      tail = &(*tail)->next;
    }
  }

public:
  using category_type = double_list_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef double_list_node_t *pointer;
  typedef const double_list_node_t *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  double_list() : root(nullptr) { }

  double_list(const size_t cnt) : root(nullptr)
  {
    if ( cnt == 0 ) return;
    __impl_heap(T(), &root);
    double_list_node_t *ptr = root;
    for ( size_t i = 1; i < cnt; i++ ) {
      __impl_heap(T(), &ptr->next);
      ptr->next->prev = ptr;
      ptr = ptr->next;
    }
  }

  double_list(const size_t cnt, const T &t) : root(nullptr)
  {
    if ( cnt == 0 ) return;
    __impl_heap(t, &root);
    double_list_node_t *ptr = root;
    for ( size_t i = 1; i < cnt; i++ ) {
      __impl_heap(t, &ptr->next);
      ptr->next->prev = ptr;
      ptr = ptr->next;
    }
  }

  double_list(const double_list &o) : root(nullptr) { __clone_from(o.root); }

  double_list(double_list &&o) : root(o.root) { o.root = nullptr; }

  double_list &
  operator=(const double_list &o)
  {
    if ( this == &o ) return *this;
    __destroy();
    __clone_from(o.root);
    return *this;
  }

  double_list &
  operator=(double_list &&o)
  {
    if ( this == &o ) return *this;
    __destroy();
    root = o.root;
    o.root = nullptr;
    return *this;
  }

  ~double_list() { __destroy(); }

  void
  clear()
  {
    __destroy();
  }

  // last node (nullptr when empty)
  const_pointer
  iend() const
  {
    if ( root == nullptr ) return nullptr;
    double_list_node_t *ptr = root;
    while ( ptr->next != nullptr ) ptr = ptr->next;
    return ptr;
  }

  // first node (nullptr when empty)
  const_pointer
  ibegin() const
  {
    return root;
  }

  // address of last element's data (nullptr when empty)
  const_iterator
  end() const
  {
    if ( root == nullptr ) return nullptr;
    double_list_node_t *ptr = root;
    while ( ptr->next != nullptr ) ptr = ptr->next;
    return &ptr->data;
  }

  // address of first element's data (nullptr when empty)
  const_iterator
  begin() const
  {
    if ( root == nullptr ) return nullptr;
    return &root->data;
  }

  void
  push_front(const T &v)
  {
    double_list_node_t *ptr;
    __impl_heap(v, &ptr);
    ptr->next = root;
    if ( root != nullptr ) root->prev = ptr;
    root = ptr;
  }

  void
  push_front(T &&v)
  {
    double_list_node_t *ptr;
    __impl_heap(micron::move(v), &ptr);
    ptr->next = root;
    if ( root != nullptr ) root->prev = ptr;
    root = ptr;
  }

  void
  push_back(const T &v)
  {
    double_list_node_t *ptr;
    __impl_heap(v, &ptr);
    if ( root == nullptr ) {
      root = ptr;
      return;
    }
    double_list_node_t *end_p = root;
    while ( end_p->next != nullptr ) end_p = end_p->next;
    end_p->next = ptr;
    ptr->prev = end_p;
  }

  void
  push_back(T &&v)
  {
    double_list_node_t *ptr;
    __impl_heap(micron::move(v), &ptr);
    if ( root == nullptr ) {
      root = ptr;
      return;
    }
    double_list_node_t *end_p = root;
    while ( end_p->next != nullptr ) end_p = end_p->next;
    end_p->next = ptr;
    ptr->prev = end_p;
  }

  // address of the matching element's data, or nullptr if absent
  const_iterator
  find(const T &srch) const
  {
    for ( const double_list_node_t *ptr = root; ptr != nullptr; ptr = ptr->next )
      if ( ptr->data == srch ) return &ptr->data;
    return nullptr;
  }

  // advance a node pointer forward by (n + 1); clamps at the last node
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

  // step a node pointer backward by (n + 1); clamps at the first node
  const_pointer
  prev(const_pointer itr, const size_t n = 0)
  {
    if ( itr == nullptr ) exc<except::runtime_error>("micron::prev invalid iterator.");
    for ( size_t i = 0; i < (n + 1); i++ ) {
      if ( itr->prev == nullptr ) break;
      itr = itr->prev;
    }
    return itr;
  }

  // remove the node itr points to, fixing up both neighbours
  void
  erase(const_pointer itr)
  {
    if ( itr == nullptr ) exc<except::runtime_error>("micron::erase invalid iterator.");
    auto *p = const_cast<pointer>(itr);
    if ( p->prev != nullptr )
      p->prev->next = p->next;
    else
      root = p->next;      // p was the head
    if ( p->next != nullptr ) p->next->prev = p->prev;
    delete p;      // ~double_list_node destroys data once
  }

  // insert a new node after itr
  void
  insert(const_pointer itr, const T &v)
  {
    if ( itr == nullptr ) exc<except::runtime_error>("micron::insert invalid iterator.");
    double_list_node_t *ptr;
    __impl_heap(v, &ptr);
    auto *p = const_cast<pointer>(itr);
    ptr->next = p->next;
    ptr->prev = p;
    if ( p->next != nullptr ) p->next->prev = ptr;
    p->next = ptr;
  }

  void
  insert(const_pointer itr, T &&v)
  {
    if ( itr == nullptr ) exc<except::runtime_error>("micron::insert invalid iterator.");
    double_list_node_t *ptr;
    __impl_heap(micron::move(v), &ptr);
    auto *p = const_cast<pointer>(itr);
    ptr->next = p->next;
    ptr->prev = p;
    if ( p->next != nullptr ) p->next->prev = ptr;
    p->next = ptr;
  }

  void
  emplace(const_pointer itr, T &&v)
  {
    if ( itr == nullptr ) exc<except::runtime_error>("micron::emplace invalid iterator.");
    double_list_node_t *ptr;
    __impl_heap(micron::move(v), &ptr);
    auto *p = const_cast<pointer>(itr);
    ptr->next = p->next;
    ptr->prev = p;
    if ( p->next != nullptr ) p->next->prev = ptr;
    p->next = ptr;
  }

  T
  front() const
  {
    if ( root == nullptr ) exc<except::runtime_error>("micron::double_list front() is empty");
    return root->data;
  }

  T
  back() const
  {
    if ( root == nullptr ) exc<except::runtime_error>("micron::double_list back() is empty");
    return *end();
  }

  void
  merge(double_list &o)
  {
    if ( &o == this or o.root == nullptr ) return;
    if ( root == nullptr ) {
      root = o.root;
      o.root = nullptr;
      return;
    }
    auto *end_ptr = const_cast<pointer>(iend());
    end_ptr->next = o.root;
    o.root->prev = end_ptr;
    o.root = nullptr;
  }

  void
  merge(double_list &&o)
  {
    if ( &o == this or o.root == nullptr ) return;
    if ( root == nullptr ) {
      root = o.root;
      o.root = nullptr;
      return;
    }
    auto *end_ptr = const_cast<pointer>(iend());
    end_ptr->next = o.root;
    o.root->prev = end_ptr;
    o.root = nullptr;
  }

  // splice o's entire chain in right after pos
  void
  splice(const_pointer pos, double_list &o)
  {
    if ( &o == this or o.root == nullptr ) return;
    if ( pos == nullptr ) exc<except::runtime_error>("micron::splice invalid position.");
    auto *p = const_cast<pointer>(pos);
    auto *ohead = o.root;
    auto *otail = const_cast<pointer>(o.iend());
    otail->next = p->next;
    if ( p->next != nullptr ) p->next->prev = otail;
    p->next = ohead;
    ohead->prev = p;
    o.root = nullptr;
  }

  bool
  operator==(const double_list &o) const
  {
    const double_list_node_t *a = root;
    const double_list_node_t *b = o.root;
    while ( a != nullptr and b != nullptr ) {
      if ( !(a->data == b->data) ) return false;
      a = a->next;
      b = b->next;
    }
    return a == nullptr and b == nullptr;
  }

  bool
  operator!=(const double_list &o) const
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
    for ( const double_list_node_t *ptr = root; ptr != nullptr; ptr = ptr->next ) cnt++;
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
    for ( double_list_node_t *ptr = root; ptr != nullptr; ptr = ptr->next ) fn(ptr->data);
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    for ( const double_list_node_t *ptr = root; ptr != nullptr; ptr = ptr->next ) fn(static_cast<const T &>(ptr->data));
  }
};
};      // namespace micron
