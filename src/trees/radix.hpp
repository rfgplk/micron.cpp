//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../array.hpp"
#include "../vector/fvector.hpp"
#include "../memory/new.hpp"
#include "../pointer.hpp"
#include "../tags.hpp"
#include "../types.hpp"

template <typename Key, typename Value>
  requires micron::is_copy_constructible_v<Key> && micron::is_move_constructible_v<Key>
           && micron::is_copy_constructible_v<Value> && micron::is_move_constructible_v<Value>
class radix_node
{
  struct node {
    Key key_fragment;
    Value value;
    bool is_leaf;
    mc::fvector<node *> children;

    node() = delete;
    node(const Key &k, const Value &v) : key_fragment(k), value(v), is_leaf(true) {}
    node(Key &&k, Value &&v) noexcept : key_fragment(micron::move(k)), value(micron::move(v)), is_leaf(true) {}
    node(const node &o) = delete;
    node(node &&o) noexcept
        : key_fragment(micron::move(o.key_fragment)), value(micron::move(o.value)), is_leaf(o.is_leaf),
          children(micron::move(o.children))
    {
      o.is_leaf = false;
      o.children.clear();
    }
    node &operator=(const node &o) = delete;
    node &
    operator=(node &&o) noexcept
    {
      if ( this != &o ) {
        key_fragment = micron::move(o.key_fragment);
        value = micron::move(o.value);
        is_leaf = o.is_leaf;
        children = micron::move(o.children);
        o.is_leaf = false;
        o.children.clear();
      }
      return *this;
    }

    ~node()
    {
      for ( auto c : children )
        delete c;
    }
  };

  node *root;

  static size_t
  common_prefix_length(const Key &a, const Key &b)
  {
    size_t i = 0;
    while ( i < a.size() && i < b.size() && a[i] == b[i] )
      i++;
    return i;
  }

public:
  radix_node() : root(nullptr) {}
  ~radix_node() { clear(); }

  radix_node(const radix_node &o) = delete;
  radix_node(radix_node &&o) noexcept : root(o.root) { o.root = nullptr; }
  radix_node &operator=(const radix_node &o) = delete;
  radix_node &
  operator=(radix_node &&o) noexcept
  {
    if ( this != &o ) {
      clear();
      root = o.root;
      o.root = nullptr;
    }
    return *this;
  }

  void
  insert(Key &&k, Value &&v)
  {
    if ( !root ) {
      root = new node(micron::move(k), micron::move(v));
      return;
    }
    node *cur = root;
    while ( true ) {
      size_t prefix = common_prefix_length(cur->key_fragment, k);
      if ( prefix == cur->key_fragment.size() ) {
        if ( prefix == k.size() ) {
          // exact match
          cur->value = micron::move(v);
          cur->is_leaf = true;
          return;
        }
        Key remaining = k.substr(prefix);
        for ( auto c : cur->children ) {
          if ( c->key_fragment[0] == remaining[0] ) {
            k = remaining;
            cur = c;
            goto next_iter;
          }
        }
        cur->children.push_back(new node(micron::move(remaining), micron::move(v)));
        cur->is_leaf = false;
        return;
      } else {
        // split node
        Key old_suffix = cur->key_fragment.substr(prefix);
        node *split_node = new node(micron::move(old_suffix), micron::move(cur->value));
        split_node->children = micron::move(cur->children);
        split_node->is_leaf = cur->is_leaf;

        cur->key_fragment = cur->key_fragment.substr(0, prefix);
        cur->children.clear();
        cur->children.push_back(split_node);
        cur->is_leaf = false;

        if ( prefix < k.size() ) {
          Key new_suffix = k.substr(prefix);
          cur->children.push_back(new node(micron::move(new_suffix), micron::move(v)));
        } else {
          cur->value = micron::move(v);
          cur->is_leaf = true;
        }
        return;
      }
    next_iter:;
    }
  }

  void
  insert(const Key &k, const Value &v)
  {
    insert(Key(k), Value(v));
  }

  Value *
  find(const Key &k)
  {
    node *cur = root;
    size_t pos = 0;
    while ( cur ) {
      size_t prefix = common_prefix_length(cur->key_fragment, k.substr(pos));
      if ( prefix == cur->key_fragment.size() ) {
        pos += prefix;
        if ( pos == k.size() )
          return cur->is_leaf ? &cur->value : nullptr;
        bool found = false;
        for ( auto c : cur->children ) {
          if ( c->key_fragment[0] == k[pos] ) {
            cur = c;
            found = true;
            break;
          }
        }
        if ( !found )
          return nullptr;
      } else
        return nullptr;
    }
    return nullptr;
  }

  void
  clear()
  {
    delete root;
    root = nullptr;
  }
};
