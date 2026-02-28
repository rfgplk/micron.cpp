//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../memory/actions.hpp"
#include "../memory/allocation/chunks.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"

#include "../mutex/locks.hpp"
#include "../vector/fvector.hpp"

namespace micron
{

template <typename T> class heapq
{
  micron::mutex __mtx;
  micron::fvector<T> heap;

  void
  sift_up(usize idx)
  {
    while ( idx > 0 ) {
      usize parent = (idx - 1) / 2;
      if ( heap[idx] < heap[parent] ) {
        micron::swap(heap[idx], heap[parent]);
        idx = parent;
      } else {
        break;
      }
    }
  }

  void
  sift_down(usize idx)
  {
    usize n = heap.size();
    while ( true ) {
      usize left = 2 * idx + 1;
      usize right = 2 * idx + 2;
      usize smallest = idx;

      if ( left < n && heap[left] < heap[smallest] )
        smallest = left;
      if ( right < n && heap[right] < heap[smallest] )
        smallest = right;

      if ( smallest != idx ) {
        micron::swap(heap[idx], heap[smallest]);
        idx = smallest;
      } else {
        break;
      }
    }
  }

public:
  heapq() = default;

  heapq(const heapq &o) : heap(o) {}

  heapq(heapq &&o) : heap(micron::move(o)) {}

  heapq &
  operator=(const heapq &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(o.__mtx);
    heap = o;
    return *this;
  }

  heapq &
  operator=(heapq &&o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(o.__mtx);
    heap = micron::move(o.heap);
    return *this;
  }

  explicit heapq(const micron::fvector<T> &v) : heap(v)
  {
    for ( int i = static_cast<int>(heap.size() / 2) - 1; i >= 0; --i )
      sift_down(i);
  }

  void
  push(const T &val)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    heap.push_back(val);
    sift_up(heap.size() - 1);
  }

  T
  pop()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( heap.empty() )
      exc<except::library_error>("micron::heapq::top() is empty");

    T min_val = heap[0];
    heap[0] = heap.back();
    heap.pop_back();
    if ( !heap.empty() )
      sift_down(0);
    return min_val;
  }

  const T &
  top() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( heap.empty() )
      exc<except::library_error>("micron::heapq::top() is empty");
    return heap[0];
  }

  bool
  empty() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return heap.empty();
  }

  usize
  size() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return heap.size();
  }

  const micron::fvector<T> &
  data() const
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    return heap;
  }
};

};     // namespace micron
