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

  void
  __release() noexcept
  {
    if ( control != nullptr && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
    control = nullptr;
  }

  static shared_handler<Type> *
  __make_control(Type *p)
  {

    try {
      return __alloc::__impl_alloc(p, static_cast<count_t>(1));
    } catch ( ... ) {
      __delete(p);
      throw;
    }
  }

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;
  using element_type = Type;

  using __alloc = __internal_pointer_alloc<shared_handler<Type>>;

  ~shared_pointer() { __release(); }

  shared_pointer() noexcept : control(nullptr) {}

  template <is_nullptr V> shared_pointer(V) noexcept : control(nullptr) {}

  explicit shared_pointer(Type *p) : control(p ? __make_control(p) : nullptr) {}

  shared_pointer(const shared_pointer &t) noexcept : control(t.control)
  {
    if ( control )
      ++control->refs;
  }

  shared_pointer(shared_pointer &t) noexcept : control(t.control)
  {
    if ( control )
      ++control->refs;
  }

  shared_pointer(shared_pointer &&t) noexcept : control(t.control) { t.control = nullptr; }

  template <class... Args> explicit shared_pointer(Args &&...args) : control(__make_control(__new<Type>(micron::forward<Args>(args)...))) {}

  shared_pointer &
  operator=(shared_pointer &&o) noexcept
  {
    if ( this != &o ) {
      __release();
      control = o.control;
      o.control = nullptr;
    }
    return *this;
  }

  shared_pointer &
  operator=(const shared_pointer &o) noexcept
  {
    if ( this != &o ) {

      if ( o.control )
        ++o.control->refs;
      __release();
      control = o.control;
    }
    return *this;
  }

  shared_pointer &
  operator=(const Type &obj)
  {
    shared_handler<Type> *nc = __make_control(__new<Type>(obj));
    __release();
    control = nc;
    return *this;
  }

  shared_pointer &
  operator=(Type &&obj)
  {
    shared_handler<Type> *nc = __make_control(__new<Type>(micron::move(obj)));
    __release();
    control = nc;
    return *this;
  }

  shared_pointer &
  operator=(Type *&&t) noexcept
  {
    if ( t == nullptr ) {
      __release();
      t = nullptr;
      return *this;
    }

    shared_handler<Type> *nc = __make_control(t);
    t = nullptr;
    __release();
    control = nc;
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
    __release();
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
    if ( !control || !control->pnt )
      exc<except::memory_error>("shared_pointer operator*(): pointer was null");
    return *control->pnt;
  }

  const Type &
  operator*() const
  {
    if ( !control || !control->pnt )
      exc<except::memory_error>("shared_pointer operator*(): pointer was null");
    return *control->pnt;
  }

  Type *
  get() noexcept
  {
    return control ? control->pnt : nullptr;
  }

  const Type *
  get() const noexcept
  {
    return control ? control->pnt : nullptr;
  }

  bool
  operator==(nullptr_t) const noexcept
  {
    return get() == nullptr;
  }

  template <is_pointer_class O>
  bool
  operator==(const O &o) const noexcept
  {
    return get() == o.get();
  }

  template <is_pointer_class O>
  bool
  operator>(const O &o) const noexcept
  {
    return get() > o.get();
  }

  template <is_pointer_class O>
  bool
  operator<(const O &o) const noexcept
  {
    return get() < o.get();
  }

  template <is_pointer_class O>
  bool
  operator<=(const O &o) const noexcept
  {
    return get() <= o.get();
  }

  template <is_pointer_class O>
  bool
  operator>=(const O &o) const noexcept
  {
    return get() >= o.get();
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return control != nullptr && control->pnt != nullptr;
  }

  bool
  operator!() const noexcept
  {
    return control == nullptr || control->pnt == nullptr;
  }

  usize
  refs() const noexcept
  {
    return control ? control->refs : 0;
  }
};

};     // namespace micron
