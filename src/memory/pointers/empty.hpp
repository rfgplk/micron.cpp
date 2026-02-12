//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "bits.hpp"

namespace micron {
template <class Type> class empty_pointer
{
  Type *internal_pointer;

public:
  ~empty_pointer()
  {
    if ( internal_pointer != nullptr )
      exc<except::memory_error>("empty_pointer ~(): pointer is not empty");
  }
  empty_pointer(void) : internal_pointer(occupied) {}
  empty_pointer(const empty_pointer &) = delete;
  empty_pointer(empty_pointer &&o) : internal_pointer(o.internal_pointer) { o.internal_pointer = nullptr; }
  empty_pointer &operator=(const empty_pointer &) = delete;
  empty_pointer &
  operator=(empty_pointer &&o)
  {
    if ( internal_pointer != nullptr )
      exc<except::memory_error>("empty_pointer operator=(): pointer is not empty");
    internal_pointer = o.internal_pointer;
    o.internal_pointer = nullptr;
    return *this;
  }
  inline Type *
  release(void) noexcept
  {
    Type *ptr = internal_pointer;
    internal_pointer = nullptr;
    return ptr;
  }
  constexpr explicit
  operator bool() const noexcept
  {
    return internal_pointer != nullptr;
  }
};

};
