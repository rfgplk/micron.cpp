//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
namespace rgx
{

struct charreach {
  u64 w[4] = { 0, 0, 0, 0 };

  constexpr void
  set(unsigned char c) noexcept
  {
    w[c >> 6] |= (u64(1) << (c & 63));
  }

  constexpr void
  unset(unsigned char c) noexcept
  {
    w[c >> 6] &= ~(u64(1) << (c & 63));
  }

  constexpr bool
  test(unsigned char c) const noexcept
  {
    return (w[c >> 6] >> (c & 63)) & 1u;
  }

  constexpr void
  set_range(unsigned char lo, unsigned char hi) noexcept
  {
    for ( unsigned int c = lo; c <= hi; ++c ) set((unsigned char)c);
  }

  constexpr void
  set_all(void) noexcept
  {
    w[0] = w[1] = w[2] = w[3] = ~u64(0);
  }

  constexpr void
  clear(void) noexcept
  {
    w[0] = w[1] = w[2] = w[3] = 0;
  }

  constexpr charreach &
  flip(void) noexcept
  {
    w[0] = ~w[0];
    w[1] = ~w[1];
    w[2] = ~w[2];
    w[3] = ~w[3];
    return *this;
  }

  constexpr charreach
  negated(void) const noexcept
  {
    charreach r = *this;
    return r.flip();
  }

  constexpr charreach &
  operator|=(const charreach &o) noexcept
  {
    w[0] |= o.w[0];
    w[1] |= o.w[1];
    w[2] |= o.w[2];
    w[3] |= o.w[3];
    return *this;
  }

  constexpr charreach &
  operator&=(const charreach &o) noexcept
  {
    w[0] &= o.w[0];
    w[1] &= o.w[1];
    w[2] &= o.w[2];
    w[3] &= o.w[3];
    return *this;
  }

  constexpr charreach
  operator|(const charreach &o) const noexcept
  {
    charreach r = *this;
    return r |= o;
  }

  constexpr charreach
  operator&(const charreach &o) const noexcept
  {
    charreach r = *this;
    return r &= o;
  }

  constexpr bool
  any(void) const noexcept
  {
    return (w[0] | w[1] | w[2] | w[3]) != 0;
  }

  constexpr bool
  none(void) const noexcept
  {
    return !any();
  }

  constexpr usize
  count(void) const noexcept
  {
    return (usize)__builtin_popcountll(w[0]) + (usize)__builtin_popcountll(w[1]) + (usize)__builtin_popcountll(w[2])
           + (usize)__builtin_popcountll(w[3]);
  }

  constexpr unsigned int
  single(void) const noexcept
  {
    if ( count() != 1 ) return 256;
    for ( unsigned int c = 0; c < 256; ++c )
      if ( test((unsigned char)c) ) return c;
    return 256;
  }

  constexpr bool
  operator==(const charreach &o) const noexcept
  {
    return w[0] == o.w[0] && w[1] == o.w[1] && w[2] == o.w[2] && w[3] == o.w[3];
  }
};

};      // namespace rgx
};      // namespace micron
