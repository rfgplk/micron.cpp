//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

namespace micron
{
// A sentinel pointer
// only keeps around a pointer value, nothing else
class sentinel_pointer
{
  const byte *const internal_pointer;

public:
  using pointer_type = void_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = immutable_tag;
  using element_type = void;
  using value_type = void;

  ~sentinel_pointer() = default;

  sentinel_pointer(const byte *P) noexcept : internal_pointer(P) {}

  sentinel_pointer(uintptr_t F) noexcept     // fix 5
      : internal_pointer(reinterpret_cast<byte *>(F))
  {
  }

  sentinel_pointer(sentinel_pointer &&) = delete;
  sentinel_pointer(const sentinel_pointer &) = delete;
  sentinel_pointer &operator=(const sentinel_pointer &) = delete;
  sentinel_pointer &operator=(sentinel_pointer &&) = delete;

  const byte *
  operator()() const noexcept
  {
    return internal_pointer;
  }

  bool
  operator!() const noexcept
  {
    return internal_pointer == nullptr;
  }

  explicit
  operator bool() const noexcept
  {
    return internal_pointer != nullptr;
  }

  bool
  operator==(const void *v) const noexcept
  {
    return internal_pointer == v;
  }

  bool
  operator==(uintptr_t u) const noexcept
  {
    return internal_pointer == reinterpret_cast<const byte *>(u);
  }

  const byte &
  operator*() const
  {
    return *internal_pointer;
  }

  byte *operator->() = delete;
  byte *release() = delete;
  void clear() = delete;
};

};     // namespace micron
