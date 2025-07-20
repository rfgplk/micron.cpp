//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../pointer.hpp"
#include "../tags.hpp"
#include "../types.hpp"
#include "../vector/fvector.hpp"

namespace micron
{

template <typename T, int deg> struct b_node {
  micron::fvector<T> keys;
  micron::fvector<b_node> chld;
  b_node() : keys((2 * deg) - 1), chld() { }//keys = new T[2 * deg - 1]; }
  b_node(T&& key) : keys((2 * deg) - 1), chld() { 
    keys[0] = micron::move(key);
  }
  b_node(const b_node &) = delete;
  b_node(b_node &&o)
  {
    keys = micron::move(o.keys);
    chld = micron::move(o.chld);
  }
  b_node &operator=(const b_node &) = delete;
  b_node &
  operator=(b_node &&o)
  {
    keys = micron::move(o.keys);
    chld = micron::move(o.chld);
    return *this;
  }
  void
  append(T &&key)
  {
    int i = keys.size();

    if ( chld.empty() ) // if it's a leaf, insert as a sorted array
      keys.insert_sort(key);
    else // not a leaf, ie has kids
    {
      if(chld.size() == keys.size()) split( &chld.last() );
      chld.emplace_back(key);
    }
  }
  void split(b_node* nd) {
    b_node* 
  }
};

template <typename T, int deg> class b_tree
{
  micron::weak_pointer<b_node<T>> root_nd;

  b_tree() : root_nd(nullptr) {}
  void
  insert()
  {
  }
  void
  traverse()
  {
  }
};

};
