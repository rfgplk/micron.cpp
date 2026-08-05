//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// environ
//
// declared here as a lightweight include
//
// the crt assigns environ in start.cpp before main, so it works under -ffreestanding too

// declared global due to convention
extern "C" char **environ;

namespace micron
{

[[nodiscard]] inline const char *
env_get(const char *key) noexcept
{
  if ( environ == nullptr || key == nullptr ) return nullptr;
  for ( char **e = environ; *e != nullptr; ++e ) {
    const char *p = *e;
    usize i = 0;
    for ( ; key[i] != 0 && p[i] == key[i]; ++i );
    // the '=' test is what stops PATHEXT answering a lookup for PATH
    if ( key[i] == 0 && p[i] == '=' ) return p + i + 1;
  }
  return nullptr;
}

[[nodiscard]] inline bool
env_has(const char *key) noexcept
{
  const char *v = env_get(key);
  return v != nullptr && v[0] != 0;
}

};      // namespace micron
