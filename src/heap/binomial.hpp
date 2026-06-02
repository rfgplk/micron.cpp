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

template<typename T> struct __binomial_less {
  constexpr bool
  operator()(const T &a, const T &b) const
  {
    return micron::less<T>(a, b);
  }
};

template<typename T, typename... Args> struct __bn_is_self_arg {
  static constexpr bool value = false;
};

template<typename T, typename U> struct __bn_is_self_arg<T, U> {
  static constexpr bool value = is_same_v<__remove_cvref_t<U>, T>;
};

template<typename T, typename C> struct __binomial_node {
  T value;
  usize degree;
  __binomial_node *parent;
  __binomial_node *child;
  __binomial_node *sibling;

  __binomial_node(const T &val) : value(val), degree(0), parent(nullptr), child(nullptr), sibling(nullptr) { }

  __binomial_node(T &&val) : value(micron::move(val)), degree(0), parent(nullptr), child(nullptr), sibling(nullptr) { }

  template<typename... Args, typename = __enable_if_t<!__bn_is_self_arg<T, Args...>::value>>
  __binomial_node(Args &&...args) : value(micron::forward<Args>(args)...), degree(0), parent(nullptr), child(nullptr), sibling(nullptr)
  {
  }
};

template<typename T, typename C = micron::__binomial_less<T>> struct __binomial_heap {
  using node_type = __binomial_node<T, C>;

  node_type *root_list;
  node_type *min_node;
  usize node_count;
  C comp;
  node_type *__pool;

  ~__binomial_heap()
  {
    clear();
    __drain_pool();
  }

  __binomial_heap() : root_list(nullptr), min_node(nullptr), node_count(0), comp(), __pool(nullptr) { }

  __binomial_heap(const C &c) : root_list(nullptr), min_node(nullptr), node_count(0), comp(c), __pool(nullptr) { }

  __binomial_heap(const __binomial_heap &) = delete;
  __binomial_heap &operator=(const __binomial_heap &) = delete;

  __binomial_heap(__binomial_heap &&o) noexcept
      : root_list(o.root_list), min_node(o.min_node), node_count(o.node_count), comp(micron::move(o.comp)), __pool(o.__pool)
  {
    o.root_list = nullptr;
    o.min_node = nullptr;
    o.node_count = 0;
    o.__pool = nullptr;
  }

  __binomial_heap &
  operator=(__binomial_heap &&o) noexcept
  {
    if ( this != &o ) {
      clear();
      __drain_pool();      // release our own recycled storage before adopting o's
      root_list = o.root_list;
      min_node = o.min_node;
      node_count = o.node_count;
      comp = micron::move(o.comp);
      __pool = o.__pool;
      o.root_list = nullptr;
      o.min_node = nullptr;
      o.node_count = 0;
      o.__pool = nullptr;
    }
    return *this;
  }

  template<typename... A>
  [[gnu::always_inline]] inline node_type *
  __acquire(A &&...a)
  {
    if ( __pool ) {
      node_type *slot = __pool;
      __pool = *reinterpret_cast<node_type **>(slot);
      return new (static_cast<void *>(slot)) node_type(micron::forward<A>(a)...);
    }
    return new node_type(micron::forward<A>(a)...);
  }

  [[gnu::always_inline]] inline void
  __recycle(node_type *n) noexcept
  {
    n->~node_type();
    *reinterpret_cast<node_type **>(n) = __pool;
    __pool = n;
  }

  inline void
  __drain_pool() noexcept
  {
    while ( __pool ) {
      node_type *nxt = *reinterpret_cast<node_type **>(__pool);
      ::operator delete(static_cast<void *>(__pool));
      __pool = nxt;
    }
  }

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
    if ( !h1 ) return h2;
    if ( !h2 ) return h1;

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
    if ( !other.root_list ) return;

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
      if ( comp(curr->value, min_node->value) ) min_node = curr;
      curr = curr->sibling;
    }
  }

  inline node_type *
  insert(const T &value)
  {
    node_type *new_node = __acquire(value);
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
    node_type *new_node = __acquire(micron::move(value));
    __binomial_heap temp(comp);
    temp.root_list = new_node;
    temp.min_node = new_node;
    temp.node_count = 1;

    union_heap(temp);
    return new_node;
  }

  template<typename... Args>
  inline node_type *
  emplace(Args &&...args)
  {
    node_type *new_node = __acquire(micron::forward<Args>(args)...);
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
    if ( !min_node ) exc<except::library_error>("micron::__binomial_heap::find_min() is empty");
    return min_node->value;
  }

  inline T
  extract_min()
  {
    if ( !min_node ) exc<except::library_error>("micron::__binomial_heap::extract_min() is empty");

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

    __recycle(min_node);
    node_count--;

    union_heap(child_heap);

    if ( !reversed ) update_min();

    return min_value;
  }

  inline void
  decrease_key(node_type *node, const T &new_value)
  {
    if ( !comp(new_value, node->value) ) return;

    node->value = new_value;

    node_type *curr = node;
    node_type *parent = curr->parent;

    while ( parent && comp(curr->value, parent->value) ) {
      micron::swap(curr->value, parent->value);
      curr = parent;
      parent = curr->parent;
    }

    if ( !min_node || comp(curr->value, min_node->value) ) min_node = curr;
  }

  inline void
  delete_node(node_type *node)
  {
    if ( !node ) return;

    node_type *curr = node;
    node_type *parent = curr->parent;

    while ( parent ) {
      micron::swap(curr->value, parent->value);
      curr = parent;
      parent = curr->parent;
    }

    min_node = curr;
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
    if ( !root ) return nullptr;

    if ( root->value == value ) return root;

    node_type *result = find_recursive(root->child, value);
    if ( result ) return result;

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
    if ( !node ) return;

    clear_recursive(node->child);
    clear_recursive(node->sibling);
    __recycle(node);
  }

  inline bool
  empty() const
  {
    return node_count == 0;
  }

  inline usize
  size() const
  {
    return node_count;
  }
};

};      // namespace micron
