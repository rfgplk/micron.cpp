//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/memory.hpp"
#include "new.hpp"
#include "type_traits.hpp"
#include "types.hpp"

namespace micron
{

template <typename T, typename C> struct __binomial_node {
  T value;
  size_t degree;
  __binomial_node *parent;
  __binomial_node *child;
  __binomial_node *sibling;

  __binomial_node(const T &val) : value(val), degree(0), parent(nullptr), child(nullptr), sibling(nullptr) {}

  __binomial_node(T &&val) : value(micron::move(val)), degree(0), parent(nullptr), child(nullptr), sibling(nullptr) {}

  template <typename... Args>
  __binomial_node(Args &&...args) : value(micron::forward<Args>(args)...), degree(0), parent(nullptr), child(nullptr), sibling(nullptr)
  {
  }
};

template <typename T, typename C = micron::less<T>> struct __binomial_heap {
  using node_type = __binomial_node<T, C>;

  node_type *root_list;
  node_type *min_node;
  size_t node_count;
  C comp;

  ~__binomial_heap() { clear(); }

  __binomial_heap() : root_list(nullptr), min_node(nullptr), node_count(0), comp() {}

  __binomial_heap(const C &c) : root_list(nullptr), min_node(nullptr), node_count(0), comp(c) {}

  inline void
  link(node_type *y, node_type *z)
  {
    y->parent = z;
    y->sibling = z->child;
    z->child = y;
    z->degree++;
  }

  inline node_type *
  merge_root_lists(node_type *h1, node_type *h2)
  {
    if ( !h1 )
      return h2;
    if ( !h2 )
      return h1;

    node_type *head;
    node_type **pos = &head;

    while ( h1 && h2 ) {
      if ( h1->degree <= h2->degree ) {
        *pos = h1;
        h1 = h1->sibling;
      } else {
        *pos = h2;
        h2 = h2->sibling;
      }
      pos = &((*pos)->sibling);
    }

    *pos = h1 ? h1 : h2;
    return head;
  }

  inline void
  union_heap(__binomial_heap &other)
  {
    if ( !other.root_list )
      return;

    root_list = merge_root_lists(root_list, other.root_list);
    other.root_list = nullptr;

    if ( !root_list ) {
      min_node = nullptr;
      node_count += other.node_count;
      other.node_count = 0;
      return;
    }

    node_type *prev = nullptr;
    node_type *curr = root_list;
    node_type *next = curr->sibling;

    while ( next ) {
      if ( (curr->degree != next->degree) || (next->sibling && next->sibling->degree == curr->degree) ) {
        prev = curr;
        curr = next;
      } else {
        if ( comp(curr->value, next->value) ) {
          curr->sibling = next->sibling;
          link(next, curr);
        } else {
          if ( !prev )
            root_list = next;
          else
            prev->sibling = next;
          link(curr, next);
          curr = next;
        }
      }
      next = curr->sibling;
    }

    update_min();
    node_count += other.node_count;
    other.node_count = 0;
    other.min_node = nullptr;
  }

  inline void
  update_min()
  {
    if ( !root_list ) {
      min_node = nullptr;
      return;
    }

    min_node = root_list;
    node_type *curr = root_list->sibling;

    while ( curr ) {
      if ( comp(curr->value, min_node->value) )
        min_node = curr;
      curr = curr->sibling;
    }
  }

  inline node_type *
  insert(const T &value)
  {
    node_type *new_node = new node_type(value);
    __binomial_heap temp(comp);
    temp.root_list = new_node;
    temp.min_node = new_node;
    temp.node_count = 1;

    union_heap(temp);
    return new_node;
  }

  inline node_type *
  insert(T &&value)
  {
    node_type *new_node = new node_type(micron::move(value));
    __binomial_heap temp(comp);
    temp.root_list = new_node;
    temp.min_node = new_node;
    temp.node_count = 1;

    union_heap(temp);
    return new_node;
  }

  template <typename... Args>
  inline node_type *
  emplace(Args &&...args)
  {
    node_type *new_node = new node_type(micron::forward<Args>(args)...);
    __binomial_heap temp(comp);
    temp.root_list = new_node;
    temp.min_node = new_node;
    temp.node_count = 1;

    union_heap(temp);
    return new_node;
  }

  inline const T &
  find_min() const
  {
    return min_node->value;
  }

  inline T
  extract_min()
  {
    if ( !min_node )
      return T{};

    T min_value = micron::move(min_node->value);

    node_type *prev = nullptr;
    node_type *curr = root_list;

    while ( curr != min_node ) {
      prev = curr;
      curr = curr->sibling;
    }

    if ( !prev )
      root_list = min_node->sibling;
    else
      prev->sibling = min_node->sibling;

    node_type *child = min_node->child;
    node_type *reversed = nullptr;

    while ( child ) {
      node_type *next = child->sibling;
      child->sibling = reversed;
      child->parent = nullptr;
      reversed = child;
      child = next;
    }

    __binomial_heap child_heap(comp);
    child_heap.root_list = reversed;
    child_heap.node_count = 0;

    delete min_node;
    node_count--;

    union_heap(child_heap);

    return min_value;
  }

  inline void
  decrease_key(node_type *node, const T &new_value)
  {
    if ( !comp(new_value, node->value) )
      return;

    node->value = new_value;

    node_type *curr = node;
    node_type *parent = curr->parent;

    while ( parent && comp(curr->value, parent->value) ) {
      micron::swap(curr->value, parent->value);
      curr = parent;
      parent = curr->parent;
    }

    if ( !min_node || comp(node->value, min_node->value) )
      min_node = node;
  }

  inline void
  delete_node(node_type *node)
  {
    node_type *curr = node;
    node_type *parent = curr->parent;

    while ( parent ) {
      micron::swap(curr->value, parent->value);
      curr = parent;
      parent = curr->parent;
    }

    extract_min();
  }

  inline node_type *
  find(const T &value)
  {
    return find_recursive(root_list, value);
  }

  inline node_type *
  find_recursive(node_type *root, const T &value)
  {
    if ( !root )
      return nullptr;

    if ( root->value == value )
      return root;

    node_type *result = find_recursive(root->child, value);
    if ( result )
      return result;

    return find_recursive(root->sibling, value);
  }

  inline void
  meld(__binomial_heap &other)
  {
    union_heap(other);
  }

  inline void
  clear()
  {
    clear_recursive(root_list);
    root_list = nullptr;
    min_node = nullptr;
    node_count = 0;
  }

  inline void
  clear_recursive(node_type *node)
  {
    if ( !node )
      return;

    clear_recursive(node->child);
    clear_recursive(node->sibling);
    delete node;
  }

  inline bool
  empty() const
  {
    return node_count == 0;
  }

  inline size_t
  size() const
  {
    return node_count;
  }
};

};
