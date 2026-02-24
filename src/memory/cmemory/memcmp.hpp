//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../attributes.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../simd/intrin.hpp"
#include "../../simd/memory.hpp"

namespace micron
{
template <typename T, typename F>
long int
memcmp(const F *__restrict _src, const F *__restrict _dest, size_t cnt) noexcept
{
  const byte *src = reinterpret_cast<const T *>(_src);
  const byte *dest = reinterpret_cast<const T *>(_dest);
  for ( size_t i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return &src[i] - &dest[i];
  return 0;
};

long int
bytecmp(const byte *__restrict src, const byte *__restrict dest, size_t cnt) noexcept
{
  for ( size_t i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return &src[i] - &dest[i];
  return 0;
};

};     // namespace micron
