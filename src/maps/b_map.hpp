//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../array.hpp"
#include "../memory/actions.hpp"
#include "../memory/new.hpp"
#include "../pointer.hpp"
#include "../tags.hpp"
#include "../types.hpp"
#include "../vector/fvector.hpp"
#include "hash/hash.hpp"

namespace micron
{

template <typename K, typename V, int Dg> struct btree_map_node {
  struct kv_pair {
    K key;
    V value;

    ~kv_pair() = default;
    kv_pair() : key(), value() {}
    kv_pair(const K &k, const V &v) : key(k), value(v) {}
    kv_pair(const K &k, V &&v) : key(k), value(micron::move(v)) {}
    kv_pair(K &&k, V &&v) : key(micron::move(k)), value(micron::move(v)) {}

    bool
    operator<(const kv_pair &other) const
    {
      return key < other.key;
    }
    bool
    operator<(const K &k) const
    {
      return key < k;
    }
    bool
    operator==(const K &k) const
    {
      return key == k;
    }
  };

  micron::array<kv_pair, 2 * Dg - 1> pairs;
  btree_map_node *chld[2 * Dg]{};
  int nkeys = 0;
  ~btree_map_node()
  {
    for ( int i = 0; i <= nkeys; ++i ) {
      if ( chld[i] ) {
        delete chld[i];
        chld[i] = nullptr;
      }
    }
  }
  inline btree_map_node() = default;

  inline explicit btree_map_node(const K &key, const V &value)
  {
    pairs[0] = kv_pair{ key, value };
    nkeys = 1;
  }

  inline explicit btree_map_node(const K &key, V &&value)
  {
    pairs[0] = kv_pair{ key, micron::move(value) };
    nkeys = 1;
  }

  inline int
  find_key(const K &key) const
  {
    int i = 0;
    while ( i < nkeys && pairs[i].key < key )
      ++i;
    return i;
  }

  inline void
  traverse(void (*visit)(const K &, V &))
  {
    for ( int i = 0; i < nkeys; ++i ) {
      if ( chld[i] )
        chld[i]->traverse(visit);
      visit(pairs[i].key, pairs[i].value);
    }
    if ( chld[nkeys] )
      chld[nkeys]->traverse(visit);
  }

  inline void
  traverse_const(void (*visit)(const K &, const V &)) const
  {
    for ( int i = 0; i < nkeys; ++i ) {
      if ( chld[i] )
        chld[i]->traverse_const(visit);
      visit(pairs[i].key, pairs[i].value);
    }
    if ( chld[nkeys] )
      chld[nkeys]->traverse_const(visit);
  }

  inline btree_map_node *
  search(const K &key, int *out_index = nullptr)
  {
    int i = find_key(key);
    if ( i < nkeys && pairs[i].key == key ) {
      if ( out_index )
        *out_index = i;
      return this;
    }
    return chld[i] ? chld[i]->search(key, out_index) : nullptr;
  }

  inline const btree_map_node *
  search(const K &key, int *out_index = nullptr) const
  {
    int i = find_key(key);
    if ( i < nkeys && pairs[i].key == key ) {
      if ( out_index )
        *out_index = i;
      return this;
    }
    return chld[i] ? chld[i]->search(key, out_index) : nullptr;
  }

  inline void
  split_child(int idx)
  {
    btree_map_node *y = chld[idx];
    btree_map_node *z = new btree_map_node();
    z->nkeys = Dg - 1;

    for ( int j = 0; j < Dg - 1; ++j )
      z->pairs[j] = micron::move(y->pairs[j + Dg]);

    if ( y->chld[0] ) {
      for ( int j = 0; j < Dg; ++j ) {
        z->chld[j] = y->chld[j + Dg];
        y->chld[j + Dg] = nullptr;
      }
    }

    y->nkeys = Dg - 1;

    for ( int j = nkeys; j >= idx + 1; --j )
      chld[j + 1] = chld[j];
    chld[idx + 1] = z;

    for ( int j = nkeys - 1; j >= idx; --j )
      pairs[j + 1] = micron::move(pairs[j]);

    pairs[idx] = micron::move(y->pairs[Dg - 1]);
    ++nkeys;
  }

  inline V *
  insert_nonfull(const K &key, V &&value, bool *inserted = nullptr)
  {
    int i = nkeys - 1;

    if ( !chld[0] ) {     // leaf
      while ( i >= 0 && key < pairs[i].key ) {
        pairs[i + 1] = micron::move(pairs[i]);
        --i;
      }

      if ( i >= 0 && pairs[i].key == key ) {
        pairs[i].value = micron::move(value);
        if ( inserted )
          *inserted = false;
        return &pairs[i].value;
      }

      pairs[i + 1] = kv_pair{ key, micron::move(value) };
      ++nkeys;
      if ( inserted )
        *inserted = true;
      return &pairs[i + 1].value;
    } else {     // internal
      while ( i >= 0 && key < pairs[i].key )
        --i;
      ++i;

      if ( i > 0 && pairs[i - 1].key == key ) {
        pairs[i - 1].value = micron::move(value);
        if ( inserted )
          *inserted = false;
        return &pairs[i - 1].value;
      }

      if ( chld[i]->nkeys == 2 * Dg - 1 ) {
        split_child(i);
        if ( key > pairs[i].key )
          ++i;
        else if ( key == pairs[i].key ) {
          pairs[i].value = micron::move(value);
          if ( inserted )
            *inserted = false;
          return &pairs[i].value;
        }
      }

      return chld[i]->insert_nonfull(key, micron::move(value), inserted);
    }
  }

  inline kv_pair
  get_pred(int idx)
  {
    btree_map_node *cur = chld[idx];
    while ( cur->chld[cur->nkeys] )
      cur = cur->chld[cur->nkeys];
    return cur->pairs[cur->nkeys - 1];
  }

  inline kv_pair
  get_succ(int idx)
  {
    btree_map_node *cur = chld[idx + 1];
    while ( cur->chld[0] )
      cur = cur->chld[0];
    return cur->pairs[0];
  }

  inline void
  fill(int idx)
  {
    if ( idx != 0 && chld[idx - 1]->nkeys >= Dg )
      borrow_from_prev(idx);
    else if ( idx != nkeys && chld[idx + 1]->nkeys >= Dg )
      borrow_from_next(idx);
    else if ( idx != nkeys )
      merge(idx);
    else
      merge(idx - 1);
  }

  inline void
  borrow_from_prev(int idx)
  {
    btree_map_node *child = chld[idx];
    btree_map_node *sibling = chld[idx - 1];

    for ( int i = child->nkeys - 1; i >= 0; --i )
      child->pairs[i + 1] = micron::move(child->pairs[i]);

    if ( child->chld[0] ) {
      for ( int i = child->nkeys; i >= 0; --i )
        child->chld[i + 1] = child->chld[i];
      child->chld[0] = sibling->chld[sibling->nkeys];
    }

    child->pairs[0] = micron::move(pairs[idx - 1]);
    pairs[idx - 1] = micron::move(sibling->pairs[sibling->nkeys - 1]);

    --sibling->nkeys;
    ++child->nkeys;
  }

  inline void
  borrow_from_next(int idx)
  {
    btree_map_node *child = chld[idx];
    btree_map_node *sibling = chld[idx + 1];

    child->pairs[child->nkeys] = micron::move(pairs[idx]);

    if ( child->chld[0] )
      child->chld[child->nkeys + 1] = sibling->chld[0];

    pairs[idx] = micron::move(sibling->pairs[0]);

    for ( int i = 1; i < sibling->nkeys; ++i )
      sibling->pairs[i - 1] = micron::move(sibling->pairs[i]);

    if ( sibling->chld[0] ) {
      for ( int i = 1; i <= sibling->nkeys; ++i )
        sibling->chld[i - 1] = sibling->chld[i];
    }

    --sibling->nkeys;
    ++child->nkeys;
  }

  inline void
  merge(int idx)
  {
    btree_map_node *child = chld[idx];
    btree_map_node *sibling = chld[idx + 1];

    child->pairs[Dg - 1] = micron::move(pairs[idx]);

    for ( int i = 0; i < sibling->nkeys; ++i )
      child->pairs[i + Dg] = micron::move(sibling->pairs[i]);

    if ( child->chld[0] ) {
      for ( int i = 0; i <= sibling->nkeys; ++i )
        child->chld[i + Dg] = sibling->chld[i];
    }

    for ( int i = idx + 1; i < nkeys; ++i )
      pairs[i - 1] = micron::move(pairs[i]);

    for ( int i = idx + 2; i <= nkeys; ++i )
      chld[i - 1] = chld[i];

    child->nkeys += sibling->nkeys + 1;
    --nkeys;

    delete sibling;
  }

  inline bool
  remove(const K &key)
  {
    int idx = find_key(key);

    if ( idx < nkeys && pairs[idx].key == key ) {
      if ( !chld[0] ) {     // leaf
        for ( int i = idx + 1; i < nkeys; ++i )
          pairs[i - 1] = micron::move(pairs[i]);
        --nkeys;
        return true;
      } else {     // internal
        if ( chld[idx]->nkeys >= Dg ) {
          kv_pair pred = get_pred(idx);
          pairs[idx] = micron::move(pred);
          return chld[idx]->remove(pred.key);
        } else if ( chld[idx + 1]->nkeys >= Dg ) {
          kv_pair succ = get_succ(idx);
          pairs[idx] = micron::move(succ);
          return chld[idx + 1]->remove(succ.key);
        } else {
          merge(idx);
          return chld[idx]->remove(key);
        }
      }
    } else if ( chld[0] ) {     // internal
      bool in_subtree = (idx == nkeys);

      if ( chld[idx]->nkeys < Dg )
        fill(idx);

      if ( in_subtree && idx > nkeys )
        return chld[idx - 1]->remove(key);
      else
        return chld[idx]->remove(key);
    }

    return false;     // ot found
  }
};

template <typename K, typename V, int Dg = 32>
  requires micron::is_move_constructible_v<V>
class btree_map
{
  using node_type = btree_map_node<K, V, Dg>;
  micron::uptr<node_type> root;
  size_t _size;

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using size_type = size_t;
  using key_type = K;
  using mapped_type = V;
  using value_type = typename node_type::kv_pair;

