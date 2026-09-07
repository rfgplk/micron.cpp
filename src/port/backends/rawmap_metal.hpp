//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

namespace micron
{
namespace port
{

inline void *
raw_map(usize __sz) noexcept
{
  (void)__sz;
  return nullptr;
}

inline void
raw_unmap(void *__p, usize __sz) noexcept
{
  (void)__p;
  (void)__sz;
}

};      // namespace port
};      // namespace micron
