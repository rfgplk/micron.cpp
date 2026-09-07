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

inline i64
wait(u32 *__addr, u32 __expected, i64 __timeout_ns) noexcept
{
  return static_cast<i64>(::mc_kport_wait(reinterpret_cast<mc_kport_u32 *>(__addr), static_cast<mc_kport_u32>(__expected),
                                          static_cast<mc_kport_i64>(__timeout_ns)));
}

inline i64
wake(u32 *__addr, i32 __n) noexcept
{
  return static_cast<i64>(::mc_kport_wake(reinterpret_cast<mc_kport_u32 *>(__addr), static_cast<mc_kport_i32>(__n)));
}

};      // namespace port
};      // namespace micron
