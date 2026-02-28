//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../memory/allocation/chunks.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"

template <typename T, class Alloc = micron::allocator_serial<>> class quake_heap : private Alloc, public immutable_memory<T>
{
  struct node {
    T value;
    node *left;
    node *right;
    usize size;

    node(const T &v) : value(v), left(nullptr), right(nullptr), size(1) {}

    node(T &&v) noexcept : value(micron::move(v)), left(nullptr), right(nullptr), size(1) {}
  };

  node **roots;
  usize levels;

  inline void
  update_size(node *n) noexcept
  {
    n->size = 1;
    if ( n->left )
      n->size += n->left->size;
    if ( n->right )
      n->size += n->right->size;
  }

  node *
  merge(node *a, node *b) noexcept
  {
    if ( !a )
      return b;
    if ( !b )
      return a;
    if ( a->value < b->value )
      micron::swap(a, b);
    a->right = merge(a->right, b);
    if ( !a->left || a->left->size < a->right->size )
      micron::swap(a->left, a->right);
    update_size(a);
    return a;
  }

  node *
  insert_node(node *root, T &&v)
  {
    node *n = new node(micron::move(v));
    return merge(root, n);
  }

  node *
  pop_node(node *root, T &out)
  {
    if ( !root )
      exc<except::library_error>("quake_heap::pop() empty");
    out = micron::move(root->value);
    node *res = merge(root->left, root->right);
    delete root;
    return res;
  }

  void
  clear_tree(node *n) noexcept
  {
    if ( !n )
      return;
    clear_tree(n->left);
    clear_tree(n->right);
    delete n;
  }

public:
  using category_type = theap_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  using value_type = T;
  using size_type = usize;
  using reference = T &;
  using const_reference = const T &;

  quake_heap(usize max_levels = 32) : levels(max_levels)
  {
    roots = static_cast<node **>(this->create(levels * sizeof(node *)));
    for ( usize i = 0; i < levels; i++ )
      roots[i] = nullptr;
  }

  ~quake_heap()
  {
    clear();
    this->destroy(to_chunk(roots, levels * sizeof(node *)));
  }

  quake_heap(const quake_heap &o) = delete;

  quake_heap(quake_heap &&o) noexcept : roots(o.roots), levels(o.levels)
  {
    o.roots = nullptr;
    o.levels = 0;
  }

  quake_heap &operator=(const quake_heap &o) = delete;

  quake_heap &
  operator=(quake_heap &&o) noexcept
  {
    if ( this != &o ) {
      clear();
      this->destroy(to_chunk(roots, levels * sizeof(node *)));
      roots = o.roots;
      levels = o.levels;
      o.roots = nullptr;
      o.levels = 0;
    }
    return *this;
  }

  void
  insert(T &&v)
  {
    roots[0] = insert_node(roots[0], micron::move(v));
    for ( usize i = 0; i + 1 < levels; i++ ) {
      if ( roots[i] && roots[i]->size >= (1ULL << i) ) {
        roots[i + 1] = merge(roots[i + 1], roots[i]);
        roots[i] = nullptr;
      }
    }
  }

  void
  insert(const T &v)
  {
    insert(T(v));
  }

  reference
  max()
  {
    usize max__n = SIZE_MAX;
    for ( usize i = 0; i < levels; i++ ) {
      if ( roots[i] ) {
        if ( max__n == SIZE_MAX || roots[i]->value > roots[max__n]->value )
          max__n = i;
      }
    }
    if ( max__n == SIZE_MAX )
      exc<except::library_error>("quake_heap::max() empty");
    return roots[max__n]->value;
  }

  T
  pop()
  {
    usize max__n = SIZE_MAX;
    for ( usize i = 0; i < levels; i++ ) {
      if ( roots[i] ) {
        if ( max__n == SIZE_MAX || roots[i]->value > roots[max__n]->value )
          max__n = i;
      }
    }
    if ( max__n == SIZE_MAX )
      exc<except::library_error>("quake_heap::pop() empty");
    T v;
    roots[max__n] = pop_node(roots[max__n], v);
    return v;
  }

  bool
  empty() const noexcept
  {
    for ( usize i = 0; i < levels; i++ )
      if ( roots[i] )
        return false;
    return true;
  }

  usize
  size() const noexcept
  {
    usize total = 0;
    for ( usize i = 0; i < levels; i++ )
      if ( roots[i] )
        total += roots[i]->size;
    return total;
  }

  void
  clear() noexcept
  {
    for ( usize i = 0; i < levels; i++ ) {
      clear_tree(roots[i]);
      roots[i] = nullptr;
    }
  }
};
