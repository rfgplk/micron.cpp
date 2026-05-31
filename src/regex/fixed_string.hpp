//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

// fixed_string<N>
//
// a small (literal-class) compile-time string usable as a non-type template parameter
namespace micron
{

template<usize N> struct fixed_string {
  char buf[N]{};

  constexpr fixed_string() noexcept = default;

  constexpr fixed_string(const char (&s)[N]) noexcept
  {
    for ( usize i = 0; i < N; ++i ) buf[i] = s[i];
  }

  constexpr usize
  size() const noexcept
  {
    return N ? N - 1 : 0;
  }

  constexpr char
  operator[](usize i) const noexcept
  {
    return buf[i];
  }

  constexpr const char *
  data() const noexcept
  {
    return buf;
  }

  constexpr const char *
  begin() const noexcept
  {
    return buf;
  }

  constexpr const char *
  end() const noexcept
  {
    return buf + size();
  }
};

template<usize N> fixed_string(const char (&)[N]) -> fixed_string<N>;

};      // namespace micron
