//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once


#include "bits.hpp"

namespace micron {

class void_pointer //: private __internal_pointer_alloc<void>
{
  void *internal_pointer;
public:
  using pointer_type = void_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = immutable_tag;
  using element_type = void;
  using value_type = void;

  using __alloc = __internal_pointer_alloc<void>;
/*
public:
  ~void_pointer() { __alloc::__impl_dealloc(internal_pointer); };
  void_pointer(void) : internal_pointer(__alloc::__impl_alloc()) {};     // new Type()) {};
  template <typename V>
    requires micron::is_null_pointer_v<V>
  void_pointer(V) : internal_pointer(nullptr){};
  void_pointer(void_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  void_pointer(const void_pointer &p) = delete;

  void_pointer &operator=(const void_pointer &) = delete;
  void_pointer &
  operator=(void *&&t) noexcept
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  void_pointer &
  operator=(void_pointer &&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  void *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      exc<except::memory_error>("void_pointer operator*(): internal_pointer was null");
  }
  template <typename Type>
  inline Type *
  yield() noexcept
  {
    Type *temp = reinterpret_cast<Type *>(internal_pointer);
    internal_pointer = nullptr;
    return temp;
  };

  inline void
  clear()
  {
    __alloc::__impl_dealloc(internal_pointer);
  };*/
};

};