  ~btree_map() = default;

  btree_map() : root(nullptr), _size(0) {}

  btree_map(const btree_map &) = delete;

  btree_map(btree_map &&o) noexcept : root(micron::move(o.root)), _size(o._size) { o._size = 0; }

  btree_map &operator=(const btree_map &) = delete;

  btree_map &
  operator=(btree_map &&o) noexcept
  {
    if ( this != &o ) {
      root = micron::move(o.root);
      _size = o._size;
      o._size = 0;
    }
    return *this;
  }

  inline size_t
  size() const noexcept
  {
    return _size;
  }
  inline bool
  empty() const noexcept
  {
    return _size == 0;
  }
  inline size_t
  max_size() const noexcept
  {
    return size_t(-1);
  }

  inline void
  clear()
  {
    root.reset();
    _size = 0;
  }

  inline V &
  insert(const K &key, V &&value)
  {
    if ( !root ) {
      root = unique<node_type>(key, micron::move(value));
      _size = 1;
      return root->pairs[0].value;
    }

    if ( root->nkeys == 2 * Dg - 1 ) {
      auto new_root = unique<node_type>();
      new_root->chld[0] = root.release();
      new_root->split_child(0);
      root = micron::move(new_root);
    }

    bool inserted = false;
    V *result = root->insert_nonfull(key, micron::move(value), &inserted);
    if ( inserted ) {
      ++_size;
    }
    return *result;
  }

