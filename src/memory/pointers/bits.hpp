//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../memory/actions.hpp"
#include "../../memory/new.hpp"
#include "../../tags.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

namespace micron
{
using count_t = size_t;
constexpr static const ptr_t nullvalue = 0x0;
constexpr static const ptr_t occupied = 0x1;

template <typename T>
concept is_pointer_class = requires {
  typename T::pointer_type;
  typename T::category_type;
} && requires {
  { typename T::category_type{} } -> micron::same_as<micron::pointer_tag>;
};

template <class Type> struct __internal_pointer_alloc {
  template <typename... Args>
  static inline __attribute__((always_inline)) Type *
  __impl_alloc(Args &&...args)
  {
    return __new<Type>(micron::forward<Args &&>(args)...);
  }

  template <typename Arr>
  static inline __attribute__((always_inline)) void
  __impl_dealloc_arr(Arr *pointer)
  {
    if ( pointer != nullptr ) {
      __delete_arr(pointer);
    }
  }

  static inline __attribute__((always_inline)) void
  __impl_dealloc(Type *pointer)
  {
    if ( pointer != nullptr ) {
      __delete(pointer);
    }
  }

  static inline __attribute__((always_inline)) void
  __impl_constdealloc(const Type *const pointer)
  {
    if ( pointer != nullptr ) {
      __const_delete(pointer);
    }
  }
};

template <class Type> struct __internal_pointer_arralloc {
  static inline __attribute__((always_inline)) auto
  __impl_alloc(size_t n)
  {
    return __new_arr<Type>(n);
    // return new Type(micron::forward<Args>(args)...);
  }

  template <typename A>
  static inline __attribute__((always_inline)) void
  __impl_dealloc(A *pointer)
  {
    if ( pointer != nullptr ) {
      __delete_arr(pointer);
    }
  }
};

template <typename T> struct shared_handler {
  T *pnt;
  count_t refs;
  // count_t weaks
};

template <typename T> using thread_safe_handler = atomic<shared_handler<T>>;

template <typename T>
concept is_nullptr = micron::is_null_pointer_v<T>;
template <typename T>
concept is_valid_pointer = micron::is_pointer_v<T> and !micron::is_null_pointer_v<T>;

};
