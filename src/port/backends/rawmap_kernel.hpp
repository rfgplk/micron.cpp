//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__kport_abi.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

inline void *
raw_map(usize __sz) noexcept
{
  return ::mc_kport_raw_map(static_cast<mc_kport_usize>(__sz));
}

inline void
raw_unmap(void *__p, usize __sz) noexcept
{
  ::mc_kport_raw_unmap(__p, static_cast<mc_kport_usize>(__sz));
}

};      // namespace port
};      // namespace micron
