//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../mutex/locks.hpp"
#include "bits.hpp"

namespace micron
{

template <class Type> class thread_pointer
{
  shared_handler<Type> *control;
  mutable micron::mutex mtx;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;
  using element_type = Type;

  using __alloc = __internal_pointer_alloc<shared_handler<Type>>;

  ~thread_pointer()
  {
    micron::lock_guard lock(mtx);
    if ( control && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
  }
  thread_pointer() : control(nullptr) {}
  template <is_nullptr V> thread_pointer(V) : control(nullptr) {}
  explicit thread_pointer(Type *p)
  {
    micron::lock_guard lock(mtx);
    control = p ? __alloc::__impl_alloc(p, 1) : nullptr;
  }
  thread_pointer(const thread_pointer &t)
  {
    micron::lock_guard lock(t.mtx);
    control = t.control;
    if ( control )
      ++control->refs;
  }

  thread_pointer(thread_pointer &t)
  {
    micron::lock_guard lock(t.mtx);
    control = t.control;
    if ( control )
      ++control->refs;
  }

  thread_pointer(thread_pointer &&t) noexcept
  {
    micron::lock_guard lock(t.mtx);
    control = t.control;
    t.control = nullptr;
  }

  template <class... Args> explicit thread_pointer(Args &&...args)
  {
    micron::lock_guard lock(mtx);
    control = __alloc::__impl_alloc(__new<Type>(micron::forward<Args>(args)...), 1);
  }

  thread_pointer &
  operator=(thread_pointer &&o) noexcept
  {
    if ( this != &o ) {
      micron::lock_guard lock_this(mtx);
      micron::lock_guard lock_other(o.mtx);

      if ( control && --control->refs == 0 ) {
        __delete(control->pnt);
        __alloc::__impl_dealloc(control);
      }

      control = o.control;
      o.control = nullptr;
    }
    return *this;
  }

  thread_pointer &
  operator=(const thread_pointer &o) noexcept
  {
    if ( this != &o ) {
      micron::lock_guard lock_this(mtx);
      micron::lock_guard lock_other(o.mtx);

      if ( control && --control->refs == 0 ) {
        __delete(control->pnt);
        __alloc::__impl_dealloc(control);
      }

      control = o.control;
      if ( control )
        ++control->refs;
    }
    return *this;
  }

  thread_pointer &
  operator=(const Type &obj) noexcept
  {
    micron::lock_guard lock(mtx);
    if ( control && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
    control = __alloc::__impl_alloc(__new<Type>(obj), 1);
    return *this;
  }

  thread_pointer &
  operator=(Type &&obj) noexcept
  {
    micron::lock_guard lock(mtx);
    if ( control && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
    control = __alloc::__impl_alloc(__new<Type>(micron::move(obj)), 1);
    return *this;
  }

  thread_pointer &
  operator=(Type *&&t) noexcept
  {
    micron::lock_guard lock(mtx);
    if ( control && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
    control = t ? __alloc::__impl_alloc(t, 1) : nullptr;
    t = nullptr;
    return *this;
  }

  thread_pointer &
  operator=(Type *&t) noexcept
  {
    return (*this = micron::move(t));
  }

  template <is_nullptr V>
  thread_pointer &
  operator=(V) noexcept
  {
    micron::lock_guard lock(mtx);
    if ( control && --control->refs == 0 ) {
      __delete(control->pnt);
      __alloc::__impl_dealloc(control);
    }
    control = nullptr;
    return *this;
  }

  Type *
  operator()() noexcept
  {
    micron::lock_guard lock(mtx);
    return control ? control->pnt : nullptr;
  }

  const Type *
  operator()() const noexcept
  {
    micron::lock_guard lock(mtx);
    return control ? control->pnt : nullptr;
  }

  Type *
  operator->() noexcept
  {
    micron::lock_guard lock(mtx);
    return control ? control->pnt : nullptr;
  }

  const Type *
  operator->() const noexcept
  {
    micron::lock_guard lock(mtx);
    return control ? control->pnt : nullptr;
  }

  Type &
  operator*()
  {
    micron::lock_guard lock(mtx);
    if ( !control )
      exc<except::memory_error>("thread_pointer operator*(): internal_pointer was null");
    return *control->pnt;
  }

  const Type &
  operator*() const
  {
    micron::lock_guard lock(mtx);
    if ( !control )
      exc<except::memory_error>("thread_pointer operator*(): internal_pointer was null");
    return *control->pnt;
  }

  Type *
  get()
  {
    micron::lock_guard lock(mtx);
    return control ? control->pnt : nullptr;
  }

  const Type *
  get() const
  {
    micron::lock_guard lock(mtx);
    return control ? control->pnt : nullptr;
  }

  bool
  operator!() const noexcept
  {
    micron::lock_guard lock(mtx);
    return control == nullptr;
  }

  constexpr explicit
  operator bool() const noexcept
  {
    micron::lock_guard lock(mtx);
    return control ? control->refs : 0;
  }
  size_t
  refs() const noexcept
  {
    micron::lock_guard lock(mtx);
    return control ? control->refs : 0;
  }
};

};
