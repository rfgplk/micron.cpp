//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../memory/actions.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"

#include "../mutex/locks.hpp"
#include "../vector/fvector.hpp"

namespace micron
{

template<typename T> class heapq
{
  mutable micron::fast_mutex __mtx;
  micron::fvector<T> heap;

  struct __hold {
    micron::fast_mutex &m;

    [[gnu::always_inline]] explicit __hold(micron::fast_mutex &mm) noexcept : m(mm) { m.lock(); }

    [[gnu::always_inline]] ~__hold() noexcept { m.unlock(); }

    __hold(const __hold &) = delete;
    __hold &operator=(const __hold &) = delete;
  };

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

      if ( left < n && heap[left] < heap[smallest] ) smallest = left;
      if ( right < n && heap[right] < heap[smallest] ) smallest = right;

      if ( smallest != idx ) {
        micron::swap(heap[idx], heap[smallest]);
        idx = smallest;
      } else {
        break;
      }
    }
  }

public:
  typedef T value_type;
  typedef usize size_type;
  typedef T &reference;
  typedef const T &const_reference;

  heapq() = default;

  heapq(const heapq &o)
  {
    __hold __lock(o.__mtx);
    heap.reserve(o.heap.size());
    for ( usize i = 0; i < o.heap.size(); ++i ) heap.push_back(o.heap[i]);
  }

  heapq(heapq &&o) : heap(micron::move(o.heap)) { }

  heapq &
  operator=(const heapq &o)
  {
    if ( this == &o ) return *this;
    // lock both mutexes in a fixed (address) order to avoid deadlock
    const bool __self_first = micron::addressof(__mtx) < micron::addressof(o.__mtx);
    micron::unique_lock<micron::lock_starts::defer, micron::fast_mutex> __l0(__self_first ? __mtx : o.__mtx);
    micron::unique_lock<micron::lock_starts::defer, micron::fast_mutex> __l1(__self_first ? o.__mtx : __mtx);
    __l0.lock();
    __l1.lock();
    heap.clear();
    heap.reserve(o.heap.size());
    for ( usize i = 0; i < o.heap.size(); ++i ) heap.push_back(o.heap[i]);
    return *this;
  }

  heapq &
  operator=(heapq &&o)
  {
    if ( this == &o ) return *this;
    // lock both mutexes in a fixed (address) order to avoid deadlock
    const bool __self_first = micron::addressof(__mtx) < micron::addressof(o.__mtx);
    micron::unique_lock<micron::lock_starts::defer, micron::fast_mutex> __l0(__self_first ? __mtx : o.__mtx);
    micron::unique_lock<micron::lock_starts::defer, micron::fast_mutex> __l1(__self_first ? o.__mtx : __mtx);
    __l0.lock();
    __l1.lock();
    heap = micron::move(o.heap);
    return *this;
  }

  explicit heapq(const micron::fvector<T> &v)
  {
    heap.reserve(v.size());
    for ( usize i = 0; i < v.size(); ++i ) heap.push_back(v[i]);
    for ( usize i = heap.size() / 2; i-- > 0; ) sift_down(i);
  }

  void
  push(const T &val)
  {
    __hold __lock(__mtx);
    heap.push_back(val);
    sift_up(heap.size() - 1);
  }

  T
  pop()
  {
    __hold __lock(__mtx);
    if ( heap.empty() ) exc<except::library_error>("micron::heapq::pop() is empty");

    T min_val = heap[0];
    heap[0] = heap.back();
    heap.pop_back();
    if ( !heap.empty() ) sift_down(0);
    return min_val;
  }

  // WARNING: the returned reference is NOT protected after this call
  const T &
  top() const
  {
    __hold __lock(__mtx);
    if ( heap.empty() ) exc<except::library_error>("micron::heapq::top() is empty");
    return heap[0];
  }

  bool
  empty() const
  {
    __hold __lock(__mtx);
    return heap.empty();
  }

  usize
  size() const
  {
    __hold __lock(__mtx);
    return heap.size();
  }

  // WARNING: the returned reference is NOT protected after this call
  const micron::fvector<T> &
  data() const
  {
    __hold __lock(__mtx);
    return heap;
  }

  // visits in heap array order, not sorted order
  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    __hold __lock(__mtx);
    for ( usize i = 0; i < heap.size(); ++i ) fn(heap[i]);
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    __hold __lock(__mtx);
    for ( usize i = 0; i < heap.size(); ++i ) fn(static_cast<const T &>(heap[i]));
  }
};

};      // namespace micron
