//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

namespace micron {

template <typename T> using thread_safe_handler = atomic<shared_handler<T>>;
// thread safe pointer
template <class Type> class thread_pointer : private __internal_pointer_alloc<thread_safe_handler<Type>>
{
  using pointer_type = threaded_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Type;
  
  using __alloc = __internal_pointer_alloc<thread_safe_handler<Type>>;
  thread_safe_handler<Type> *control;

public:
  ~thread_pointer()
  {
    if ( control != nullptr ) [[likely]] {
      auto *t = control->get();
      if ( !--t->refs ) [[unlikely]] {
        __delete(t->pnt);     // TODO: think about passing through stored type to __dealloc
        __alloc::__impl_dealloc(t);
        control->release();
      }
    }
  };
  thread_pointer(void)
      : control(__alloc::__impl_alloc(__new<Type>(), 1)) {};     // new shared_handler<Type>({ new Type(), 1 })) {};

  thread_pointer(Type *p)
      : control(__alloc::__impl_alloc(__new<Type>(), 1)) {}     // new thread_safe_handler<Type>({ p, 1 })) {};
  thread_pointer(const thread_pointer<Type> &t) : control(t.control) { control->get()->refs++; };
  thread_pointer(thread_pointer<Type> &&t) : control(t.control) { t.control = nullptr; };

  template <class... Args>
  thread_pointer(Args &&...args) : control(__alloc::__impl_alloc(__new<Type>(forward<Args>(args)...), 1))
  {
  }     // new thread_safe_handler<Type>({ new Type(args...), 1 })){};
  const auto
  operator=(const thread_safe_handler<Type> &o)
  {
    if ( control != nullptr ) {
      control = o.control;
      control->get()->refs++;
      control->release();
    }
  };

  const auto
  operator()() const
  {
    if ( control != nullptr ) {
      auto t = control->get()->pnt;
      control->release();
      return t;
    } else
      return nullptr;
  };

  Type *
  operator->() const
  {
    if ( control != nullptr ) {
      auto *t = *control->get()->pnt;
      control->release();
      return t;
    } else
      throw nullptr;
  };
  Type
  operator*() const
  {
    if ( control != nullptr ) {
      auto t = *control->get()->pnt;
      control->release();
      return t;
    } else
      throw nullptr;
  };
};

};
