//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

namespace micron
{
// A constant pointer, cannot be reassigned nor written to
template <class Type> class const_pointer : private __internal_pointer_alloc<Type>
{
  const Type *const internal_pointer;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = immutable_tag;
  using value_type = Type;
  using element_type = Type;
  using __alloc = __internal_pointer_alloc<Type>;

  ~const_pointer() { __alloc::__impl_constdealloc(internal_pointer); }

  const_pointer() : internal_pointer(__alloc::__impl_alloc()) {}

  template <is_nullptr V> const_pointer(V) = delete;

  const_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; }

  template <class... Args> const_pointer(Args &&...args) : internal_pointer(__alloc::__impl_alloc(forward<Args>(args)...)) {}

  // Cannot be moved nor copied
  const_pointer(const_pointer &&p) = delete;
  const_pointer(const const_pointer &p) = delete;
  const_pointer &operator=(const const_pointer &) = delete;
  const_pointer &operator=(const_pointer &&t) = delete;

  const Type *
  operator()() const noexcept
  {
    return internal_pointer;
  }

  bool
  operator!(void) const noexcept
  {
    return internal_pointer == nullptr;
  }

  bool
  operator==(const Type *u) const noexcept
  {
    return internal_pointer == u;
  }

  bool
  operator==(uintptr_t u) const noexcept
  {
    return internal_pointer == reinterpret_cast<const Type *>(u);
  }

  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      exc<except::memory_error>("const_pointer operator->(): internal_pointer was null");
  }

  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("const_pointer operator*(): internal_pointer was null");
  }

  inline Type *release() = delete;
  void clear() = delete;
};

// A constant array pointer, cannot be reassigned nor written to
template <class Type> class const_pointer<Type[]> : private __internal_pointer_alloc<Type[]>
{
  const Type *const internal_pointer;
  const usize array_size;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = immutable_tag;
  using value_type = Type;
  using element_type = Type;
  using __alloc = __internal_pointer_alloc<Type[]>;

  ~const_pointer() { __alloc::__impl_constdealloc(internal_pointer); }

  const_pointer() : internal_pointer(nullptr), array_size(0) {}

  explicit const_pointer(usize size) : internal_pointer(__alloc::__impl_alloc(size)), array_size(size) {}

  template <is_nullptr V> const_pointer(V) = delete;

  const_pointer(Type *&&raw_ptr, usize size) : internal_pointer(raw_ptr), array_size(size) { raw_ptr = nullptr; }

  const_pointer(const_pointer &&p) = delete;
  const_pointer(const const_pointer &p) = delete;
  const_pointer &operator=(const const_pointer &) = delete;
  const_pointer &operator=(const_pointer &&t) = delete;

  const Type *
  operator()() const noexcept
  {
    return internal_pointer;
  }

  bool
  operator!(void) const noexcept
  {
    return internal_pointer == nullptr;
  }

  bool
  operator==(const Type *u) const noexcept
  {
    return internal_pointer == u;
  }

  bool
  operator==(uintptr_t u) const noexcept
  {
    return internal_pointer == reinterpret_cast<const Type *>(u);
  }

  const Type &
  operator[](usize index) const
  {
    if ( internal_pointer != nullptr && index < array_size )
      return internal_pointer[index];
    else
      exc<except::memory_error>("const_pointer operator[]: index out of bounds or internal_pointer was null");
  }

  usize
  size() const noexcept
  {
    return array_size;
  }

  const Type *
  data() const noexcept
  {
    return internal_pointer;
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return internal_pointer != nullptr;
  }

  inline Type *release() = delete;
  void clear() = delete;
};

}     // namespace micron
