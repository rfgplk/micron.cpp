#pragma once

#include "simd.hpp"
#include "types.hpp"

namespace micron
{
namespace simd
{

inline size_t
sse_strlen(const char *str)
{
  const char *p = str;
  i128 zero = _mm_setzero_si128();

  while ( reinterpret_cast<uintptr_t>(p) & 0xF ) {
    if ( *p == '\0' )
      return p - str;
    ++p;
  }

  for ( ;; ) {
    i128 chunk = _mm_load_si128(reinterpret_cast<const i128 *>(p));
    i128 cmp = _mm_cmpeq_epi8(chunk, zero);
    int mask = _mm_movemask_epi8(cmp);
    if ( mask != 0 ) {
      return (p - str) + __builtin_ctz(mask);
    }
    p += 16;
  }
}

};     // namespace simd

};     // namespace micron
