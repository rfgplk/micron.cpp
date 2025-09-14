//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

#include <iostream>

namespace micron
{

// A global pointer, equivalent to a unique ptr with a few select differences
// a global pointer is meant to exist for the entire duration of the binary execution, within global scope. as such it
// does not reclaim memory on destruction
template <class Type> class __global_pointer : private __internal_pointer_alloc<Type>
{
  Type *internal_pointer;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;

  using __alloc = __internal_pointer_alloc<Type>;

  ~__global_pointer() { /*nothing*/ };
  __global_pointer(void) : internal_pointer(__alloc::__impl_alloc()) {};
  template <typename V>
    requires micron::is_null_pointer_v<V>
  __global_pointer(V) : internal_pointer(nullptr){};
  __global_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };
  template <class... Args>
    requires(sizeof...(Args) > 0)
  __global_pointer(Args &&...args)
      : internal_pointer(__alloc::__impl_alloc(micron::forward<Args>(args)...)){};     // new Type(args...)){};

  __global_pointer(__global_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  __global_pointer(const __global_pointer &p) = delete;
  __global_pointer &
  operator=(__global_pointer &&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  __global_pointer &operator=(const __global_pointer &) = delete;
  __global_pointer &
  operator=(Type *&&t) noexcept
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  template <is_pointer_class O>
  bool
  operator==(const O &o) const noexcept
  {
    return internal_pointer == o.get();
  }
  template <is_pointer_class O>
  bool
  operator>(const O &o) const noexcept
  {
    return internal_pointer > o.get();
  }
  template <is_pointer_class O>
  bool
  operator<(const O &o) const noexcept
  {
    return internal_pointer < o.get();
  }
  template <is_pointer_class O>
  bool
  operator<=(const O &o) const noexcept
  {
    return internal_pointer <= o.get();
  }
  template <is_pointer_class O>
  bool
  operator>=(const O &o) const noexcept
  {
    return internal_pointer >= o.get();
  }

  bool
  operator!(void) const noexcept
  {
    return internal_pointer == nullptr;
  };
  Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("__global_pointer operator*(): internal_pointer was null");
  }
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("__global_pointer operator*(): internal_pointer was null");
  };
  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("__global_pointer operator*(): internal_pointer was null");
  };
  inline Type *
  release() noexcept
  {
    auto *temp = internal_pointer;
    internal_pointer = nullptr;
    return temp;
  };
  Type *
  get() const noexcept
  {
    return internal_pointer;
  }
  inline void
  clear()
  {
    __alloc::__impl_dealloc(internal_pointer);
  };
};

template <class Type> class __global_pointer<Type[]> : private __internal_pointer_arralloc<Type[]>
{
  Type *internal_pointer;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;

  using __alloc = __internal_pointer_arralloc<Type[]>;

  ~__global_pointer() { /*nothing*/ };
  __global_pointer(void) = delete;
  template <typename... Args>
  requires (sizeof...(Args) > 0)
  __global_pointer(Args &&...args) : internal_pointer(__alloc::__impl_alloc(micron::forward<Args>(args)...))
  {
  }     // internal_pointer(new Type[sizeof...(args)]{ args... }){};

  __global_pointer(Type *&&raw_ptr) : internal_pointer(raw_ptr) { raw_ptr = nullptr; };

  __global_pointer(__global_pointer &&p) : internal_pointer(p.internal_pointer) { p.internal_pointer = nullptr; };
  __global_pointer(const __global_pointer &p) = delete;
  __global_pointer(__global_pointer &p) = delete;
  __global_pointer &operator=(const __global_pointer &) = delete;
  __global_pointer &
  operator=(__global_pointer &&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t.internal_pointer;
    t.internal_pointer = nullptr;
    return *this;
  };
  const Type &
  operator[](const size_t n)
  {
    if ( internal_pointer != nullptr )
      return (*internal_pointer)[n];
    else
      throw except::memory_error("__global_pointer[] operator[](): internal_pointer was null");
  };
  __global_pointer &
  operator=(Type *&&t)
  {
    __alloc::__impl_dealloc(internal_pointer);
    internal_pointer = t;
    t = nullptr;
    return *this;
  };
  const Type *
  operator()() const
  {
    return internal_pointer;
  }
  const Type *
  operator&() const
  {
    return internal_pointer;
  }
  inline Type *
  release()
  {
    auto *temp = internal_pointer;
    internal_pointer = nullptr;
    return temp;
  };
  Type *
  get() const noexcept
  {
    return internal_pointer;
  }
  inline void
  clear()
  {
    __impl_dealloc(internal_pointer);
  };
  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("__global_pointer[] operator*(): internal_pointer was null");
  };
  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      throw except::memory_error("__global_pointer[] operator*(): internal_pointer was null");
  };
  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("__global_pointer[] operator*(): internal_pointer was null");
  };

  Type *
  operator->()
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      throw except::memory_error("__global_pointer[] operator*(): internal_pointer was null");
  };
};

};