  inline V &
  insert(const K &key, const V &value)
  {
    V copy = value;
    return insert(key, micron::move(copy));
  }

  template <typename... Args>
  inline V &
  emplace(const K &key, Args &&...args)
  {
    V value(micron::forward<Args>(args)...);
    return insert(key, micron::move(value));
  }

  inline bool
  erase(const K &key)
  {
    if ( !root )
      return false;

    bool removed = root->remove(key);

    if ( removed ) {
      --_size;

      if ( root->nkeys == 0 ) {
        node_type *tmp = root.release();
        if ( tmp->chld[0] ) {
          root.reset(tmp->chld[0]);
          tmp->chld[0] = nullptr;
        }
        delete tmp;
      }
    }

    return removed;
  }

  inline void
  swap(btree_map &o) noexcept
  {
    micron::swap(root, o.root);
    micron::swap(_size, o._size);
  }

  inline V *
  find(const K &key)
  {
    if ( !root )
      return nullptr;

    int idx = -1;
    node_type *node = root->search(key, &idx);
    return (node && idx >= 0) ? &node->pairs[idx].value : nullptr;
  }

  inline const V *
  find(const K &key) const
  {
    if ( !root )
      return nullptr;

    int idx = -1;
    const node_type *node = root->search(key, &idx);
    return (node && idx >= 0) ? &node->pairs[idx].value : nullptr;
  }

  inline bool
  contains(const K &key) const
  {
    return find(key) != nullptr;
  }

  inline size_t
  count(const K &key) const
  {
    return contains(key) ? 1 : 0;
  }

  inline size_t
  exists(const K &key) const
  {
    return count(key);
  }

  inline V &
  at(const K &key)
  {
    V *val = find(key);
    if ( !val ) {
      exc<except::library_error>("btree_map::at: key not found");
    }
    return *val;
  }

