//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/elf/elf.hpp"
#include "../vector.hpp"

namespace micron
{
namespace elf
{

inline const micron::vector<symbol_info_t> &
list_symbols(const handle_t &h) noexcept
{
  return h.symbols();
}

};      // namespace elf
};      // namespace micron
