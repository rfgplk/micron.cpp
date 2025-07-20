//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
enum class RBColor : i32 { RED, BLACK };
template <typename T>
  requires std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>
class node
{

  enum class kind_t { empty, leaf, inner };

  T data;
  RBColor color;
  node *parent;
  node *left;
  node *right;
  kind_t kind;

public:
  static constexpr auto bits = B;
  static constexpr auto bits_leaf = BL;
  node() = delete;
  node(const T &dt) : data(dt), color(RED), parent(nullptr), left(nullptr), right(nullptr), kind(leaf) {}
  node(T &&dt) : data(micron::move(dt)), color(RED), parent(o.parent), left(o.left), right(o.right), kind(o.kind)
  {
    o.color = 0;
    o.parent = nullptr;
    o.left = nullptr;
    o.right = nullptr;
    o.kind = empty;
  }
  node(const node &o) : data(o.data), color(o.color), parent(o.parent), left(o.left), right(o.right), kind(o.kind) {}
  node(node &&o)
      : data(micron::move(o.data)), color(o.color), parent(o.parent), left(o.left), right(o.right), kind(o.kind)
  {
    o.color = 0;
    o.parent = nullptr;
    o.left = nullptr;
    o.right = nullptr;
    o.kind = empty;
  }
  node &
  operator=(const node &o)
  {
    data = o.data;
    color = o.color;
    parent = o.parent;
    left = o.left;
    right = o.right;
    kind = o.kind;
    return *this;
  }
  node &
  operator=(node &&o)
  {
    data = micron::move(o.data);
    parent = o.parent;
    color = o.color;
    left = o.left;
    right = o.right;
    kind = o.kind;
    o.parent = nullptr;
    o.color = 0;
    o.left = nullptr;
    o.right = nullptr;
    o.kind = empty;
    return *this;
  }
  ~node()
  {
    data.~T();
    color = 0;
    parent = nullptr;
    left = nullptr;
    right = nullptr;
  }

  bool
  is_leaf() const
  {
    return kind == leaf;
  }     // no children
  bool
  is_inner() const
  {
    return kind == inner;
  }     // has a child
};
template <typename T>
  requires std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>
class rb
{
  using node_t = node<T>;
  node_t *root;
  node_t *tail;
  rb() = delete;
  rb(const rb &) = delete;
  rb(rb &&o) : root(o.root), tail(o.tail)
  {
    o.root = nullptr;
    o.tail = nullptr;
  }
  rb &operator=(const rb &) = delete;
  rb &
  operator=(rb &&o)
  {
    root = o.root;
    tail = o.tail;
    o.root = nullptr;
    o.tail = nullptr;
    return *this;
  };
  ~rb()
  {
    root = nullptr;
    tail = nullptr;
  }
};

};     // namespace micron
