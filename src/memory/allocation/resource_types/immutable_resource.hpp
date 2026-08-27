//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "mutable_resource.hpp"

namespace micron
{

template<typename T, typename Alloc = allocator_serial<>>
  requires micron::is_copy_constructible_v<T> and micron::is_move_constructible_v<T>
struct __immutable_memory_resource: public __owned_memory_resource<T, Alloc> {
  using __base = __owned_memory_resource<T, Alloc>;
  using __base::__base;
  __immutable_memory_resource(__immutable_memory_resource &&) noexcept = default;
  __immutable_memory_resource &operator=(__immutable_memory_resource &&) noexcept = default;
  __immutable_memory_resource(const __immutable_memory_resource &) = delete;
  __immutable_memory_resource &operator=(const __immutable_memory_resource &) = delete;
};

};      // namespace micron
