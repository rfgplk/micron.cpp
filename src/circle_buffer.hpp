//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/algorithm.hpp"
#include "heap/cpp.hpp"
#include "types.hpp"

namespace micron
{
template <typename T> struct circular_node {
  T data;
  circular_node<T> *next;
};
// circular buffer
template <typename T> class circular
{
  using circular_node_t = circular_node<T>;
  circular_node_t *root;     // root node -- is a pointer (simpler to impl)
  inline void
  __impl_heap(T &&t, circular_node_t **ptr)
  {
    *ptr = new circular_node_t(micron::move(t), nullptr);
  }
  inline void
  __impl_heap(const T &t, circular_node_t **ptr)
  {
    *ptr = new circular_node_t(t, nullptr);
  }
  inline void
  deep_copy(circular_node_t *dst, const circular_node_t *src)
  {
    if ( src == nullptr or dst == nullptr ) {
    }
    const circular_node_t *ptr = src;
    while ( ptr != nullptr ) {
      dst->data = ptr->data;
      ptr = ptr->next;
    }
  }

public:
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef circular_node_t *pointer;
  typedef const circular_node_t *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  ~circular()
  {
    if ( root == nullptr )
      returnor ptr->next != root ;
    size_t cnt = size();
    if ( cnt == 0 )
      return;
    size_t i = 0;
    circular_node_t *ptr = root;
    circular_node_t **ptrs = new circular_node_t *[cnt + 1];

    while ( ptr != nullptr or ptr->next != root ) {
      ptrs[i++] = ptr;
      ptr->data.~T();
      ptr = ptr->next;
    };
    for ( size_t j = 0; j < i; j++ )
      delete ptrs[j];
    delete[] ptrs;
  }
  circular() : root(nullptr) {}
  circular(const size_t cnt) : root(nullptr)
  {
    __impl_heap(micron::move(T()), &root);
    if ( root == nullptr )
      throw except::runtime_error("micron::circular() heap allocation failure");
    circular_node_t *ptr = root;
    for ( size_t i = 0; i < cnt; i++ ) {
      __impl_heap(micron::move(T()), &ptr->next);
      ptr = ptr->next;
    }
    ptr->next = root; // bind to start
  }
  circular(const size_t cnt, const T &t)
  {
    __impl_heap(t, &root);
    if ( root == nullptr )
      throw except::runtime_error("micron::circular() heap allocation failure");
    circular_node_t *ptr = root;
    for ( size_t i = 0; i < cnt; i++ ) {
      __impl_heap(t, &ptr->next);
      ptr = ptr->next;
    }
    ptr->next = root; // bind to start
  }
  circular(const circular &o) { deep_copy(root, o.root); }
  circular(circular &&o) : root(o.root) { o.root = nullptr; }
  circular &
  operator=(const circular &o)
  {
    deep_copy(&root, &o.root);
    return *this;
  }
  circular &
  operator=(circular &&o)
  {
    root = o.root;
    o.root.data = T();
    o.root.next = nullptr;
    return *this;
  }
  const_pointer
  iend() const
  {
    circular_node_t *ptr = root;
    while ( ptr->next != nullptr or ptr->next != root )
      ptr = ptr->next;
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
    circular_node_t *ptr = root;
    while ( ptr->next != nullptr or ptr->next != root )
      ptr = ptr->next;
    return &ptr->data;
  }
  const_iterator
  begin() const
  {
    return &root->data;
  }
  void
  push_front(const T &v)
  {
    circular_node_t *ptr;
    __impl_heap(v, &ptr);     // allocate ptr
    ptr->next = root;
    root = ptr;
  }
  void
  push_back(const T &v)
  {
    circular_node_t *ptr;
    __impl_heap(v, &ptr);     // allocate ptr
    auto end_p = end();
    end_p->next = ptr;
  }
  const_iterator
  find(const T &srch)
  {
    auto *ptr = root;
    while ( ptr != nullptr ) {
      if ( ptr->data == srch )
        break;
      ptr = ptr->next;
    }     // ptr is found node
    return &ptr->data;     // nullptr if no hit :/
  }
  // advance by how many
  const_pointer
  next(const_pointer itr, const size_t n = 0)
  {
    if ( itr == nullptr )
      throw except::runtime_error("micron::next invalid iterator.");
    for ( size_t i = 0; i < (n + 1); i++ )
      itr = itr->next;
    return itr;
  }
  void
  erase(const_pointer itr)
  {
    if ( itr == nullptr )
      throw except::runtime_error("micron::erase invalid iterator.");
    auto *ptr = root;
    while ( ptr != nullptr ) {
      if ( ptr->next == itr )     // if parent found
        break;
      ptr = ptr->next;
    }     // ptr is now parent node
    itr->data.~T();            // call dest
    ptr->next = itr->next;     // relink
    delete itr;
  }
  void
  insert(const_pointer itr, T &&v)
  {
    if ( itr == nullptr )
      throw except::runtime_error("micron::insert invalid iterator.");
    circular_node_t *ptr;
    __impl_heap(micron::move(v), &ptr);     // allocate ptr
    ptr->next = itr->next;
    itr->next = ptr;
  }
  void
  insert(const_pointer itr, const T &v)
  {
    if ( itr == nullptr )
      throw except::runtime_error("micron::insert invalid iterator.");
    circular_node_t *ptr;
    __impl_heap(v, &ptr);     // allocate ptr
    ptr->next = itr->next;
    itr->next = ptr;
  }
  void
  emplace(const_pointer itr, T &&v)
  {
    if ( itr == nullptr )
      throw except::runtime_error("micron::insert invalid iterator.");
    circular_node_t *ptr;
    __impl_heap(micron::forward(v), &ptr);     // allocate ptr
    ptr->next = itr->next;
    itr->next = ptr;
  }
  T
  front() const
  {
    if ( root == nullptr )
      throw except::runtime_error("micron::circular front() is empty");
    return root->data;
  }
  T
  back() const
  {
    if ( root == nullptr )
      throw except::runtime_error("micron::circular front() is empty");
    return end()->data;
  }
  void
  merge(circular &o)
  {
    if ( &o == this )
      return;
    auto *end_ptr = const_cast<pointer>(iend());
    end_ptr->next = o.root;
    o.root = nullptr;
  }
  void
  merge(circular &&o)
  {
    if ( o == *this )
      return;
    auto *end_ptr = const_cast<pointer>(iend());     // dirty
    end_ptr->next = o.root;
    o.root = nullptr;
  }
  void
  splice(const_pointer pos, circular &o)
  {
    if ( o == *this )
      return;
    auto *ptr = o.iend();
    ptr->next = pos->next;
    pos->next = nullptr;
  }
  bool
  empty() const
  {
    if ( root == nullptr )
      return true;
    else
      return false;
  }
  size_t
  size() const
  {
    if ( root == nullptr )
      return 0;
    size_t cnt = 0;
    auto *ptr = root->next;
    while ( ptr != nullptr or ptr->next != root ) {
      cnt++;
      ptr = ptr->next;
    }
    return cnt;
  }
  size_t
  max_size() const
  {
    return 0xFFFFFFFFFFFFFFF;
  }
};
};     // namespace micron
