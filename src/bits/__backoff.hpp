//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__arch.hpp"
#include "__pause.hpp"

#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// lockfree plumbing

namespace micron
{

// exponential CAS backoff; doubles per failed attempt, saturates at 64 pauses
[[gnu::always_inline]] inline unsigned
__spin_backoff(unsigned b) noexcept
{
  for ( unsigned i = 0; i < b; ++i ) __cpu_pause();
  return (b < 64u) ? (b << 1u) : 64u;
}

// portable cache-line filler
template<usize N> struct __cache_pad {
  char __[N];
};

// no unique address should be free
template<> struct __cache_pad<0> {
};

};      // namespace micron
