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

// Return the per-handle symbol vector built at open() time. Includes every
// .dynsym entry — defined exports AND undefined imports. Callers filter on
// `symbol_info_t::defined` if only exports are wanted.
//
// The returned reference (and the strings/addresses inside each row) stays
// valid for the lifetime of `h`.
inline const micron::vector<symbol_info_t> &
list_symbols(const handle_t &h) noexcept
{
  return h.symbols();
}

};      // namespace elf
};      // namespace micron
