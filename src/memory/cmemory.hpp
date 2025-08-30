//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../attributes.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../simd/intrin.hpp"

// NOTE: originally included all memory handling functions, later separated into cmemory.hpp, cstring.hpp, and map.hpp
// for maintainability check all include headers to make sure they match

// TO ALL READERS
// this set of functions is ever so slightly different from the standard string.h set of memory manip. functions
// generally if you'd like to set memory you should use
// bset = set bytes, in bytes
// memset = set memory, in bytes or type provided (ie size of the type you provided)
// cmemset = set memory with known length at compile time
// wordset = set word, in words
// typeset = set memory, as if it were provided type
// functions suffixed with a number represent the width or alignment of the function
// functions prefixed with a 'c' all take in certain parameters at compile time

namespace micron
{

template <typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
F &
rtypeset(F &s, const T in, const size_t cnt)
{
  T *src = reinterpret_cast<T *>(&s);
  if ( cnt % 4 == 0 )
    for ( size_t n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F &>(src);
};

template <typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
typeset(F *s, const T in, const size_t cnt)
{
  T *src = reinterpret_cast<T *>(s);
  if ( cnt % 4 == 0 )
    for ( size_t n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

#define stackalloc(x, T) reinterpret_cast<T *>(__builtin_alloca(x));

template <size_t M, typename F, size_t L>
constexpr F *
cfull(const F (&src)[L])
{
  for ( size_t n = 0; n < M; n++ )
    src[n] = 0xFF;
  return reinterpret_cast<F *>(src);
};

template <size_t M, typename F>
constexpr F *
cfull(F *src)
{
  for ( size_t n = 0; n < M; n++ )
    src[n] = 0xFF;
  return reinterpret_cast<F *>(src);
};

template <size_t M, typename F>
constexpr F *
cfull(const F *src)
{
  for ( size_t n = 0; n < M; n++ )
    src[n] = 0xFF;
  return reinterpret_cast<F *>(src);
};

template <typename F, size_t L, typename M = size_t>
constexpr F *
full(const F (&src)[L], const M cnt)
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0xFF;
  return reinterpret_cast<F *>(src);
};
template <typename F, typename M = size_t>
constexpr F *
full(F *src, const M cnt)
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0xFF;
  return reinterpret_cast<F *>(src);
};

template <size_t M, typename F>
constexpr F &
czero(F &src)
{
  for ( size_t n = 0; n < M; n++ )
    src[n] = 0x0;
  return src;
};

template <size_t M, typename F>
constexpr F *
czero(F *src)
{
  for ( size_t n = 0; n < M; n++ )
    src[n] = 0x0;
  return reinterpret_cast<F *>(src);
};
template <size_t M, typename F>
constexpr F &
cbzero(F &_src)
{
  byte *src = reinterpret_cast<F *>(_src);
  for ( size_t n = 0; n < M; n++ )
    src[n] = 0x0;
  return src;
};

template <size_t M, typename F>
constexpr F *
cbzero(F *_src)
{
  byte *src = reinterpret_cast<F *>(_src);
  for ( size_t n = 0; n < M; n++ )
    src[n] = 0x0;
  return reinterpret_cast<F *>(src);
};

template <typename F, size_t L, typename M = size_t>
F *
zero(const F (&src)[L], const M cnt)
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x0;
  return reinterpret_cast<F *>(src);
};
template <typename F, typename M = size_t>
F *
zero(F *src, const M cnt)
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x0;
  return reinterpret_cast<F *>(src);
};

template <typename M = size_t>
byte *
bzero(byte *src, const M cnt)
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x0;
  return src;
};
template <typename F>
F *
memfrob(F *src, size_t n)
{
  F *a = src;
  while ( n-- > 0 )
    *a++ ^= 0x15;
  return a;
}

