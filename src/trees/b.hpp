//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../array.hpp"
#include "../memory/new.hpp"
#include "../pointer.hpp"
#include "../tags.hpp"
#include "../types.hpp"
#include "../vector/fvector.hpp"

// TODO: redo

namespace micron
{

template <typename T, int Dg> struct b_node {
  micron::array<T, 2 * Dg - 1> keys;
  b_node *chld[2 * Dg]{};
  int nkeys = 0;

  inline b_node() = default;

  inline explicit b_node(const T &key)
  {
    keys[0] = key;
    nkeys = 1;
  }

  inline int
  find_key(const T &key) const
  {
    int i = 0;
    while ( i < nkeys && keys[i] < key )
      ++i;
    return i;
  }

  inline void
  traverse(void (*visit)(const T &)) const
  {
    for ( int i = 0; i < nkeys; ++i ) {
      if ( chld[i] )
        chld[i]->traverse(visit);
      visit(keys[i]);
    }
    if ( chld[nkeys] )
      chld[nkeys]->traverse(visit);
  }

  inline b_node *
  search(const T &key, int *out_index = nullptr)
  {
    int i = find_key(key);
    if ( i < nkeys && keys[i] == key ) {
      if ( out_index )
        *out_index = i;
      return this;
    }
    return chld[i] ? chld[i]->search(key, out_index) : nullptr;
  }

  inline void
  split_child(int _k)
  {
    b_node *y = chld[_k];
    b_node *z = new b_node();
    z->nkeys = Dg - 1;

    for ( int j = 0; j < Dg - 1; ++j )
      z->keys[j] = micron::move(y->keys[j + Dg]);
    for ( int j = 0; j < Dg; ++j ) {
      z->chld[j] = y->chld[j + Dg];
      y->chld[j + Dg] = nullptr;
    }

    y->nkeys = Dg - 1;

    for ( int j = nkeys; j >= _k + 1; --j )
      chld[j + 1] = chld[j];
    chld[_k + 1] = z;

    for ( int j = nkeys - 1; j >= _k; --j )
      keys[j + 1] = micron::move(keys[j]);
    keys[_k] = micron::move(y->keys[Dg - 1]);
    ++nkeys;
  }

  inline void
  insert_nonfull(const T &key)
  {
    int i = nkeys - 1;
    if ( !chld[0] ) {     // leaf
      while ( i >= 0 && key < keys[i] ) {
        keys[i + 1] = keys[i];
        --i;
      }
      keys[i + 1] = key;
      ++nkeys;
    } else {
      while ( i >= 0 && key < keys[i] )
        --i;
      ++i;
      if ( chld[i]->nkeys == 2 * Dg - 1 ) {
        split_child(i);
        if ( key > keys[i] )
          ++i;
      }
      chld[i]->insert_nonfull(key);
    }
  }

  inline T
  get_pred(int _k)
  {
    b_node *cur = chld[_k];
    while ( cur->chld[0] )
      cur = cur->chld[cur->nkeys];
    return cur->keys[cur->nkeys - 1];
  }

  inline T
  get_succ(int _k)
  {
    b_node *cur = chld[_k + 1];
    while ( cur->chld[0] )
      cur = cur->chld[0];
    return cur->keys[0];
  }

  inline void
  fill(int _k)
  {
    if ( _k != 0 && chld[_k - 1]->nkeys >= Dg )
      borrow_from_prev(_k);
    else if ( _k != nkeys && chld[_k + 1]->nkeys >= Dg )
      borrow_from_next(_k);
    else if ( _k != nkeys )
      merge(_k);
    else
      merge(_k - 1);
  }

  inline void
  borrow_from_prev(int _k)
  {
    b_node *child = chld[_k];
    b_node *sibling = chld[_k - 1];

    for ( int i = child->nkeys - 1; i >= 0; --i )
      child->keys[i + 1] = micron::move(child->keys[i]);
    if ( child->chld[0] ) {
      for ( int i = child->nkeys; i >= 0; --i )
        child->chld[i + 1] = child->chld[i];
      child->chld[0] = sibling->chld[sibling->nkeys];
    }

    child->keys[0] = micron::move(keys[_k - 1]);
    keys[_k - 1] = micron::move(sibling->keys[sibling->nkeys - 1]);
    --sibling->nkeys;
    ++child->nkeys;
  }

  inline void
  borrow_from_next(int _k)
  {
    b_node *child = chld[_k];
    b_node *sibling = chld[_k + 1];

    child->keys[child->nkeys] = micron::move(keys[_k]);
    if ( child->chld[0] )
      child->chld[child->nkeys + 1] = sibling->chld[0];

    keys[_k] = micron::move(sibling->keys[0]);

    for ( int i = 1; i < sibling->nkeys; ++i )
      sibling->keys[i - 1] = micron::move(sibling->keys[i]);
    if ( sibling->chld[0] ) {
      for ( int i = 1; i <= sibling->nkeys; ++i )
        sibling->chld[i - 1] = sibling->chld[i];
    }

    --sibling->nkeys;
    ++child->nkeys;
  }

  inline void
  merge(int _k)
  {
    b_node *child = chld[_k];
    b_node *sibling = chld[_k + 1];

    child->keys[Dg - 1] = micron::move(keys[_k]);
    for ( int i = 0; i < sibling->nkeys; ++i )
      child->keys[i + Dg] = micron::move(sibling->keys[i]);
    for ( int i = 0; i <= sibling->nkeys; ++i )
      child->chld[i + Dg] = sibling->chld[i];

    for ( int i = _k + 1; i < nkeys; ++i )
      keys[i - 1] = micron::move(keys[i]);
    for ( int i = _k + 2; i <= nkeys; ++i )
      chld[i - 1] = chld[i];

    child->nkeys += sibling->nkeys + 1;
    --nkeys;
    delete sibling;
  }

  inline void
  remove(const T &key)
  {
    int _k = find_key(key);

    if ( _k < nkeys && keys[_k] == key ) {
      if ( !chld[0] ) {
        for ( int i = _k + 1; i < nkeys; ++i )
          keys[i - 1] = micron::move(keys[i]);
        --nkeys;
      } else {
        if ( chld[_k]->nkeys >= Dg ) {
          keys[_k] = micron::move(get_pred(_k));
          chld[_k]->remove(keys[_k]);
        } else if ( chld[_k + 1]->nkeys >= Dg ) {
          keys[_k] = micron::move(get_succ(_k));
          chld[_k + 1]->remove(keys[_k]);
        } else {
          merge(_k);
          chld[_k]->remove(key);
        }
      }
    } else if ( chld[0] ) {
      bool flag = (_k == nkeys);
      if ( chld[_k]->nkeys < Dg )
        fill(_k);
      if ( flag && _k > nkeys )
        chld[_k - 1]->remove(key);
      else
        chld[_k]->remove(key);
    }
  }
};

template <typename T, int Dg> struct b_tree {
  micron::uptr<b_node<T, Dg>> root;

  ~b_tree() = default;
  b_tree() : root(unique<b_node<T, Dg>>()) {}
  b_tree(const b_tree &) = delete;
  b_tree(b_tree &&o) : root(micron::move(o.root)) {}
  b_tree &operator=(const b_tree &) = delete;
  b_tree &
  operator=(b_tree &&o)
  {
    if ( this != &o ) {
      root = micron::move(o.root);
    }
    return *this;
  }
  inline void
  insert(const T &key)
  {
    if ( !root ) {
      root = unique<b_node<T, Dg>>(key);     // new b_node<T, Dg>(key);
      return;
    }
    if ( root->nkeys == 2 * Dg - 1 ) {
      auto s = unique<b_node<T, Dg>>();
      ;
      s->chld[0] = root.release();
      s->split_child(0);
      root = micron::move(s);
    }
    root->insert_nonfull(key);
  }

  inline bool
  search(const T &key)
  {
    return root.get() && root->search(key);
  }

  inline void
  remove(const T &key)
  {
    if ( !root )
      return;
    root->remove(key);
    if ( root->nkeys == 0 ) {
      b_node<T, Dg> *tmp = root.release();
      root = root->chld[0];
      tmp->chld[0] = nullptr;
      delete tmp;
    }
  }

  inline void
  traverse(void (*visit)(const T &)) const
  {
    if ( root.get() )
      root->traverse(visit);
  }

  inline T *
  get(const T &key)
  {
    if ( !root )
      return nullptr;
    int idx = -1;
    b_node<T, Dg> *n = root->search(key, &idx);
    return (n && idx >= 0) ? &n->keys[idx] : nullptr;
  }

  inline micron::array<b_node<T, Dg> *, 2>
  get_near_children(const T &key)
  {
    if ( !root )
      return { nullptr, nullptr };
    int idx = -1;
    b_node<T, Dg> *n = root->search(key, &idx);
    if ( !n || idx < 0 )
      return { nullptr, nullptr };
    return { n->chld[idx], n->chld[idx + 1] };
  }
  inline int
  size() const
  {
    int count = 0;
    if ( root ) {
      root->traverse([&](const T &) { ++count; });
    }
    return count;
  }
  inline T *
  min() const
  {
    if ( !root )
      return nullptr;
    b_node<T, Dg> *cur = root.get();
    while ( cur->chld[0] )
      cur = cur->chld[0];
    return &cur->keys[0];
  }

  inline T *
  max() const
  {
    if ( !root )
      return nullptr;
    b_node<T, Dg> *cur = root.get();
    while ( cur->chld[cur->nkeys] )
      cur = cur->chld[cur->nkeys];
    return &cur->keys[cur->nkeys - 1];
  }
  inline T *
  lower_bound(const T &key)
  {
    b_node<T, Dg> *cur = root.get();
    while ( cur ) {
      int i = 0;
      while ( i < cur->nkeys && cur->keys[i] < key )
        ++i;
      if ( i < cur->nkeys && cur->keys[i] >= key )
        return &cur->keys[i];
      cur = cur->chld[i];
    }
    return nullptr;
  }

  inline T *
  upper_bound(const T &key)
  {
    b_node<T, Dg> *cur = root.get();
    while ( cur ) {
      int i = 0;
      while ( i < cur->nkeys && cur->keys[i] <= key )
        ++i;
      if ( i < cur->nkeys )
        return &cur->keys[i];
      cur = cur->chld[i];
    }
    return nullptr;
  }
  inline T *
  predecessor(const T &key)
  {
    int idx = -1;
    b_node<T, Dg> *n = root->search(key, &idx);
    if ( !n || idx < 0 )
      return nullptr;
    if ( n->chld[idx] ) {     // left subtree exists
      b_node<T, Dg> *cur = n->chld[idx];
      while ( cur->chld[cur->nkeys] )
        cur = cur->chld[cur->nkeys];
      return &cur->keys[cur->nkeys - 1];
    }
    return (idx > 0) ? &n->keys[idx - 1] : nullptr;
  }

  inline T *
  successor(const T &key)
  {
    int idx = -1;
    b_node<T, Dg> *n = root->search(key, &idx);
    if ( !n || idx < 0 )
      return nullptr;
    if ( n->chld[idx + 1] ) {
      b_node<T, Dg> *cur = n->chld[idx + 1];
      while ( cur->chld[0] )
        cur = cur->chld[0];
      return &cur->keys[0];
    }
    return (idx < n->nkeys - 1) ? &n->keys[idx + 1] : nullptr;
  }
  inline int
  height() const
  {
    int h = 0;
    b_node<T, Dg> *cur = root.get();
    while ( cur ) {
      ++h;
      cur = cur->chld[0];
    }
    return h;
  }
  inline micron::fvector<b_node<T, Dg> *>
  get_children(const T &key)
  {
    micron::fvector<b_node<T, Dg> *> result;
    if ( !root )
      return result;
    int idx = -1;
    b_node<T, Dg> *n = root->search(key, &idx);
    if ( !n || idx < 0 )
      return result;

    for ( int i = 0; i <= n->nkeys; ++i ) {
      if ( n->chld[i] )
        result.emplace_back(n->chld[i]);
    }
    return result;
  }
};

};
