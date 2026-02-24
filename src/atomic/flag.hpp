//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "intrin.hpp"

#include "../memory/actions.hpp"
#include "../types.hpp"

#include "../__special/initializer_list"
#include "../concepts.hpp"
#include "../type_traits.hpp"

namespace micron
{

struct atomic_flag {
  atomic_token<bool> tk;

  constexpr atomic_flag() noexcept : tk(false) {}

  atomic_flag(const atomic_flag &) = delete;
  atomic_flag &operator=(const atomic_flag &) = delete;

  bool
  test(memory_order order = memory_order::seq_cst) const noexcept
  {
    return tk.get(order);
  }

  bool
  test_and_set([[maybe_unused]] memory_order order = memory_order::seq_cst) noexcept
  {
    return tk.swap(true);
  }

  void
  clear(memory_order order = memory_order::seq_cst) noexcept
  {
    tk.store(false, order);
  }

  void
  wait(bool old, memory_order order = memory_order::seq_cst) const noexcept
  {
    while ( tk.get(order) == old )
      ;
  }
};
};     // namespace micron
