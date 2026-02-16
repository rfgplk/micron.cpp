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
class sentinel_pointer
{
  const byte *const internal_pointer;

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = immutable_tag;
  using element_type = void;
  using value_type = void;

  ~sentinel_pointer() = default;
  sentinel_pointer(byte *P) : internal_pointer(P) {}                                              // internal_pointer(new Type()) {};
  sentinel_pointer(uintptr_t F) : internal_pointer(reinterpret_cast<byte *>(uintptr_t(F))) {}     // internal_pointer(new Type()) {};
  sentinel_pointer(sentinel_pointer &&p) = delete;
  sentinel_pointer(const sentinel_pointer &p) = delete;

  sentinel_pointer &operator=(const sentinel_pointer &) = delete;
  sentinel_pointer &operator=(sentinel_pointer &&t) = delete;
  const byte *
  operator()() const noexcept
  {
    return internal_pointer;
  }
  bool
  operator!(void) const noexcept
  {
    return internal_pointer == nullptr;
  };
  template <typename T>
  bool
  operator==(T *v) const noexcept
  {
    return internal_pointer == v;
  };
  bool
  operator==(uintptr_t u) const noexcept
  {
    return internal_pointer == reinterpret_cast<byte *>(u);
  };
  constexpr explicit
  operator bool() const noexcept
  {
    return internal_pointer != nullptr;
  }
  byte *operator->() = delete;
  const byte *
  operator*() const
  {
    return internal_pointer;
  }
  inline byte *release() = delete;
  void clear() = delete;
};

};
