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
  using element_type = Type;

  using __alloc = __internal_pointer_alloc<shared_handler<Type>>;

  ~shared_pointer()
  {
    if ( control != nullptr && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
  }

  shared_pointer() : control(nullptr) {}

  template <is_nullptr V> shared_pointer(V) : control(nullptr) {}

  explicit shared_pointer(Type *p) : control(p ? __alloc::__impl_alloc(p, 1) : nullptr) {}

  shared_pointer(const shared_pointer &t) : control(t.control)
  {
    if ( control )
      ++control->refs;
  }

  shared_pointer(shared_pointer &t) : control(t.control)
  {
    if ( control )
      ++control->refs;
  }

  shared_pointer(shared_pointer &&t) noexcept : control(t.control) { t.control = nullptr; }

  template <class... Args>
  explicit shared_pointer(Args &&...args) : control(__alloc::__impl_alloc(__new<Type>(micron::forward<Args>(args)...), 1))
  {
  }

  shared_pointer &
  operator=(shared_pointer &&o) noexcept
  {
    if ( this != &o ) {
      if ( control != nullptr && --control->refs == 0 ) {
        __delete(control->pnt);
        __alloc::__impl_dealloc(control);
      }
      control = o.control;
      o.control = nullptr;
    }
    return *this;
  }

  shared_pointer &
  operator=(const shared_pointer &o) noexcept
  {
    if ( this != &o ) {
      if ( control != nullptr && --control->refs == 0 ) {
        __delete(control->pnt);
        __alloc::__impl_dealloc(control);
      }
      control = o.control;
      if ( control )
        ++control->refs;
    }
    return *this;
  }

  shared_pointer &
  operator=(const Type &obj) noexcept
  {
    if ( control != nullptr && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
    control = __alloc::__impl_alloc(__new<Type>(obj), 1);
    return *this;
  }

  shared_pointer &
  operator=(Type &&obj) noexcept
  {
    if ( control != nullptr && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
    control = __alloc::__impl_alloc(__new<Type>(micron::move(obj)), 1);
    return *this;
  }

  shared_pointer &
  operator=(Type *&&t) noexcept
  {
    if ( control != nullptr && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
    control = t ? __alloc::__impl_alloc(t, 1) : nullptr;
    t = nullptr;
    return *this;
  }

  shared_pointer &
  operator=(Type *&t) noexcept
  {
    return (*this = micron::move(t));
  }

  template <is_nullptr V>
  shared_pointer &
  operator=(V) noexcept
  {
    if ( control != nullptr && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
    control = nullptr;
    return *this;
  }

  Type *
  operator()() noexcept
  {
    return control ? control->pnt : nullptr;
  }

  const Type *
  operator()() const noexcept
  {
    return control ? control->pnt : nullptr;
  }

  Type *
  operator->() noexcept
  {
    return control ? control->pnt : nullptr;
  }

  const Type *
  operator->() const noexcept
  {
    return control ? control->pnt : nullptr;
  }

  Type &
  operator*()
  {
    if ( !control )
      exc<except::memory_error>("shared_pointer operator*(): internal_pointer was null");
    return *control->pnt;
  }

  const Type &
  operator*() const
  {
    if ( !control )
      exc<except::memory_error>("shared_pointer operator*(): internal_pointer was null");
    return *control->pnt;
  }

  Type *
  get()
  {
    return control ? control->pnt : nullptr;
  }

  const Type *
  get() const
  {
    return control ? control->pnt : nullptr;
  }

  template <is_pointer_class O>
  bool
  operator==(const O &o) const noexcept
  {
    return (control ? control->pnt : nullptr) == o.get();
  }

  template <is_pointer_class O>
  bool
  operator>(const O &o) const noexcept
  {
    return (control ? control->pnt : nullptr) > o.get();
  }

  template <is_pointer_class O>
  bool
  operator<(const O &o) const noexcept
  {
    return (control ? control->pnt : nullptr) < o.get();
  }

  template <is_pointer_class O>
  bool
  operator<=(const O &o) const noexcept
  {
    return (control ? control->pnt : nullptr) <= o.get();
  }

  template <is_pointer_class O>
  bool
  operator>=(const O &o) const noexcept
  {
    return (control ? control->pnt : nullptr) >= o.get();
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return control ? control->pnt != nullptr : false;
  }

  bool
  operator!() const noexcept
  {
    return control == nullptr;
  }

  usize
  refs() const noexcept
  {
    return control ? control->refs : 0;
  }
};

};     // namespace micron
