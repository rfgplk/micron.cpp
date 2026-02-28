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

template <typename T, class Alloc = micron::allocator_serial<>> class fibonacci_heap : private Alloc, public immutable_memory<T>
{
  struct node {
    T value;
    node *parent;
    node *child;
    node *left;
    node *right;
    usize degree;
    bool mark;

    node(const T &v) : value(v), parent(nullptr), child(nullptr), left(this), right(this), degree(0), mark(false) {}

    node(T &&v) noexcept : value(micron::move(v)), parent(nullptr), child(nullptr), left(this), right(this), degree(0), mark(false) {}
  };

  node *min_root;
  usize total_nodes;

  void
  link(node *y, node *x) noexcept
  {
    y->left->right = y->right;
    y->right->left = y->left;

    y->parent = x;
    y->right = y->left = y;
    if ( !x->child )
      x->child = y;
    else {
      y->right = x->child;
      y->left = x->child->left;
      x->child->left->right = y;
      x->child->left = y;
    }
    x->degree++;
    y->mark = false;
  }

  void
  consolidate() noexcept
  {
    if ( !min_root )
      return;
    usize max_degree = 64;
    node *A[max_degree] = { nullptr };

    node *start = min_root;
    node *w = min_root;
    do {
      node *x = w;
      usize d = x->degree;
      while ( A[d] ) {
        node *y = A[d];
        if ( x->value < y->value )
          micron::swap(x, y);
        link(y, x);
        A[d] = nullptr;
        d++;
      }
      A[d] = x;
      w = w->right;
    } while ( w != start );

    min_root = nullptr;
    for ( usize i = 0; i < max_degree; i++ ) {
      if ( A[i] ) {
        if ( !min_root ) {
          min_root = A[i];
          min_root->left = min_root->right = min_root;
        } else {
          A[i]->left = min_root;
          A[i]->right = min_root->right;
          min_root->right->left = A[i];
          min_root->right = A[i];
          if ( A[i]->value > min_root->value )
            min_root = A[i];
        }
      }
    }
  }

  void
  cut(node *x, node *y) noexcept
  {
    if ( y->child == x )
      y->child = (x->right != x ? x->right : nullptr);
    x->left->right = x->right;
    x->right->left = x->left;
    y->degree--;

    x->left = min_root;
    x->right = min_root->right;
    min_root->right->left = x;
    min_root->right = x;
    x->parent = nullptr;
    x->mark = false;
  }

  void
  cascading_cut(node *y) noexcept
  {
    node *z = y->parent;
    if ( z ) {
      if ( !y->mark )
        y->mark = true;
      else {
        cut(y, z);
        cascading_cut(z);
      }
    }
  }

public:
  using category_type = theap_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  using value_type = T;
  using size_type = usize;
  using reference = T &;
  using const_reference = const T &;

  fibonacci_heap() : min_root(nullptr), total_nodes(0) {}

  ~fibonacci_heap() { clear(); }

  fibonacci_heap(const fibonacci_heap &o) = delete;

  fibonacci_heap(fibonacci_heap &&o) noexcept : min_root(o.min_root), total_nodes(o.total_nodes)
  {
    o.min_root = nullptr;
    o.total_nodes = 0;
  }

  fibonacci_heap &operator=(const fibonacci_heap &o) = delete;

  fibonacci_heap &
  operator=(fibonacci_heap &&o) noexcept
  {
    if ( this != &o ) {
      clear();
      min_root = o.min_root;
      total_nodes = o.total_nodes;
      o.min_root = nullptr;
      o.total_nodes = 0;
    }
    return *this;
  }

  void
  insert(T &&v)
  {
    node *n = new node(micron::move(v));
    if ( !min_root ) {
      min_root = n;
    } else {
      n->left = min_root;
      n->right = min_root->right;
      min_root->right->left = n;
      min_root->right = n;
      if ( n->value > min_root->value )
        min_root = n;
    }
    total_nodes++;
  }

  void
  insert(const T &v)
  {
    insert(T(v));
  }

  bool
  empty() const noexcept
  {
    return total_nodes == 0;
  }

  usize
  size() const noexcept
  {
    return total_nodes;
  }

  reference
  max()
  {
    if ( !min_root )
      exc<except::library_error>("fibonacci_heap::max() empty");
    return min_root->value;
  }

  T
  pop()
  {
    if ( !min_root )
      exc<except::library_error>("fibonacci_heap::pop() empty");
    node *z = min_root;
    if ( z->child ) {
      node *c = z->child;
      do {
        node *next = c->right;
        c->left = min_root;
        c->right = min_root->right;
        min_root->right->left = c;
        min_root->right = c;
        c->parent = nullptr;
        c = next;
      } while ( c != z->child );
    }

    z->left->right = z->right;
    z->right->left = z->left;

    if ( z == z->right )
      min_root = nullptr;
    else {
      min_root = z->right;
      consolidate();
    }

    T v = micron::move(z->value);
    delete z;
    total_nodes--;
    return v;
  }

  void
  clear() noexcept
  {
    if ( !min_root )
      return;
    node *start = min_root;
    do {
      node *n = start;
      start = start->right;
      delete_subtree(n);
    } while ( start != min_root );
    min_root = nullptr;
    total_nodes = 0;
  }

private:
  void
  delete_subtree(node *n) noexcept
  {
    if ( !n )
      return;
    if ( n->child ) {
      node *c = n->child;
      do {
        node *next = c->right;
        delete_subtree(c);
        c = next;
      } while ( c != n->child );
    }
    delete n;
  }
};
