//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "atomic.hpp"
#include "bits.hpp"

namespace micron
{

// atomic_pointer
// thread safe unique pointer
// equivalent of unique_pointer but with atomic guarantees via atomic<>
template <class Type> class atomic_pointer : private __internal_pointer_alloc<Type>
{
  atomic_token<Type *> internal_pointer;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;
  using element_type = Type;

  using __alloc = __internal_pointer_alloc<Type>;

  ~atomic_pointer() noexcept { __alloc::__impl_dealloc(internal_pointer.swap(nullptr)); }

  atomic_pointer(void) noexcept : internal_pointer(nullptr) {}

  template <typename V>
    requires micron::is_null_pointer_v<V>
  atomic_pointer(V) noexcept : internal_pointer(nullptr)
  {
  }

  atomic_pointer(Type *&&raw_ptr) noexcept : internal_pointer(raw_ptr) { raw_ptr = nullptr; }

  template <class... Args>
    requires(sizeof...(Args) > 0)
  atomic_pointer(Args &&...args) : internal_pointer(__alloc::__impl_alloc(micron::forward<Args>(args)...))
  {
  }

  atomic_pointer(atomic_pointer &&o) noexcept : internal_pointer(o.internal_pointer.swap(nullptr)) {}

  atomic_pointer(const atomic_pointer &) = delete;
  atomic_pointer &operator=(const atomic_pointer &) = delete;

  atomic_pointer &
  operator=(atomic_pointer &&o) noexcept
  {
    if ( this != &o ) {
      Type *incoming = o.internal_pointer.swap(nullptr);
      Type *old = internal_pointer.swap(incoming);
      __alloc::__impl_dealloc(old);
    }
    return *this;
  }

  atomic_pointer &
  operator=(Type *&&raw_ptr) noexcept
  {
    Type *old = internal_pointer.swap(raw_ptr);
    raw_ptr = nullptr;
    __alloc::__impl_dealloc(old);
    return *this;
  }

  atomic_pointer &
  operator=(nullptr_t) noexcept
  {
    clear();
    return *this;
  }

  Type *
  get(memory_order order = memory_order_acquire) const noexcept
  {
    return internal_pointer.get(order);
  }

  Type *
  operator->()
  {
    Type *p = internal_pointer.get(memory_order_acquire);
    if ( p != nullptr ) return p;
    exc<except::memory_error>("atomic_pointer operator->(): internal_pointer was null");
  }

  const Type *
  operator->() const
  {
    const Type *p = internal_pointer.get(memory_order_acquire);
    if ( p != nullptr ) return p;
    exc<except::memory_error>("atomic_pointer operator->(): internal_pointer was null");
  }

  Type &
  operator*()
  {
    Type *p = internal_pointer.get(memory_order_acquire);
    if ( p != nullptr ) return *p;
    exc<except::memory_error>("atomic_pointer operator*(): internal_pointer was null");
  }

  const Type &
  operator*() const
  {
    const Type *p = internal_pointer.get(memory_order_acquire);
    if ( p != nullptr ) return *p;
    exc<except::memory_error>("atomic_pointer operator*(): internal_pointer was null");
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return internal_pointer.get(memory_order_acquire) != nullptr;
  }

  bool
  operator!(void) const noexcept
  {
    return internal_pointer.get(memory_order_acquire) == nullptr;
  }

  bool
  active() const noexcept
  {
    return internal_pointer.get(memory_order_acquire) != nullptr;
  }

  template <is_pointer_class O>
  bool
  operator==(const O &o) const noexcept
  {
    return internal_pointer.get(memory_order_acquire) == o.get();
  }

  template <is_pointer_class O>
  bool
  operator!=(const O &o) const noexcept
  {
    return internal_pointer.get(memory_order_acquire) != o.get();
  }

  template <is_pointer_class O>
  bool
  operator<(const O &o) const noexcept
  {
    return internal_pointer.get(memory_order_acquire) < o.get();
  }

  template <is_pointer_class O>
  bool
  operator<=(const O &o) const noexcept
  {
    return internal_pointer.get(memory_order_acquire) <= o.get();
  }

  template <is_pointer_class O>
  bool
  operator>(const O &o) const noexcept
  {
    return internal_pointer.get(memory_order_acquire) > o.get();
  }

  template <is_pointer_class O>
  bool
  operator>=(const O &o) const noexcept
  {
    return internal_pointer.get(memory_order_acquire) >= o.get();
  }

  inline Type *
  release() noexcept
  {
    return internal_pointer.swap(nullptr);
  }

  inline void
  reset(Type *p = nullptr) noexcept
  {
    Type *old = internal_pointer.swap(p);
    __alloc::__impl_dealloc(old);
  }

  inline Type *
  exchange(Type *&&raw_ptr) noexcept
  {
    Type *old = internal_pointer.swap(raw_ptr);
    raw_ptr = nullptr;
    return old;
  }

  inline void
  swap(atomic_pointer &o) noexcept
  {
    Type *mine = internal_pointer.swap(nullptr);
    Type *theirs = o.internal_pointer.swap(mine);
    internal_pointer.swap(theirs);
  }

  bool
  compare_exchange_strong(Type *&expected, Type *&&desired, memory_order success = memory_order_seq_cst,
                          memory_order failure = memory_order_seq_cst) noexcept
  {
    if ( internal_pointer.compare_and_swap(expected, desired) ) {
      desired = nullptr;
      return true;
    }
    expected = internal_pointer.get(failure);
    return false;
  }

  inline void
  clear() noexcept
  {
    Type *old = internal_pointer.swap(nullptr);
    __alloc::__impl_dealloc(old);
  }
};

template <class Type> class atomic_pointer<Type[]> : private __internal_pointer_arralloc<Type>
{
  atomic_token<Type *> internal_pointer;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;
  using element_type = Type;

  using __alloc = __internal_pointer_arralloc<Type>;

  ~atomic_pointer() noexcept { __alloc::__impl_dealloc(internal_pointer.swap(nullptr)); }

  atomic_pointer(void) noexcept : internal_pointer(nullptr) {}

  template <typename V>
    requires micron::is_null_pointer_v<V>
  atomic_pointer(V) noexcept : internal_pointer(nullptr)
  {
  }

  atomic_pointer(Type *&&raw_ptr) noexcept : internal_pointer(raw_ptr) { raw_ptr = nullptr; }

  template <typename... Args>
    requires(sizeof...(Args) > 0)
  atomic_pointer(Args &&...args) : internal_pointer(__alloc::__impl_alloc(micron::forward<Args>(args)...))
  {
  }

  atomic_pointer(atomic_pointer &&o) noexcept : internal_pointer(o.internal_pointer.swap(nullptr)) {}

  atomic_pointer(const atomic_pointer &) = delete;
  atomic_pointer &operator=(const atomic_pointer &) = delete;

  atomic_pointer &
  operator=(atomic_pointer &&o) noexcept
  {
    if ( this != &o ) {
      Type *incoming = o.internal_pointer.swap(nullptr);
      Type *old = internal_pointer.swap(incoming);
      __alloc::__impl_dealloc(old);
    }
    return *this;
  }

  atomic_pointer &
  operator=(Type *&&raw_ptr) noexcept
  {
    Type *old = internal_pointer.swap(raw_ptr);
    raw_ptr = nullptr;
    __alloc::__impl_dealloc(old);
    return *this;
  }

  atomic_pointer &
  operator=(nullptr_t) noexcept
  {
    clear();
    return *this;
  }

  Type &
  operator[](const usize n)
  {
    Type *p = internal_pointer.get(memory_order_acquire);
    if ( p != nullptr ) return p[n];
    exc<except::memory_error>("atomic_pointer[] operator[](): internal_pointer was null");
  }

  const Type &
  operator[](const usize n) const
  {
    const Type *p = internal_pointer.get(memory_order_acquire);
    if ( p != nullptr ) return p[n];
    exc<except::memory_error>("atomic_pointer[] operator[](): internal_pointer was null");
  }

  Type *
  operator->()
  {
    Type *p = internal_pointer.get(memory_order_acquire);
    if ( p != nullptr ) return p;
    exc<except::memory_error>("atomic_pointer[] operator->(): internal_pointer was null");
  }

  const Type *
  operator->() const
  {
    const Type *p = internal_pointer.get(memory_order_acquire);
    if ( p != nullptr ) return p;
    exc<except::memory_error>("atomic_pointer[] operator->(): internal_pointer was null");
  }

  Type &
  operator*()
  {
    Type *p = internal_pointer.get(memory_order_acquire);
    if ( p != nullptr ) return *p;
    exc<except::memory_error>("atomic_pointer[] operator*(): internal_pointer was null");
  }

  const Type &
  operator*() const
  {
    const Type *p = internal_pointer.get(memory_order_acquire);
    if ( p != nullptr ) return *p;
    exc<except::memory_error>("atomic_pointer[] operator*(): internal_pointer was null");
  }

  const Type *
  operator()() const noexcept
  {
    return internal_pointer.get(memory_order_acquire);
  }

  const Type *
  operator&() const noexcept
  {
    return internal_pointer.get(memory_order_acquire);
  }

  Type *
  get(memory_order order = memory_order_acquire) const noexcept
  {
    return internal_pointer.get(order);
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return internal_pointer.get(memory_order_acquire) != nullptr;
  }

  bool
  operator!(void) const noexcept
  {
    return internal_pointer.get(memory_order_acquire) == nullptr;
  }

  bool
  active() const noexcept
  {
    return internal_pointer.get(memory_order_acquire) != nullptr;
  }

  inline Type *
  release() noexcept
  {
    return internal_pointer.swap(nullptr);
  }

  inline void
  reset(Type *p = nullptr) noexcept
  {
    Type *old = internal_pointer.swap(p);
    __alloc::__impl_dealloc(old);
  }

  inline Type *
  exchange(Type *&&raw_ptr) noexcept
  {
    Type *old = internal_pointer.swap(raw_ptr);
    raw_ptr = nullptr;
    return old;
  }

  inline void
  swap(atomic_pointer &o) noexcept
  {
    Type *mine = internal_pointer.swap(nullptr);
    Type *theirs = o.internal_pointer.swap(mine);
    internal_pointer.swap(theirs);
  }

  inline void
  clear() noexcept
  {
    Type *old = internal_pointer.swap(nullptr);
    __alloc::__impl_dealloc(old);
  }
};

template <typename T, typename... Args>
atomic_pointer<T>
make_atomic(Args &&...args)
{
  return atomic_pointer<T>(micron::forward<Args>(args)...);
}

template <typename T>
inline void
swap(atomic_pointer<T> &a, atomic_pointer<T> &b) noexcept
{
  a.swap(b);
}

}     // namespace micron
