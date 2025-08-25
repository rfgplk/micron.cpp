//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

namespace micron
{

template <class Type> class shared_pointer : private __internal_pointer_alloc<shared_handler<Type>>
{
  shared_handler<Type> *control;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;

  using __alloc = __internal_pointer_alloc<shared_handler<Type>>;

  ~shared_pointer()
  {
    if ( control != nullptr ) [[likely]] {
      if ( !--control->refs ) [[unlikely]] {
        __delete(control->pnt);     // TODO: think about passing through stored type to __dealloc
        __alloc::__impl_dealloc(control);
        // delete control->pnt;
        // delete control;
      }
    }
  };
  shared_pointer(void)
      : control(__alloc::__impl_alloc(__new<Type>(), 1)) {};     // new shared_handler<Type>({ new Type(), 1 })) {};
  template <is_nullptr V> shared_pointer(V) : control(nullptr){};
  shared_pointer(Type *p) : control(__impl_alloc(p, 1)) {};     // new shared_pointer_handler<Type>({ p, 1 })) {};
  shared_pointer(const shared_pointer<Type> &t) : control(t.control)
  {
    if ( control != nullptr ) [[likely]]
      control->refs++;
  };
  shared_pointer(shared_pointer<Type> &t) : control(t.control)
  {
    if ( control != nullptr ) [[likely]]
      control->refs++;
  };
  shared_pointer(shared_pointer<Type> &&t) noexcept : control(t.control) { t.control = nullptr; };

  template <class... Args>
  shared_pointer(Args &&...args)
      : control(__alloc::__impl_alloc(__new<Type>(forward<Args>(args)...),
                                      1)){};     // control(new shared_handler<Type>({ new Type(args...), 1 })){};
  shared_pointer &
  operator=(shared_pointer<Type> &&o) noexcept
  {
    if ( control != nullptr ) {
      control = o.control;
    }
    return *this;
  };
  shared_pointer &
  operator=(const shared_pointer<Type> &o) noexcept
  {
    if ( control != nullptr ) {
      control = o.control;
      control->refs++;
    }
    return *this;
  };
  template <is_nullptr V>
  shared_pointer &
  operator=(V) noexcept
  {
    if ( control != nullptr ) {
      control->refs--;
    }
    return *this;
  };
  Type *
  operator()() noexcept
  {
    if ( control != nullptr ) {
      return control->pnt;
    } else
      return nullptr;
  };
  const Type *
  operator()() const noexcept
  {
    if ( control != nullptr ) {
      return control->pnt;
    } else
      return nullptr;
  };

  Type *
  operator->() noexcept
  {
    if ( control != nullptr )
      return control->pnt;
    else
      return nullptr;
  };
  const Type *
  operator->() const noexcept
  {
    if ( control != nullptr )
      return control->pnt;
    else
      return nullptr;
  };

  bool
  operator!(void) const noexcept
  {
    return control == nullptr;
  };
  Type &
  operator*()
  {
    if ( control != nullptr )
      return *control->pnt;
    else
      throw except::memory_error("shared_pointer operator*(): internal_pointer was null");
  };
  const Type &
  operator*() const
  {
    if ( control != nullptr )
      return *(control->pnt);
    else
      throw except::memory_error("shared_pointer operator*(): internal_pointer was null");
  };
  Type *
  get(void)
  {
    if ( control != nullptr )
      return control->pnt;
    else
      throw except::memory_error("shared_pointer operator*(): internal_pointer was null");
  };
  const Type *
  get(void) const
  {
    if ( control != nullptr )
      return control->pnt;
    else
      throw except::memory_error("shared_pointer operator*(): internal_pointer was null");
  };
  size_t
  refs() const noexcept
  {
    if ( control != nullptr )
      return control->refs;
    else
      return sizeof(size_t);
  }
};
};
