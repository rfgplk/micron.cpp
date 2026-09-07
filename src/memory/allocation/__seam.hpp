//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// __seam.hpp; DECLARATIONS ONLY

#include "../../types.hpp"

namespace micron
{

// WEAK AND POSSIBLY UNDEFINED; do not call it without the null test
[[gnu::weak]] bool __heap_owns_provider(const void *__ptr) noexcept;

// true if ptr lies inside memory the compiled-in allocator handed out
inline bool
__heap_owns(const void *__ptr) noexcept
{
  if ( __heap_owns_provider == nullptr ) return true;
  return __heap_owns_provider(__ptr);
}

};