template <typename F, typename N = byte>
F *
memset256(F *src, N in, const size_t cnt)
{
  __m256i v = _mm256_set1_epi32(in);
  for ( size_t n = 0; n < cnt; n += 8 ) {
    _mm256_store_si256(reinterpret_cast<__m256i *>(src + n), v);
  }
  return reinterpret_cast<F *>(src);
};

template <typename F, typename N = byte>
F *
memset128(F *src, N in, const size_t cnt)
{
  __m128i v = _mm_set1_epi32(in);
  for ( size_t n = 0; n < cnt; n += 4 ) {
    _mm_store_si128(reinterpret_cast<__m128i *>(src + n), v);
  }
  return reinterpret_cast<F *>(src);
};

template <size_t M, typename F, typename N>
F *
cmemset(F *src, const N in)
{
  if constexpr ( micron::is_pointer_v<F> ) {
    for ( size_t n = 0; n < M; n++ )
      src[n] = reinterpret_cast<F>(in);
    return reinterpret_cast<F *>(src);
  } else if constexpr ( M % 4 == 0 )
    for ( size_t n = 0; n < M; n += 4 ) {
      src[n] = static_cast<F>(in);
      src[n + 1] = static_cast<F>(in);
      src[n + 2] = static_cast<F>(in);
      src[n + 3] = static_cast<F>(in);
    }
  else if constexpr ( M % 4 != 0 )
    for ( size_t n = 0; n < M; n++ )
      src[n] = static_cast<F>(in);
  return reinterpret_cast<F *>(src);
};