  inline const V &
  at(const K &key) const
  {
    const V *val = find(key);
    if ( !val ) {
      exc<except::library_error>("btree_map::at: key not found");
    }
    return *val;
  }

  inline V &
  operator[](const K &key)
  {
    V *val = find(key);
    if ( val )
      return *val;

    return insert(key, V{});
  }

  inline V *
  min()
  {
    if ( !root )
      return nullptr;

    node_type *cur = root.get();
    while ( cur->chld[0] )
      cur = cur->chld[0];

    return cur->nkeys > 0 ? &cur->pairs[0].value : nullptr;
  }

  inline const V *
  min() const
  {
    if ( !root )
      return nullptr;

    const node_type *cur = root.get();
    while ( cur->chld[0] )
      cur = cur->chld[0];

    return cur->nkeys > 0 ? &cur->pairs[0].value : nullptr;
  }

  inline V *
  max()
  {
    if ( !root )
      return nullptr;

    node_type *cur = root.get();
    while ( cur->chld[cur->nkeys] )
      cur = cur->chld[cur->nkeys];

    return cur->nkeys > 0 ? &cur->pairs[cur->nkeys - 1].value : nullptr;
  }

  inline const V *
  max() const
  {
    if ( !root )
      return nullptr;

    const node_type *cur = root.get();
    while ( cur->chld[cur->nkeys] )
      cur = cur->chld[cur->nkeys];

    return cur->nkeys > 0 ? &cur->pairs[cur->nkeys - 1].value : nullptr;
  }

  inline V *
  lower_bound(const K &key)
  {
    if ( !root )
      return nullptr;

    node_type *cur = root.get();
    V *result = nullptr;

    while ( cur ) {
      int i = 0;
      while ( i < cur->nkeys && cur->pairs[i].key < key )
        ++i;

      if ( i < cur->nkeys ) {
        result = &cur->pairs[i].value;
        cur = cur->chld[i];
      } else {
        cur = cur->chld[i];
      }
    }

    return result;
  }

  inline const V *
  lower_bound(const K &key) const
  {
    return const_cast<btree_map *>(this)->lower_bound(key);
  }

  inline V *
  upper_bound(const K &key)
  {
    if ( !root )
      return nullptr;

    node_type *cur = root.get();
    V *result = nullptr;

    while ( cur ) {
      int i = 0;
      while ( i < cur->nkeys && cur->pairs[i].key <= key )
        ++i;

      if ( i < cur->nkeys ) {
        result = &cur->pairs[i].value;
        cur = cur->chld[i];
      } else {
        cur = cur->chld[i];
      }
    }

    return result;
  }

  inline const V *
  upper_bound(const K &key) const
  {
    return const_cast<btree_map *>(this)->upper_bound(key);
  }

  inline V *
  predecessor(const K &key)
  {
    if ( !root )
      return nullptr;

    int idx = -1;
    node_type *node = root->search(key, &idx);
    if ( !node || idx < 0 )
      return nullptr;

    if ( node->chld[idx] ) {
      node_type *cur = node->chld[idx];
      while ( cur->chld[cur->nkeys] )
        cur = cur->chld[cur->nkeys];
      return &cur->pairs[cur->nkeys - 1].value;
    }

    return (idx > 0) ? &node->pairs[idx - 1].value : nullptr;
  }

  inline const V *
  predecessor(const K &key) const
  {
    return const_cast<btree_map *>(this)->predecessor(key);
  }

  inline V *
  successor(const K &key)
  {
    if ( !root )
      return nullptr;

    int idx = -1;
    node_type *node = root->search(key, &idx);
    if ( !node || idx < 0 )
      return nullptr;

    if ( node->chld[idx + 1] ) {
      node_type *cur = node->chld[idx + 1];
      while ( cur->chld[0] )
        cur = cur->chld[0];
      return &cur->pairs[0].value;
    }

    return (idx < node->nkeys - 1) ? &node->pairs[idx + 1].value : nullptr;
  }

  inline const V *
  successor(const K &key) const
  {
    return const_cast<btree_map *>(this)->successor(key);
  }

  inline int
  height() const
  {
    int h = 0;
    const node_type *cur = root.get();
    while ( cur ) {
      ++h;
      cur = cur->chld[0];
    }
    return h;
  }

  inline void
  traverse(void (*visit)(const K &, V &))
  {
    if ( root )
      root->traverse(visit);
  }

  inline void
  traverse(void (*visit)(const K &, const V &)) const
  {
    if ( root )
      root->traverse_const(visit);
  }
};

}     // namespace micron
