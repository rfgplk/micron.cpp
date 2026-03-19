//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../tags.hpp"
#include "bits.hpp"

namespace micron
{
template <class Type> class weak_pointer : private __internal_pointer_alloc<Type>
{
  Type *internal_pointer;

public:
  using pointer_type = weak_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using element_type = Type;
  using value_type = Type;
  using __alloc = __internal_pointer_alloc<Type>;

  // nonowning destructor never frees
  ~weak_pointer() {}

  weak_pointer() : internal_pointer(nullptr) {}

  template <is_nullptr V> weak_pointer(V) : internal_pointer(nullptr) {}

  template <is_pointer_class C> weak_pointer(C &c) : internal_pointer(c.get()) {}

  weak_pointer(weak_pointer &&p) : internal_pointer(p.internal_pointer)
  {
    if ( &p != this )
      p.internal_pointer = nullptr;
  }

  weak_pointer(const weak_pointer &p) : internal_pointer(p.internal_pointer) {}

  weak_pointer &
  operator=(weak_pointer &&t)
  {
    if ( this != &t ) {
      internal_pointer = t.internal_pointer;
      t.internal_pointer = nullptr;
    }
    return *this;
  }

  weak_pointer &
  operator=(const weak_pointer &t)
  {
    if ( this != &t )
      internal_pointer = t.internal_pointer;
    return *this;
  }

  template <is_pointer_class C>
  weak_pointer &
  operator=(C &ptr)
  {
    internal_pointer = ptr.get();
    return *this;
  }

  bool
  operator!(void) const noexcept
  {
    return internal_pointer == nullptr;
  }

  Type *
  operator->()
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      exc<except::memory_error>("weak_pointer operator->(): internal_pointer was null");
  }

  const Type *
  operator->() const
  {
    if ( internal_pointer != nullptr )
      return internal_pointer;
    else
      exc<except::memory_error>("weak_pointer operator->(): internal_pointer was null");
  }

  const Type &
  operator*() const
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("weak_pointer operator*(): internal_pointer was null");
  }

  Type &
  operator*()
  {
    if ( internal_pointer != nullptr )
      return *internal_pointer;
    else
      exc<except::memory_error>("weak_pointer operator*(): internal_pointer was null");
  }

  Type *
  get(void) noexcept
  {
    return internal_pointer;
  }

  const Type *
  get(void) const noexcept
  {
    return internal_pointer;
  }

  bool
  active() const noexcept
  {
    return internal_pointer != nullptr;
  }

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

  constexpr explicit
  operator bool() const noexcept
  {
    return internal_pointer != nullptr;
  }

  Type *
  operator()()
  {
    return release();
  }

  inline Type *
  release() noexcept
  {
    auto *temp = internal_pointer;
    internal_pointer = nullptr;
    return temp;
  }

  template <typename P = unique_pointer<Type>>
  inline P
  assume_ownership() noexcept
  {
    return P(micron::move(internal_pointer));     // nulls internal_pointer
  }
};
};     // namespace micron