void *
bset(void *isrc, const byte in, const size_t cnt)
{
  byte *src = reinterpret_cast<byte *>(isrc);
  if ( cnt % 4 == 0 )
    for ( size_t n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      src[n] = in;
  return reinterpret_cast<void *>(src);
};
template <size_t N>
void *
cbset(void *isrc, const byte in)
{
  byte *src = reinterpret_cast<byte *>(isrc);
  if ( N % 4 == 0 )
    for ( size_t n = 0; n < N; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( size_t n = 0; n < N; n++ )
      src[n] = in;
  return reinterpret_cast<void *>(src);
};

template <typename F>
inline F *
memset_32b(F *src, int in)
{
  umax_t val = (unsigned char)in * 0x0101010101010101ULL;
  uint64_t *mem = reinterpret_cast<uint64_t *>(src);
  mem[0] = val;
  mem[1] = val;
  mem[2] = val;
  mem[3] = val;
  return reinterpret_cast<F *>(src);
};

template <typename F>
inline F *
memset_64b(F *src, int in)
{
  umax_t val = (unsigned char)in * 0x0101010101010101ULL;
  uint64_t *mem = reinterpret_cast<uint64_t *>(src);

  mem[0] = val;
  mem[1] = val;
  mem[2] = val;
  mem[3] = val;
  mem[4] = val;
  mem[5] = val;
  mem[6] = val;
  mem[7] = val;
  return reinterpret_cast<F *>(src);
};

template <byte in, size_t cnt, typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
bset(F *s)
{
  byte *src = reinterpret_cast<byte *>(s);
  if constexpr ( cnt % 4 == 0 )
    for ( size_t n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

template <word in, size_t cnt>
word *
wordset(word *src)
{
  if constexpr ( cnt % 4 == 0 )
    for ( size_t n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      src[n] = (in);
  return (src);
};

template <byte in, size_t cnt, typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
memset(F *s)
{
  byte *src = reinterpret_cast<byte *>(s);
  if constexpr ( cnt % 4 == 0 )
    for ( size_t n = 0; n < cnt; n += 4 ) {
      src[n] = static_cast<F>(in);
      src[n + 1] = static_cast<F>(in);
      src[n + 2] = static_cast<F>(in);
      src[n + 3] = static_cast<F>(in);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      src[n] = static_cast<F>(in);
  return reinterpret_cast<F *>(src);
};

template <typename F>
  requires(!micron::is_null_pointer_v<F>)
F &
rmemset(F &s, const byte in, const size_t cnt)
{
  byte *src = reinterpret_cast<byte *>(&s);
  if ( cnt % 4 == 0 )
    for ( size_t n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F &>(src);
};

template <typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
memset(F *s, const byte in, const size_t cnt)
{
  byte *src = reinterpret_cast<byte *>(s);
  if ( cnt % 4 == 0 )
    for ( size_t n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

template <typename T>
  requires(!micron::is_null_pointer_v<T>)
T *
memchr(const T &restrict src, byte c, size_t n)
{
  if ( src == nullptr )
    return nullptr;
  for ( size_t i = 0; i < n; i++ )
    if ( src[i] == c )
      return const_cast<T *>(src + i);
  return nullptr;
}

template <typename T>
  requires(!micron::is_null_pointer_v<T>)
T *
memchr(const T *restrict src, byte c, size_t n)
{
  if ( src == nullptr )
    return nullptr;
  for ( size_t i = 0; i < n; i++ )
    if ( src[i] == c )
      return const_cast<T *>(src + i);
  return nullptr;
}
// compiler whines, this is correct (won't fix it)

template <typename T, typename F, typename S = size_t>
T *
_rmemcpy_32(T &restrict _d, const F &restrict _s, const S n)
{
  T *d = &_d;
  F *s = &_s;
  if ( n == 0 )
    return d;
  switch ( n ) {
  case 1:
    *d = *s;
    break;
  case 2:
    *reinterpret_cast<uint16_t *>(d) = *reinterpret_cast<const uint16_t *>(s);
    break;
  case 3:
    *reinterpret_cast<uint16_t *>(d) = *reinterpret_cast<const uint16_t *>(s);
    d[2] = s[2];
    break;
  case 4:
    *reinterpret_cast<uint32_t *>(d) = *reinterpret_cast<const uint32_t *>(s);
    break;
  case 5:
    *reinterpret_cast<uint32_t *>(d) = *reinterpret_cast<const uint32_t *>(s);
    d[4] = s[4];
    break;
  case 6:
    *reinterpret_cast<uint32_t *>(d) = *reinterpret_cast<const uint32_t *>(s);
    *reinterpret_cast<uint16_t *>(d + 4) = *reinterpret_cast<const uint16_t *>(s + 4);
    break;
  case 7:
    *reinterpret_cast<uint32_t *>(d) = *reinterpret_cast<const uint32_t *>(s);
    *reinterpret_cast<uint16_t *>(d + 4) = *reinterpret_cast<const uint16_t *>(s + 4);
    d[6] = s[6];
    break;
  case 8:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    break;
  case 9:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    d[8] = s[8];
    break;
  case 10:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint16_t *>(d + 8) = *reinterpret_cast<const uint16_t *>(s + 8);
    break;
  case 11:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint16_t *>(d + 8) = *reinterpret_cast<const uint16_t *>(s + 8);
    d[10] = s[10];
    break;
  case 12:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint32_t *>(d + 8) = *reinterpret_cast<const uint32_t *>(s + 8);
    break;
  case 13:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint32_t *>(d + 8) = *reinterpret_cast<const uint32_t *>(s + 8);
    d[12] = s[12];
    break;
  case 14:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint32_t *>(d + 8) = *reinterpret_cast<const uint32_t *>(s + 8);
    *reinterpret_cast<uint16_t *>(d + 12) = *reinterpret_cast<const uint16_t *>(s + 12);
    break;
  case 15:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint32_t *>(d + 8) = *reinterpret_cast<const uint32_t *>(s + 8);
    *reinterpret_cast<uint16_t *>(d + 12) = *reinterpret_cast<const uint16_t *>(s + 12);
    d[14] = s[14];
    break;
  case 16:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    break;
  case 17:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    d[16] = s[16];
    break;
  case 18:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint16_t *>(d + 16) = *reinterpret_cast<const uint16_t *>(s + 16);
    break;
  case 19:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint16_t *>(d + 16) = *reinterpret_cast<const uint16_t *>(s + 16);
    d[18] = s[18];
    break;
  case 20:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    break;
  case 21:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    d[20] = s[20];
    break;
  case 22:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint16_t *>(d + 20) = *reinterpret_cast<const uint16_t *>(s + 20);
    break;
  case 23:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint16_t *>(d + 20) = *reinterpret_cast<const uint16_t *>(s + 20);
    d[22] = s[22];
    break;
  case 24:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    break;
  case 25:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    d[24] = s[24];
    break;
  case 26:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint16_t *>(d + 24) = *reinterpret_cast<const uint16_t *>(s + 24);
    break;
  case 27:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint16_t *>(d + 24) = *reinterpret_cast<const uint16_t *>(s + 24);
    d[26] = s[26];
    break;
  case 28:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint32_t *>(d + 24) = *reinterpret_cast<const uint32_t *>(s + 24);
    break;
  case 29:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint32_t *>(d + 24) = *reinterpret_cast<const uint32_t *>(s + 24);
    d[28] = s[28];
    break;
  case 30:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint32_t *>(d + 24) = *reinterpret_cast<const uint32_t *>(s + 24);
    *reinterpret_cast<uint16_t *>(d + 28) = *reinterpret_cast<const uint16_t *>(s + 28);
    break;
  case 31:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint32_t *>(d + 24) = *reinterpret_cast<const uint32_t *>(s + 24);
    *reinterpret_cast<uint16_t *>(d + 28) = *reinterpret_cast<const uint16_t *>(s + 28);
    d[30] = s[30];
    break;
  case 32:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint64_t *>(d + 16) = *reinterpret_cast<const uint64_t *>(s + 16);
    *reinterpret_cast<uint64_t *>(d + 24) = *reinterpret_cast<const uint64_t *>(s + 24);
    break;
  }
  return d;
}
template <typename T, typename F, typename S = size_t>
T *
_memcpy_32(T *__restrict d, const F *__restrict s, const S n)
{
  if ( n == 0 )
    return d;
  switch ( n ) {
  case 1:
    *d = *s;
    break;
  case 2:
    *reinterpret_cast<uint16_t *>(d) = *reinterpret_cast<const uint16_t *>(s);
    break;
  case 3:
    *reinterpret_cast<uint16_t *>(d) = *reinterpret_cast<const uint16_t *>(s);
    d[2] = s[2];
    break;
  case 4:
    *reinterpret_cast<uint32_t *>(d) = *reinterpret_cast<const uint32_t *>(s);
    break;
  case 5:
    *reinterpret_cast<uint32_t *>(d) = *reinterpret_cast<const uint32_t *>(s);
    d[4] = s[4];
    break;
  case 6:
    *reinterpret_cast<uint32_t *>(d) = *reinterpret_cast<const uint32_t *>(s);
    *reinterpret_cast<uint16_t *>(d + 4) = *reinterpret_cast<const uint16_t *>(s + 4);
    break;
  case 7:
    *reinterpret_cast<uint32_t *>(d) = *reinterpret_cast<const uint32_t *>(s);
    *reinterpret_cast<uint16_t *>(d + 4) = *reinterpret_cast<const uint16_t *>(s + 4);
    d[6] = s[6];
    break;
  case 8:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    break;
  case 9:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    d[8] = s[8];
    break;
  case 10:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint16_t *>(d + 8) = *reinterpret_cast<const uint16_t *>(s + 8);
    break;
  case 11:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint16_t *>(d + 8) = *reinterpret_cast<const uint16_t *>(s + 8);
    d[10] = s[10];
    break;
  case 12:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint32_t *>(d + 8) = *reinterpret_cast<const uint32_t *>(s + 8);
    break;
  case 13:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint32_t *>(d + 8) = *reinterpret_cast<const uint32_t *>(s + 8);
    d[12] = s[12];
    break;
  case 14:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint32_t *>(d + 8) = *reinterpret_cast<const uint32_t *>(s + 8);
    *reinterpret_cast<uint16_t *>(d + 12) = *reinterpret_cast<const uint16_t *>(s + 12);
    break;
  case 15:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint32_t *>(d + 8) = *reinterpret_cast<const uint32_t *>(s + 8);
    *reinterpret_cast<uint16_t *>(d + 12) = *reinterpret_cast<const uint16_t *>(s + 12);
    d[14] = s[14];
    break;
  case 16:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    break;
  case 17:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    d[16] = s[16];
    break;
  case 18:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint16_t *>(d + 16) = *reinterpret_cast<const uint16_t *>(s + 16);
    break;
  case 19:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint16_t *>(d + 16) = *reinterpret_cast<const uint16_t *>(s + 16);
    d[18] = s[18];
    break;
  case 20:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    break;
  case 21:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    d[20] = s[20];
    break;
  case 22:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint16_t *>(d + 20) = *reinterpret_cast<const uint16_t *>(s + 20);
    break;
  case 23:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint16_t *>(d + 20) = *reinterpret_cast<const uint16_t *>(s + 20);
    d[22] = s[22];
    break;
  case 24:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    break;
  case 25:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    d[24] = s[24];
    break;
  case 26:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint16_t *>(d + 24) = *reinterpret_cast<const uint16_t *>(s + 24);
    break;
  case 27:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint16_t *>(d + 24) = *reinterpret_cast<const uint16_t *>(s + 24);
    d[26] = s[26];
    break;
  case 28:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint32_t *>(d + 24) = *reinterpret_cast<const uint32_t *>(s + 24);
    break;
  case 29:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint32_t *>(d + 24) = *reinterpret_cast<const uint32_t *>(s + 24);
    d[28] = s[28];
    break;
  case 30:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint32_t *>(d + 24) = *reinterpret_cast<const uint32_t *>(s + 24);
    *reinterpret_cast<uint16_t *>(d + 28) = *reinterpret_cast<const uint16_t *>(s + 28);
    break;
  case 31:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint32_t *>(d + 16) = *reinterpret_cast<const uint32_t *>(s + 16);
    *reinterpret_cast<uint32_t *>(d + 24) = *reinterpret_cast<const uint32_t *>(s + 24);
    *reinterpret_cast<uint16_t *>(d + 28) = *reinterpret_cast<const uint16_t *>(s + 28);
    d[30] = s[30];
    break;
  case 32:
    *reinterpret_cast<uint64_t *>(d) = *reinterpret_cast<const uint64_t *>(s);
    *reinterpret_cast<uint64_t *>(d + 8) = *reinterpret_cast<const uint64_t *>(s + 8);
    *reinterpret_cast<uint64_t *>(d + 16) = *reinterpret_cast<const uint64_t *>(s + 16);
    *reinterpret_cast<uint64_t *>(d + 24) = *reinterpret_cast<const uint64_t *>(s + 24);
    break;
  }
  return d;
}
template <size_t M, typename F, typename D>
F *
cmemcpy(F *__restrict dest, const D *__restrict src)
{
  if constexpr ( M % 4 == 0 )
    for ( size_t n = 0; n < M; n += 4 ) {
      dest[n] = static_cast<F>(src[n]);
      dest[n + 1] = static_cast<F>(src[n + 1]);
      dest[n + 2] = static_cast<F>(src[n + 2]);
      dest[n + 3] = static_cast<F>(src[n + 3]);
    }
  else if constexpr ( M % 4 != 0 )
    for ( size_t n = 0; n < M; n++ )
      dest[n] = static_cast<F>(src[n]);
  return reinterpret_cast<F *>(dest);
};

template <size_t M, typename F, typename D>
F &
crmemcpy(F &restrict dest, const D &restrict src)
{
  if constexpr ( M % 4 == 0 )
    for ( size_t n = 0; n < M; n += 4 ) {
      dest[n] = static_cast<F>(src[n]);
      dest[n + 1] = static_cast<F>(src[n + 1]);
      dest[n + 2] = static_cast<F>(src[n + 2]);
      dest[n + 3] = static_cast<F>(src[n + 3]);
    }
  else if constexpr ( M % 4 != 0 )
    for ( size_t n = 0; n < M; n++ )
      dest[n] = static_cast<F>(src[n]);
  return reinterpret_cast<F &>(dest);
};

template <typename F, typename D>
F &
rmemcpy(F &restrict dest, const D &restrict src, const size_t cnt)
{
  if ( cnt % 4 == 0 ) [[likely]]
    for ( size_t n = 0; n < cnt; n += 4 ) {
      dest[n] = static_cast<F>(src[n]);
      dest[n + 1] = static_cast<F>(src[n + 1]);
      dest[n + 2] = static_cast<F>(src[n + 2]);
      dest[n + 3] = static_cast<F>(src[n + 3]);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      dest[n] = static_cast<F>(src[n]);
  return reinterpret_cast<F &>(dest);
};

template <typename F, typename D>
F *
memcpy(F *restrict dest, const D *restrict src, const size_t cnt)
{
  if ( cnt % 4 == 0 ) [[likely]]
    for ( size_t n = 0; n < cnt; n += 4 ) {
      dest[n] = static_cast<F>(src[n]);
      dest[n + 1] = static_cast<F>(src[n + 1]);
      dest[n + 2] = static_cast<F>(src[n + 2]);
      dest[n + 3] = static_cast<F>(src[n + 3]);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      dest[n] = static_cast<F>(src[n]);
  return reinterpret_cast<F *>(dest);
};

template <size_t M, typename F, typename D>
F *
cbytecpy(F *restrict _dest, const D *restrict _src)
{
  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);
  if constexpr ( M % 4 == 0 )
    for ( size_t n = 0; n < M; n += 4 ) {
      dest[n] = (src[n]);
      dest[n + 1] = (src[n + 1]);
      dest[n + 2] = (src[n + 2]);
      dest[n + 3] = (src[n + 3]);
    }
  else if constexpr ( M % 4 != 0 )
    for ( size_t n = 0; n < M; n++ )
      dest[n] = (src[n]);
  return reinterpret_cast<F *>(dest);
};
template <typename F, typename D>
F *
bytecpy(F *restrict _dest, const D *restrict _src, const size_t cnt)
{
  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);
  if ( cnt % 4 == 0 ) [[likely]]
    for ( size_t n = 0; n < cnt; n += 4 ) {
      dest[n] = (src[n]);
      dest[n + 1] = (src[n + 1]);
      dest[n + 2] = (src[n + 2]);
      dest[n + 3] = (src[n + 3]);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      dest[n] = (src[n]);
  return reinterpret_cast<F *>(dest);
};
template <typename F, typename D>
F &
rbytecpy(F &restrict _dest, const D &restrict _src, const size_t cnt)
{
  byte *dest = reinterpret_cast<byte *>(&_dest);
  const byte *src = reinterpret_cast<const byte *>(&_src);
  if ( cnt % 4 == 0 ) [[likely]]
    for ( size_t n = 0; n < cnt; n += 4 ) {
      dest[n] = (src[n]);
      dest[n + 1] = (src[n + 1]);
      dest[n + 2] = (src[n + 2]);
      dest[n + 3] = (src[n + 3]);
    }
  else
    for ( size_t n = 0; n < cnt; n++ )
      dest[n] = (src[n]);
  return reinterpret_cast<F &>(dest);
};

template <typename F, typename D>
F *
memcpy128(F *restrict dest, D *restrict src, const size_t cnt)
{
  for ( size_t n = 0; n < (cnt / (size_t)sizeof(__m128i)); n++ ) {
    __m128i pkt = _mm_loadu_si128(reinterpret_cast<__m128i *>(&src[n]));
    _mm_storeu_si128(reinterpret_cast<__m128i *>(&dest[n]), pkt);
  }
  return reinterpret_cast<F *>(dest);
};

template <typename F, typename D>
F &
rmemcpy128(F &restrict dest, D &restrict src, const size_t cnt)
{
  for ( size_t n = 0; n < (cnt / (size_t)sizeof(__m128i)); n++ ) {
    __m128i pkt = _mm_loadu_si128(reinterpret_cast<__m128i *>(&src[n]));
    _mm_storeu_si128(reinterpret_cast<__m128i *>(&dest[n]), pkt);
  }
  return reinterpret_cast<F &>(dest);
};

template <typename F, typename D>
F *
memcpy256(F *restrict dest, D *restrict src, const size_t cnt)
{
  for ( size_t n = 0; n < (cnt / (size_t)sizeof(__m256i)); n++ ) {
    __m256i pkt = _mm256_loadu_si256(reinterpret_cast<__m256i *>(&src[n]));
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(&dest[n]), pkt);
  }
  return reinterpret_cast<F *>(dest);
};

template <typename F, typename D>
F &
rmemcpy256(F &restrict dest, D &restrict src, const size_t cnt)
{
  for ( size_t n = 0; n < (cnt / (size_t)sizeof(__m256i)); n++ ) {
    __m256i pkt = _mm256_loadu_si256(reinterpret_cast<__m256i *>(&src[n]));
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(&dest[n]), pkt);
  }
  return reinterpret_cast<F &>(dest);
};

template <typename F, typename D>
F *
bytemove(F *_dest, D *_src, const size_t cnt)
{
  byte *dest = reinterpret_cast<byte *>(_dest);
  byte *src = reinterpret_cast<byte *>(_src);
  if ( dest < src )
    for ( size_t i = 0; i < cnt; i++ )
      dest[i] = src[i];
  else
    for ( size_t i = cnt; i > 0; --i )
      dest[i - 1] = src[i - 1];
  return reinterpret_cast<F *>(dest);
};

template <typename F, typename D>
F &
rbytemove(F &_dest, D &_src, const size_t cnt)
{
  byte *dest = reinterpret_cast<byte *>(&_dest);
  byte *src = reinterpret_cast<byte *>(&_src);
  if ( dest < src )
    for ( size_t i = 0; i < cnt; i++ )
      dest[i] = src[i];
  else
    for ( size_t i = cnt; i > 0; --i )
      dest[i - 1] = src[i - 1];
  return reinterpret_cast<F *>(dest);
};
// NOTE: UNLIKE DEFAULT MEMMOVE THIS OPERATES ON THE SIZE TYPE OF THE UNDERLYING DATA, FOR "NORMAL" MEMMOVE USE BYTEMOVE
template <typename F, typename D>
F *
memmove(F *dest, D *src, const size_t cnt)
{
  if ( dest < src )
    for ( size_t i = 0; i < cnt; i++ )
      dest[i] = src[i];
  else
    for ( size_t i = cnt; i > 0; --i )
      dest[i - 1] = src[i - 1];
  return reinterpret_cast<F *>(dest);
};
template <typename F, typename D>
F *
memmove128(F *dest, D *src, const size_t cnt)
{
  if ( dest < src || dest >= src + cnt ) {
    for ( size_t i = 0; i <= cnt; i += 16 ) {
      __m128i pkt = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + i), pkt);
    }
  } else {
    size_t i = 0;
    while ( i >= 16 ) {
      i -= 16;
      __m128i pkt = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(src + i), pkt);
    }
  }
  return reinterpret_cast<F *>(dest);
};

};     // namespace micron
