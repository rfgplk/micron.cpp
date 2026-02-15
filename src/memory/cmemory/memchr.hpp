#pragma once

#include "../../attributes.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../simd/intrin.hpp"
#include "../../simd/memory.hpp"

namespace micron
{
template <typename T>
  requires(!micron::is_null_pointer_v<T>)
T *
memchr(const T &restrict src, byte c, u64 n) noexcept
{
  if ( src == nullptr )
    return nullptr;
  for ( u64 i = 0; i < n; i++ )
    if ( src[i] == c )
      return const_cast<T *>(src + i);
  return nullptr;
}

template <typename T>
  requires(!micron::is_null_pointer_v<T>)
T *
memchr(const T *restrict src, byte c, u64 n) noexcept
{
  if ( src == nullptr )
    return nullptr;
  for ( u64 i = 0; i < n; i++ )
    if ( src[i] == c )
      return const_cast<T *>(src + i);
  return nullptr;
}

};
