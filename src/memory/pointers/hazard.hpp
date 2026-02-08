//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../memory/actions.hpp"
#include "../../numerics.hpp"
#include "bits.hpp"

namespace micron
{

struct hazard_entry {
  atomic<void *> ptr = nullptr;
  atomic<bool> occupied = false;
};

constexpr static const size_t __max_hazard_threads = 256;
inline hazard_entry __hazard_table[__max_hazard_threads];

inline __attribute__((always_inline)) size_t
__emplace_hazard(void)
{
  for ( size_t i = 0; i < __max_hazard_threads; ++i ) {
    bool _f = false;
    if ( __hazard_table[i].occupied.compare_exchange_strong(_f, true, memory_order::relaxed) ) {
      __hazard_table[i].ptr.__store(0, memory_order::relaxed);
      return i;
    }
  }
  return numeric_limits<size_t>::max();
};

inline __attribute__((always_inline)) size_t
__pop_hazard(size_t _id) noexcept
{
  __hazard_table[_id].ptr.__store(0, memory_order::relaxed);
  __hazard_table[_id].occupied.__store(false, memory_order::relaxed);
}

class hazard_pointer
{
  using pointer_type = threaded_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = void;
  using value_type = void;
  static constexpr size_t _end = numeric_limits<size_t>::max();
  size_t _id = _end;

  template <typename T>
  inline __attribute__((always_inline)) void
  __update_seqcst(T *_ptr)
  {
    if ( _id == _end )
      _id = __emplace_hazard();
    if ( _id != _end ) [[likely]]
      __hazard_table[_id].ptr.__store(static_cast<void *>(_ptr), memory_order::seq_cst);
  }
  inline void
  __impl_delete(void) noexcept
  {
    if ( _id != _end ) {
      __pop_hazard(_id);
      _id = _end;
    }
  }

public:
  ~hazard_pointer() { __impl_delete(); }
  hazard_pointer(void) noexcept : _id(__emplace_hazard()) {}
  // all in one go
  template <typename T> hazard_pointer(const atomic<T *> &src) : _id(__emplace_hazard()) { protect(src); }
  hazard_pointer(hazard_pointer &&o) noexcept : _id(exchange(o._id, _end)) {}
  hazard_pointer &
  operator=(hazard_pointer &&o) noexcept
  {
    if ( this != &o ) {
      __impl_delete();
      _id = exchange(o._id, _end);
    }
    return *this;
  }

  bool
  empty() const noexcept
  {
    return (_id == _end) or (__hazard_table[_id].ptr.__get(memory_order::acquire) == nullptr);
  }
  template <class T>
  T *
  protect(const atomic<T *> &src) noexcept
  {
    for ( ;; ) {
      T *ptr = src.__get(memory_order::acquire);
      __update_seqcst(ptr);
      if ( ptr == src.load(memory_order::acquire) )
        return ptr;
    }
  }
  template <class T>
  bool
  try_protect(T *&ptr, const atomic<T *> &src) noexcept
  {
    T *p = src.__get(memory_order::acquire);
    __update_seqcst(p);
    ptr = p;
    bool ok = (p == src.__get(memory_order::acquire));
    if ( !ok ) {
      __hazard_table[_id].ptr.__store(0, memory_order::release);
    }
    return ok;
  }
  template <class T>
  void
  reset_protection(const T *ptr) noexcept
  {
    if ( _id == _end )
      return;
    T *expected = const_cast<T *>(ptr);
    __hazard_table[_id].ptr.compare_exchange_strong(expected, (bool)nullptr, memory_order::acq_rel, memory_order::acquire);
  }
  void
  reset_protection(nullptr_t = nullptr) noexcept
  {
    if ( _id == _end )
      return;
    __hazard_table[_id].ptr.__store(0, memory_order::release);
  }
};

template <typename Type, typename... Args>
hazard_pointer
make_hazard(Args &&...args)
{
  Type *raw_ptr = __internal_pointer_alloc<Type>::__impl_alloc(forward<Args>(args)...);
  atomic<Type *> a_ptr{ raw_ptr };
  return hazard_pointer(a_ptr);
}

};
