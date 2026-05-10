#pragma once

#include "../../attributes.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../simd/intrin.hpp"
#include "../../simd/memory.hpp"

#include "bits.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memchr
template <typename T>
  requires(!micron::is_null_pointer_v<T>)
T *
memchr(const T &restrict src, byte c, u64 n) noexcept
{
  if ( src == nullptr ) return nullptr;
  if constexpr ( sizeof(T) == 1 ) {
    const u64 bytes = n;
    if ( bytes < __simd_dispatch_threshold ) {
      for ( u64 i = 0; i < bytes; i++ )
        if ( src[i] == static_cast<T>(c) ) return const_cast<T *>(src + i);
      return nullptr;
    }
#if defined(__micron_x86_avx2)
    return simd::memchr256<T>(src, c, n);
#else
    return simd::memchr128<T>(src, c, n);
#endif
  } else {
    for ( u64 i = 0; i < n; i++ )
      if ( src[i] == c ) return const_cast<T *>(src + i);
    return nullptr;
  }
}

template <typename T>
  requires(!micron::is_null_pointer_v<T>)
T *
memchr(const T *restrict src, byte c, u64 n) noexcept
{
  if ( src == nullptr ) return nullptr;
  if constexpr ( sizeof(T) == 1 ) {
    const u64 bytes = n;
    if ( bytes < __simd_dispatch_threshold ) {
      for ( u64 i = 0; i < bytes; i++ )
        if ( src[i] == static_cast<T>(c) ) return const_cast<T *>(src + i);
      return nullptr;
    }
#if defined(__micron_x86_avx2)
    return simd::memchr256<T>(src, c, n);
#else
    return simd::memchr128<T>(src, c, n);
#endif
  } else {
    for ( u64 i = 0; i < n; i++ )
      if ( src[i] == c ) return const_cast<T *>(src + i);
    return nullptr;
  }
}

// %%%%%%%%%%%%%%%%%%%%
// memrchr

template <typename T>
  requires(!micron::is_null_pointer_v<T>)
T *
memrchr(const T *restrict src, byte c, u64 n) noexcept
{
  if ( src == nullptr ) return nullptr;
  if constexpr ( sizeof(T) == 1 ) {
    const u64 bytes = n;
    if ( bytes < __simd_dispatch_threshold ) {
      for ( u64 i = bytes; i > 0; i-- )
        if ( src[i - 1] == static_cast<T>(c) ) return const_cast<T *>(src + i - 1);
      return nullptr;
    }
#if defined(__micron_x86_avx2)
    return simd::memrchr256<T>(src, c, n);
#else
    return simd::memrchr128<T>(src, c, n);
#endif
  } else {
    for ( u64 i = n; i > 0; i-- )
      if ( src[i - 1] == c ) return const_cast<T *>(src + i - 1);
    return nullptr;
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memmem

template <typename T>
  requires(!micron::is_null_pointer_v<T>)
T *
memmem(const T *restrict hay, u64 hlen, const T *restrict nee, u64 nlen) noexcept
{
  if ( hay == nullptr ) return nullptr;
  if ( nlen == 0 ) return const_cast<T *>(hay);
  if ( nee == nullptr ) return nullptr;
  if ( nlen > hlen ) return nullptr;

  if constexpr ( sizeof(T) == 1 ) {
    const u64 hbytes = hlen;
    if ( hbytes < __simd_dispatch_threshold ) {
      const u64 limit = hbytes - nlen + 1;
      for ( u64 i = 0; i < limit; i++ ) {
        bool match = true;
        for ( u64 j = 0; j < nlen; j++ )
          if ( hay[i + j] != nee[j] ) {
            match = false;
            break;
          }
        if ( match ) return const_cast<T *>(hay + i);
      }
      return nullptr;
    }
#if defined(__micron_x86_avx2)
    return simd::memmem256<T>(hay, hlen, nee, nlen);
#else
    return simd::memmem128<T>(hay, hlen, nee, nlen);
#endif
  } else {
    const u64 limit = hlen - nlen + 1;
    for ( u64 i = 0; i < limit; i++ ) {
      bool match = true;
      for ( u64 j = 0; j < nlen; j++ )
        if ( hay[i + j] != nee[j] ) {
          match = false;
          break;
        }
      if ( match ) return const_cast<T *>(hay + i);
    }
    return nullptr;
  }
}

};     // namespace micron
