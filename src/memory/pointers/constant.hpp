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

  using __alloc = __internal_pointer_alloc<Type>;

  ~const_pointer() { __alloc::__impl_constdealloc(internal_pointer); };
  const_pointer() : internal_pointer(__alloc::__impl_alloc()) {}     // internal_pointer(new Type()) {};
  template <is_nullptr V> const_pointer(V) = delete;
  const_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <class... Args>
  const_pointer(Args &&...args) : internal_pointer(__alloc::__impl_alloc(forward<Args>(args)...))
  {
  }     // internal_pointer(new Type(args...)){};

  // important, cannot be moved
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
  };
  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("const_pointer operator->(): internal_pointer was null");
  }
  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("const_pointer operator*(): internal_pointer was null");
  };
  inline Type *release() = delete;
  void clear() = delete;
};

};
