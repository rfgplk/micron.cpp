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

#include "bits.hpp"

namespace micron
{

// START MEMSET

// BASIC MEMSET - RUNTIME COUNT
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
memset(F *s, const byte in, const u64 cnt) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// MEMSET WITH REFERENCE RETURN
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
F &
rmemset(F &s, const byte in, const u64 cnt) noexcept
{
  byte *src = reinterpret_cast<byte *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return s;
};

// CONSTEXPR MEMSET
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
constexpr F *
constexpr_memset(F *s, const byte in, const u64 cnt) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// COMPILE-TIME CONSTANT MEMSET - TEMPLATE COUNT AND VALUE
template <byte in, u64 cnt, typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
memset(F *s) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  if constexpr ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// COMPILE-TIME CONSTANT MEMSET - TEMPLATE COUNT ONLY
template <u64 M, typename F>
F *
cmemset(F *s, const byte in) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// COMPILE-TIME CONSTANT MEMSET WITH REFERENCE RETURN
template <u64 M, typename F>
F &
rcmemset(F &s, const byte in) noexcept
{
  byte *src = reinterpret_cast<byte *>(&s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return s;
};

// SECURE COMPILE-TIME CONSTANT MEMSET
template <u64 M, typename F>
F *
scmemset(F *s, const byte in) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  __mem_barrier();
  return s;
};

// SECURE COMPILE-TIME CONSTANT MEMSET WITH REFERENCE RETURN
template <u64 M, typename F>
F &
rscmemset(F &s, const byte in) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(&s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  __mem_barrier();
  return s;
};

// SIMD MEMSET VARIANTS, REQUIRES ALIGNMENT
template <typename F, typename N = byte>
F *
memset256(F *src, N in, const u64 cnt) noexcept
{
  __m256i v = _mm256_set1_epi32(in);
  for ( u64 n = 0; n < cnt; n += 8 ) {
    _mm256_store_si256(reinterpret_cast<__m256i *>(src + n), v);
  }
  return reinterpret_cast<F *>(src);
};

template <typename F, typename N = byte>
F *
memset128(F *src, N in, const u64 cnt) noexcept
{
  __m128i v = _mm_set1_epi32(in);
  for ( u64 n = 0; n < cnt; n += 4 ) {
    _mm_store_si128(reinterpret_cast<__m128i *>(src + n), v);
  }
  return reinterpret_cast<F *>(src);
};

// MANUALLY UNROLLED MEMSET - BE CAREFUL!
template <typename F>
inline F *
memset_8b(F *src, int in) noexcept
{
  umax_t val = broadcast_byte(static_cast<byte>(in));
  u64 *mem = reinterpret_cast<u64 *>(src);
  mem[0] = val;
  return reinterpret_cast<F *>(src);
};

template <typename F>
inline F *
memset_16b(F *src, int in) noexcept
{
  umax_t val = broadcast_byte(static_cast<byte>(in));
  u64 *mem = reinterpret_cast<u64 *>(src);
  mem[0] = val;
  mem[1] = val;
  return reinterpret_cast<F *>(src);
};

template <typename F>
inline F *
memset_32b(F *src, int in) noexcept
{
  umax_t val = broadcast_byte(static_cast<byte>(in));
  u64 *mem = reinterpret_cast<u64 *>(src);
  mem[0] = val;
  mem[1] = val;
  mem[2] = val;
  mem[3] = val;
  return reinterpret_cast<F *>(src);
};

template <typename F>
inline F *
memset_64b(F *src, int in) noexcept
{
  umax_t val = broadcast_byte(static_cast<byte>(in));
  u64 *mem = reinterpret_cast<u64 *>(src);
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

// SAFE MEMSET WITH NULLPTR AND ALIGNMENT CHECKING - RUNTIME COUNT
template <typename F, u64 alignment = alignof(F)>
  requires(!micron::is_null_pointer_v<F>)
F *
smemset(F *s, const byte in, const u64 cnt) noexcept
{
  if ( s == nullptr )
    return nullptr;
  if ( !__is_aligned_to(s, alignment) )
    return nullptr;
  if ( !__is_valid_address(s, cnt) )
    return nullptr;
  byte *src = reinterpret_cast<byte *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// SAFE MEMSET WITH REFERENCE RETURN
template <typename F, u64 alignment = alignof(F)>
  requires(!micron::is_null_pointer_v<F>)
bool
rsmemset(F &s, const byte in, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(s, alignment) )
    return false;
  if ( !__is_valid_address(s, cnt) )
    return false;

  byte *src = reinterpret_cast<byte *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return true;
};

// SAFE COMPILE-TIME CONSTANT MEMSET - TEMPLATE COUNT ONLY
template <u64 M, typename F, u64 alignment = alignof(F)>
F *
scmemset_safe(F *s, const byte in) noexcept
{
  if ( s == nullptr )
    return nullptr;
  if ( !__is_aligned_to(s, alignment) )
    return nullptr;
  if ( !__is_valid_address(s, M) )
    return nullptr;

  byte *src = reinterpret_cast<byte *>(s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// SAFE COMPILE-TIME CONSTANT MEMSET WITH REFERENCE RETURN
template <u64 M, typename F, u64 alignment = alignof(F)>
bool
rscmemset_safe(F &s, const byte in) noexcept
{
  if ( !__is_aligned_to(s, alignment) )
    return false;
  if ( !__is_valid_address(s, M) )
    return nullptr;

  byte *src = reinterpret_cast<byte *>(&s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return true;
};

// START BYTESET

// BASIC BYTESET - RUNTIME COUNT
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
byteset(F *s, const byte in, const u64 cnt) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = in;
  return reinterpret_cast<F *>(src);
};

// BYTESET ALIAS (BSET)
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
bset(F *s, const byte in, const u64 cnt) noexcept
{
  return byteset(s, in, cnt);
};

// BYTESET WITH REFERENCE RETURN
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
F &
rbyteset(F &s, const byte in, const u64 cnt) noexcept
{
  byte *src = reinterpret_cast<byte *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = in;
  return s;
};

// BYTESET ALIAS WITH REFERENCE RETURN
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
F &
rbset(F &s, const byte in, const u64 cnt) noexcept
{
  return rbyteset(s, in, cnt);
};

// COMPILE-TIME CONSTANT BYTESET - TEMPLATE COUNT ONLY
template <u64 N, typename F>
F *
cbyteset(F *s, const byte in) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  if constexpr ( N % 4 == 0 )
    for ( u64 n = 0; n < N; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < N; n++ )
      src[n] = in;
  return reinterpret_cast<F *>(src);
};

// COMPILE-TIME CONSTANT BYTESET ALIAS (CBSET)
template <u64 N, typename F>
F *
cbset(F *s, const byte in) noexcept
{
  return cbyteset<N, F>(s, in);
};

// COMPILE-TIME CONSTANT BYTESET WITH REFERENCE RETURN
template <u64 N, typename F>
F &
rcbyteset(F &s, const byte in) noexcept
{
  byte *src = reinterpret_cast<byte *>(&s);
  if constexpr ( N % 4 == 0 )
    for ( u64 n = 0; n < N; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < N; n++ )
      src[n] = in;
  return s;
};

// COMPILE-TIME CONSTANT BYTESET ALIAS WITH REFERENCE RETURN
template <u64 N, typename F>
F &
rcbset(F &s, const byte in) noexcept
{
  return rcbyteset<N, F>(s, in);
};

// SECURE COMPILE-TIME CONSTANT BYTESET
template <u64 N, typename F>
F *
scbyteset(F *s, const byte in) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(s);
  if constexpr ( N % 4 == 0 )
    for ( u64 n = 0; n < N; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < N; n++ )
      src[n] = in;
  __mem_barrier();
  return s;
};

// SECURE COMPILE-TIME CONSTANT BYTESET ALIAS
template <u64 N, typename F>
F *
scbset(F *s, const byte in) noexcept
{
  return scbyteset<N, F>(s, in);
};

// SECURE COMPILE-TIME CONSTANT BYTESET WITH REFERENCE RETURN
template <u64 N, typename F>
F &
rscbyteset(F &s, const byte in) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(&s);
  if constexpr ( N % 4 == 0 )
    for ( u64 n = 0; n < N; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < N; n++ )
      src[n] = in;
  __mem_barrier();
  return s;
};

// SECURE COMPILE-TIME CONSTANT BYTESET ALIAS WITH REFERENCE RETURN
template <u64 N, typename F>
F &
rscbset(F &s, const byte in) noexcept
{
  return rscbyteset<N, F>(s, in);
};

// COMPILE-TIME CONSTANT BYTESET - TEMPLATE COUNT AND VALUE
template <byte in, u64 cnt, typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
byteset(F *s) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  if constexpr ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// COMPILE-TIME CONSTANT BYTESET ALIAS (BSET) - TEMPLATE COUNT AND VALUE
template <byte in, u64 cnt, typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
bset(F *s) noexcept
{
  return byteset<in, cnt, F>(s);
};

// MANUALLY UNROLLED BYTESET - 8 BYTES
template <typename F>
inline F *
byteset_8b(F *src, byte in) noexcept
{
  umax_t val = broadcast_byte(static_cast<byte>(in));
  u64 *mem = reinterpret_cast<u64 *>(src);
  mem[0] = val;
  return reinterpret_cast<F *>(src);
};

// MANUALLY UNROLLED BYTESET ALIAS - 8 BYTES
template <typename F>
inline F *
bset_8b(F *src, byte in) noexcept
{
  return byteset_8b(src, in);
};

// MANUALLY UNROLLED BYTESET - 16 BYTES
template <typename F>
inline F *
byteset_16b(F *src, byte in) noexcept
{
  umax_t val = broadcast_byte(static_cast<byte>(in));
  u64 *mem = reinterpret_cast<u64 *>(src);
  mem[0] = val;
  mem[1] = val;
  return reinterpret_cast<F *>(src);
};

// MANUALLY UNROLLED BYTESET ALIAS - 16 BYTES
template <typename F>
inline F *
bset_16b(F *src, byte in) noexcept
{
  return byteset_16b(src, in);
};

// MANUALLY UNROLLED BYTESET - 32 BYTES
template <typename F>
inline F *
byteset_32b(F *src, byte in) noexcept
{
  umax_t val = broadcast_byte(static_cast<byte>(in));
  u64 *mem = reinterpret_cast<u64 *>(src);
  mem[0] = val;
  mem[1] = val;
  mem[2] = val;
  mem[3] = val;
  return reinterpret_cast<F *>(src);
};

// MANUALLY UNROLLED BYTESET ALIAS - 32 BYTES
template <typename F>
inline F *
bset_32b(F *src, byte in) noexcept
{
  return byteset_32b(src, in);
};

// MANUALLY UNROLLED BYTESET - 64 BYTES
template <typename F>
inline F *
byteset_64b(F *src, byte in) noexcept
{
  umax_t val = broadcast_byte(static_cast<byte>(in));
  u64 *mem = reinterpret_cast<u64 *>(src);
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

// MANUALLY UNROLLED BYTESET ALIAS - 64 BYTES
template <typename F>
inline F *
bset_64b(F *src, byte in) noexcept
{
  return byteset_64b(src, in);
};

// SAFE BYTESET - RUNTIME COUNT
template <typename F, u64 alignment = 1>
  requires(!micron::is_null_pointer_v<F>)
F *
sbyteset(F *s, const byte in, const u64 cnt) noexcept
{
  if ( s == nullptr )
    return nullptr;
  if ( !__is_aligned_to(s, alignment) )
    return nullptr;
  if ( !__is_valid_address(s, cnt) )
    return nullptr;

  byte *src = reinterpret_cast<byte *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = in;
  return reinterpret_cast<F *>(src);
};

// SAFE BYTESET ALIAS (SBSET)
template <typename F, u64 alignment = 1>
  requires(!micron::is_null_pointer_v<F>)
F *
sbset(F *s, const byte in, const u64 cnt) noexcept
{
  return sbyteset<F, alignment>(s, in, cnt);
};

// SAFE BYTESET WITH REFERENCE RETURN
template <typename F, u64 alignment = 1>
  requires(!micron::is_null_pointer_v<F>)
bool
rsbyteset(F &s, const byte in, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(s, alignment) )
    return false;
  if ( !__is_valid_address(s, cnt) )
    return nullptr;

  byte *src = reinterpret_cast<byte *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = in;
  return true;
};

// SAFE BYTESET ALIAS WITH REFERENCE RETURN
template <typename F, u64 alignment = 1>
  requires(!micron::is_null_pointer_v<F>)
bool
rsbset(F &s, const byte in, const u64 cnt) noexcept
{
  return rsbyteset<F, alignment>(s, in, cnt);
};

// SAFE COMPILE-TIME CONSTANT BYTESET - TEMPLATE COUNT ONLY
template <u64 N, typename F, u64 alignment = 1>
F *
scbyteset_safe(F *s, const byte in) noexcept
{
  if ( s == nullptr )
    return nullptr;
  if ( !__is_aligned_to(s, alignment) )
    return nullptr;
  if ( !__is_valid_address(s, N) )
    return nullptr;

  byte *src = reinterpret_cast<byte *>(s);
  if constexpr ( N % 4 == 0 )
    for ( u64 n = 0; n < N; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < N; n++ )
      src[n] = in;
  return reinterpret_cast<F *>(src);
};

// SAFE COMPILE-TIME CONSTANT BYTESET ALIAS (SCBSET_SAFE)
template <u64 N, typename F, u64 alignment = 1>
F *
scbset_safe(F *s, const byte in) noexcept
{
  return scbyteset_safe<N, F, alignment>(s, in);
};

// SAFE COMPILE-TIME CONSTANT BYTESET WITH REFERENCE RETURN
template <u64 N, typename F, u64 alignment = 1>
bool
rscbyteset_safe(F &s, const byte in) noexcept
{
  if ( !__is_aligned_to(s, alignment) )
    return false;
  if ( !__is_valid_address(s, N) )
    return nullptr;

  byte *src = reinterpret_cast<byte *>(&s);
  if constexpr ( N % 4 == 0 )
    for ( u64 n = 0; n < N; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < N; n++ )
      src[n] = in;
  return true;
};

// SAFE COMPILE-TIME CONSTANT BYTESET ALIAS WITH REFERENCE RETURN
template <u64 N, typename F, u64 alignment = 1>
bool
rscbset_safe(F &s, const byte in) noexcept
{
  return rscbyteset_safe<N, F, alignment>(s, in);
};

// BYTE-LEVEL OPERATIONS ON TYPED POINTERS - RUNTIME COUNT
template <typename M = u64>
byte *
bzero(byte *src, const M cnt) noexcept
{
  memset<byte>(src, 0x0, cnt);
  return src;
};

// BYTE-LEVEL ZERO - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
  requires micron::is_fundamental_v<F>
constexpr F &
cbzero(F &_src) noexcept
{
  byte *src = reinterpret_cast<byte *>(&_src);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x0;
  return _src;
};

// BYTE-LEVEL ZERO - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
  requires micron::is_fundamental_v<F>
constexpr F *
cbzero(F *_src) noexcept
{
  byte *src = reinterpret_cast<byte *>(_src);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x0;
  return reinterpret_cast<F *>(src);
};

// SECURE BYTE-LEVEL ZERO - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
  requires micron::is_fundamental_v<F>
constexpr F &
scbzero(F &_src) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(&_src);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x0;
  __mem_barrier();
  return _src;
};

// SECURE BYTE-LEVEL ZERO - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
  requires micron::is_fundamental_v<F>
constexpr F *
scbzero(F *_src) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(_src);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x0;
  __mem_barrier();
  return _src;
};

// START TYPESET

// BASIC TYPESET - RUNTIME COUNT
template <typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
F *
typeset(F *s, const T in, const u64 cnt) noexcept
{
  T *src = reinterpret_cast<T *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// TYPESET WITH REFERENCE RETURN
template <typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
F &
rtypeset(F &s, const T in, const u64 cnt) noexcept
{
  T *src = reinterpret_cast<T *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return s;
};

// COMPILE-TIME CONSTANT TYPESET - TEMPLATE COUNT ONLY
template <u64 M, typename T, typename F>
F *
ctypeset(F *s, const T in) noexcept
{
  T *src = reinterpret_cast<T *>(s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// COMPILE-TIME CONSTANT TYPESET WITH REFERENCE RETURN
template <u64 M, typename T, typename F>
F &
rctypeset(F &s, const T in) noexcept
{
  T *src = reinterpret_cast<T *>(&s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return s;
};

// SECURE COMPILE-TIME CONSTANT TYPESET
template <u64 M, typename T, typename F>
F *
sctypeset(F *s, const T in) noexcept
{
  volatile T *src = reinterpret_cast<volatile T *>(s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  __mem_barrier();
  return s;
};

// SECURE COMPILE-TIME CONSTANT TYPESET WITH REFERENCE RETURN
template <u64 M, typename T, typename F>
F &
rsctypeset(F &s, const T in) noexcept
{
  volatile T *src = reinterpret_cast<volatile T *>(&s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  __mem_barrier();
  return s;
};

// SAFE TYPESET - RUNTIME COUNT
template <typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
F *
stypeset(F *s, const T in, const u64 cnt) noexcept
{
  if ( s == nullptr )
    return nullptr;
  if ( !__is_aligned_to(s, alignment) )
    return nullptr;
  if ( !__is_valid_address(s, cnt) )
    return nullptr;

  T *src = reinterpret_cast<T *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// SAFE TYPESET WITH REFERENCE RETURN
template <typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
bool
rstypeset(F &s, const T in, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(s, alignment) )
    return false;
  if ( !__is_valid_address(s, cnt) )
    return nullptr;

  T *src = reinterpret_cast<T *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return true;
};

// SAFE COMPILE-TIME CONSTANT TYPESET - TEMPLATE COUNT ONLY
template <u64 M, typename T, typename F, u64 alignment = alignof(T)>
F *
sctypeset_safe(F *s, const T in) noexcept
{
  if ( s == nullptr )
    return nullptr;
  if ( !__is_aligned_to(s, alignment) )
    return nullptr;
  if ( !__is_valid_address(s, M) )
    return nullptr;

  T *src = reinterpret_cast<T *>(s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return reinterpret_cast<F *>(src);
};

// SAFE COMPILE-TIME CONSTANT TYPESET WITH REFERENCE RETURN
template <u64 M, typename T, typename F, u64 alignment = alignof(T)>
bool
rsctypeset_safe(F &s, const T in) noexcept
{
  if ( !__is_aligned_to(s, alignment) )
    return false;
  if ( !__is_valid_address(s, M) )
    return nullptr;

  T *src = reinterpret_cast<T *>(&s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return true;
};

// START WORDSET

// BASIC WORDSET - RUNTIME COUNT
word *
wordset(word *src, const word in, const u64 cnt) noexcept
{
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return src;
};

// WORDSET WITH REFERENCE RETURN
word &
rwordset(word &s, const word in, const u64 cnt) noexcept
{
  word *src = reinterpret_cast<word *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return s;
};

// COMPILE-TIME CONSTANT WORDSET - TEMPLATE COUNT ONLY
template <u64 M>
word *
cwordset(word *src, const word in) noexcept
{
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return src;
};

// COMPILE-TIME CONSTANT WORDSET WITH REFERENCE RETURN
template <u64 M>
word &
rcwordset(word &s, const word in) noexcept
{
  word *src = reinterpret_cast<word *>(&s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return s;
};

// SECURE COMPILE-TIME CONSTANT WORDSET
template <u64 M>
word *
scwordset(word *src, const word in) noexcept
{
  volatile word *vsrc = reinterpret_cast<volatile word *>(src);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      vsrc[n] = (in);
      vsrc[n + 1] = (in);
      vsrc[n + 2] = (in);
      vsrc[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      vsrc[n] = (in);
  __mem_barrier();
  return src;
};

// SECURE COMPILE-TIME CONSTANT WORDSET WITH REFERENCE RETURN
template <u64 M>
word &
rscwordset(word &s, const word in) noexcept
{
  volatile word *src = reinterpret_cast<volatile word *>(&s);
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  __mem_barrier();
  return s;
};

// COMPILE-TIME CONSTANT WORDSET - TEMPLATE COUNT AND VALUE
template <word in, u64 cnt>
word *
wordset(word *src) noexcept
{
  if constexpr ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return src;
};

// MANUALLY UNROLLED WORDSET - 4 WORDS (8 BYTES ON 16-BIT, 16 BYTES ON 32-BIT, 32 BYTES ON 64-BIT)
template <word in>
inline word *
wordset_4w(word *src) noexcept
{
  src[0] = in;
  src[1] = in;
  src[2] = in;
  src[3] = in;
  return src;
};

// MANUALLY UNROLLED WORDSET - 8 WORDS
template <word in>
inline word *
wordset_8w(word *src) noexcept
{
  src[0] = in;
  src[1] = in;
  src[2] = in;
  src[3] = in;
  src[4] = in;
  src[5] = in;
  src[6] = in;
  src[7] = in;
  return src;
};

// MANUALLY UNROLLED WORDSET - 16 WORDS
template <word in>
inline word *
wordset_16w(word *src) noexcept
{
  src[0] = in;
  src[1] = in;
  src[2] = in;
  src[3] = in;
  src[4] = in;
  src[5] = in;
  src[6] = in;
  src[7] = in;
  src[8] = in;
  src[9] = in;
  src[10] = in;
  src[11] = in;
  src[12] = in;
  src[13] = in;
  src[14] = in;
  src[15] = in;
  return src;
};

// MANUALLY UNROLLED WORDSET - 32 WORDS
template <word in>
inline word *
wordset_32w(word *src) noexcept
{
  src[0] = in;
  src[1] = in;
  src[2] = in;
  src[3] = in;
  src[4] = in;
  src[5] = in;
  src[6] = in;
  src[7] = in;
  src[8] = in;
  src[9] = in;
  src[10] = in;
  src[11] = in;
  src[12] = in;
  src[13] = in;
  src[14] = in;
  src[15] = in;
  src[16] = in;
  src[17] = in;
  src[18] = in;
  src[19] = in;
  src[20] = in;
  src[21] = in;
  src[22] = in;
  src[23] = in;
  src[24] = in;
  src[25] = in;
  src[26] = in;
  src[27] = in;
  src[28] = in;
  src[29] = in;
  src[30] = in;
  src[31] = in;
  return src;
};

// SAFE WORDSET - RUNTIME COUNT
template <u64 alignment = alignof(word)>
word *
swordset(word *src, const word in, const u64 cnt) noexcept
{
  if ( src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(src, alignment) )
    return nullptr;
  if ( !__is_valid_address(src, cnt) )
    return nullptr;

  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return src;
};

// SAFE WORDSET WITH REFERENCE RETURN
template <u64 alignment = alignof(word)>
bool
rswordset(word &s, const word in, const u64 cnt) noexcept
{
  if ( !__is_aligned_to_r(s, alignment) )
    return false;
  if ( !__is_valid_address(s, cnt) )
    return nullptr;

  word *src = reinterpret_cast<word *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      src[n] = (in);
  return true;
};

// SAFE COMPILE-TIME CONSTANT WORDSET - TEMPLATE COUNT ONLY
template <u64 M, u64 alignment = alignof(word)>
word *
scwordset_safe(word *src, const word in) noexcept
{
  if ( src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(src, alignment) )
    return nullptr;
  if ( !__is_valid_address(src, M) )
    return nullptr;

  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      src[n] = (in);
      src[n + 1] = (in);
      src[n + 2] = (in);
      src[n + 3] = (in);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      src[n] = (in);
  return src;
};

// START BITWISE

// ZERO - RUNTIME COUNT FOR POINTERS
template <typename F, typename M = u64>
F *
zero(F *src, const M cnt) noexcept
{
  memset(src, 0x0, cnt);
  return reinterpret_cast<F *>(src);
};

template <typename F, typename M = u64>
F &
rzero(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] = 0x0;
  return s;
};

// ZERO - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
  requires micron::is_fundamental_v<F>
constexpr F &
czero(F &src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x0;
  return src;
};

// ZERO - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
  requires micron::is_fundamental_v<F>
constexpr F *
czero(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x0;
  return reinterpret_cast<F *>(src);
};

// SECURE ZERO - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
  requires micron::is_fundamental_v<F>
constexpr F &
sczero(F &s) noexcept
{
  volatile F *src = reinterpret_cast<volatile F *>(&s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x0;
  __mem_barrier();
  return s;
};

// SECURE ZERO - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
  requires micron::is_fundamental_v<F>
constexpr F *
sczero(F *s) noexcept
{
  volatile F *src = reinterpret_cast<volatile F *>(s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x0;
  __mem_barrier();
  return s;
};

// MANUALLY UNROLLED ZERO - 4 ELEMENTS
template <typename F>
inline F *
zero_4(F *src) noexcept
{
  src[0] = 0x0;
  src[1] = 0x0;
  src[2] = 0x0;
  src[3] = 0x0;
  return src;
};

// MANUALLY UNROLLED ZERO - 8 ELEMENTS
template <typename F>
inline F *
zero_8(F *src) noexcept
{
  src[0] = 0x0;
  src[1] = 0x0;
  src[2] = 0x0;
  src[3] = 0x0;
  src[4] = 0x0;
  src[5] = 0x0;
  src[6] = 0x0;
  src[7] = 0x0;
  return src;
};

// MANUALLY UNROLLED ZERO - 16 ELEMENTS
template <typename F>
inline F *
zero_16(F *src) noexcept
{
  src[0] = 0x0;
  src[1] = 0x0;
  src[2] = 0x0;
  src[3] = 0x0;
  src[4] = 0x0;
  src[5] = 0x0;
  src[6] = 0x0;
  src[7] = 0x0;
  src[8] = 0x0;
  src[9] = 0x0;
  src[10] = 0x0;
  src[11] = 0x0;
  src[12] = 0x0;
  src[13] = 0x0;
  src[14] = 0x0;
  src[15] = 0x0;
  return src;
};

// MANUALLY UNROLLED ZERO - 32 ELEMENTS
template <typename F>
inline F *
zero_32(F *src) noexcept
{
  src[0] = 0x0;
  src[1] = 0x0;
  src[2] = 0x0;
  src[3] = 0x0;
  src[4] = 0x0;
  src[5] = 0x0;
  src[6] = 0x0;
  src[7] = 0x0;
  src[8] = 0x0;
  src[9] = 0x0;
  src[10] = 0x0;
  src[11] = 0x0;
  src[12] = 0x0;
  src[13] = 0x0;
  src[14] = 0x0;
  src[15] = 0x0;
  src[16] = 0x0;
  src[17] = 0x0;
  src[18] = 0x0;
  src[19] = 0x0;
  src[20] = 0x0;
  src[21] = 0x0;
  src[22] = 0x0;
  src[23] = 0x0;
  src[24] = 0x0;
  src[25] = 0x0;
  src[26] = 0x0;
  src[27] = 0x0;
  src[28] = 0x0;
  src[29] = 0x0;
  src[30] = 0x0;
  src[31] = 0x0;
  return src;
};

// FILL WITH 0XFF - RUNTIME COUNT FOR POINTERS
template <typename F, typename M = u64>
constexpr F *
full(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0xFF;
  return reinterpret_cast<F *>(src);
};

// FILL WITH 0XFF - REFERENCE RETURN
template <typename F, typename M = u64>
constexpr F &
rfull(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] = 0xFF;
  return s;
};

// FILL WITH 0XFF - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcfull(F &s) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] = 0xFF;
  return s;
};

template <u64 M, typename F>
constexpr F *
cfull(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0xFF;
  return reinterpret_cast<F *>(src);
};

// SECURE FILL WITH 0XFF - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
scfull(F *s) noexcept
{
  volatile F *src = reinterpret_cast<volatile F *>(s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0xFF;
  __mem_barrier();
  return s;
};

// SECURE FILL WITH 0XFF - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rscfull(F &s) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] = 0xFF;
  __mem_barrier();
  return s;
};

// MANUALLY UNROLLED FULL - 4 ELEMENTS
template <typename F>
inline F *
full_4(F *src) noexcept
{
  src[0] = 0xFF;
  src[1] = 0xFF;
  src[2] = 0xFF;
  src[3] = 0xFF;
  return src;
};

// MANUALLY UNROLLED FULL - 8 ELEMENTS
template <typename F>
inline F *
full_8(F *src) noexcept
{
  src[0] = 0xFF;
  src[1] = 0xFF;
  src[2] = 0xFF;
  src[3] = 0xFF;
  src[4] = 0xFF;
  src[5] = 0xFF;
  src[6] = 0xFF;
  src[7] = 0xFF;
  return src;
};

// MANUALLY UNROLLED FULL - 16 ELEMENTS
template <typename F>
inline F *
full_16(F *src) noexcept
{
  src[0] = 0xFF;
  src[1] = 0xFF;
  src[2] = 0xFF;
  src[3] = 0xFF;
  src[4] = 0xFF;
  src[5] = 0xFF;
  src[6] = 0xFF;
  src[7] = 0xFF;
  src[8] = 0xFF;
  src[9] = 0xFF;
  src[10] = 0xFF;
  src[11] = 0xFF;
  src[12] = 0xFF;
  src[13] = 0xFF;
  src[14] = 0xFF;
  src[15] = 0xFF;
  return src;
};

// MANUALLY UNROLLED FULL - 32 ELEMENTS
template <typename F>
inline F *
full_32(F *src) noexcept
{
  src[0] = 0xFF;
  src[1] = 0xFF;
  src[2] = 0xFF;
  src[3] = 0xFF;
  src[4] = 0xFF;
  src[5] = 0xFF;
  src[6] = 0xFF;
  src[7] = 0xFF;
  src[8] = 0xFF;
  src[9] = 0xFF;
  src[10] = 0xFF;
  src[11] = 0xFF;
  src[12] = 0xFF;
  src[13] = 0xFF;
  src[14] = 0xFF;
  src[15] = 0xFF;
  src[16] = 0xFF;
  src[17] = 0xFF;
  src[18] = 0xFF;
  src[19] = 0xFF;
  src[20] = 0xFF;
  src[21] = 0xFF;
  src[22] = 0xFF;
  src[23] = 0xFF;
  src[24] = 0xFF;
  src[25] = 0xFF;
  src[26] = 0xFF;
  src[27] = 0xFF;
  src[28] = 0xFF;
  src[29] = 0xFF;
  src[30] = 0xFF;
  src[31] = 0xFF;
  return src;
};

// SAFE ZERO - RUNTIME COUNT
template <typename F, typename M = u64, u64 alignment = alignof(F)>
F *
szero(F *src, const M cnt) noexcept
{
  if ( src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(src, alignment) )
    return nullptr;
  if ( !__is_valid_address(src, cnt) )
    return nullptr;

  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x0;
  return src;
};

// SAFE ZERO WITH REFERENCE RETURN
template <typename F, typename M = u64, u64 alignment = alignof(F)>
bool
rszero(F &s, const M cnt) noexcept
{
  if ( !__is_aligned_to(s, alignment) )
    return false;
  if ( !__is_valid_address(s, cnt) )
    return nullptr;

  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x0;
  return true;
};

// SAFE COMPILE-TIME CONSTANT ZERO
template <u64 M, typename F, u64 alignment = alignof(F)>
F *
sczero(F *src) noexcept
{
  if ( src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(src, alignment) )
    return nullptr;
  if ( !__is_valid_address(src, M) )
    return nullptr;

  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x0;
  return src;
};

// SAFE COMPILE-TIME CONSTANT ZERO WITH REFERENCE RETURN
template <u64 M, typename F, u64 alignment = alignof(F)>
bool
rsczero(F &s) noexcept
{
  if ( !__is_aligned_to(s, alignment) )
    return false;
  if ( !__is_valid_address(s, M) )
    return nullptr;

  F *src = reinterpret_cast<F *>(&s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x0;
  return true;
};

// SAFE FULL - RUNTIME COUNT
template <typename F, typename M = u64, u64 alignment = alignof(F)>
F *
sfull(F *src, const M cnt) noexcept
{
  if ( src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(src, alignment) )
    return nullptr;
  if ( !__is_valid_address(src, cnt) )
    return nullptr;

  for ( M n = 0; n < cnt; n++ )
    src[n] = 0xFF;
  return src;
};

// SAFE FULL WITH REFERENCE RETURN
template <typename F, typename M = u64, u64 alignment = alignof(F)>
bool
rsfull(F &s, const M cnt) noexcept
{
  if ( !__is_aligned_to(s, alignment) )
    return false;
  if ( !__is_valid_address(s, cnt) )
    return nullptr;

  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0xFF;
  return true;
};

// SAFE COMPILE-TIME CONSTANT FULL
template <u64 M, typename F, u64 alignment = alignof(F)>
F *
scfull_safe(F *src) noexcept
{
  if ( src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(src, alignment) )
    return nullptr;
  if ( !__is_valid_address(src, M) )
    return nullptr;

  for ( u64 n = 0; n < M; n++ )
    src[n] = 0xFF;
  return src;
};

// SAFE COMPILE-TIME CONSTANT FULL WITH REFERENCE RETURN
template <u64 M, typename F, u64 alignment = alignof(F)>
bool
rscfull_safe(F &s) noexcept
{
  if ( !__is_aligned_to(s, alignment) )
    return false;
  if ( !__is_valid_address(s, M) )
    return nullptr;

  F *src = reinterpret_cast<F *>(&s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0xFF;
  return true;
};

// FILL WITH 0X01 - RUNTIME COUNT
template <typename F, typename M = u64>
F *
one(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x01;
  return src;
};

// FILL WITH 0X01 - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rone(F &s, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x01;
  return s;
};

// FILL WITH 0X01 - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cone(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x01;
  return src;
};

// FILL WITH 0X01 - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcone(F &s) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x01;
  return s;
};

// FILL WITH ARBITRARY PATTERN - RUNTIME COUNT
template <typename F, typename M = u64>
F *
pattern(F *src, const u8 p, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = p;
  return src;
};

// FILL WITH ARBITRARY PATTERN - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rpattern(F &s, const u8 p, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ )
    src[n] = p;
  return s;
};

// FILL WITH ARBITRARY PATTERN - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cpattern(F *src, const u8 p) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = p;
  return src;
};

// FILL WITH ARBITRARY PATTERN - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcpattern(F &s, const u8 p) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = p;
  return s;
};

// FILL WITH 0XAA ALTERNATING PATTERN - RUNTIME COUNT
template <typename F, typename M = u64>
F *
alternating_aa(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0xAA;
  return src;
};

// FILL WITH 0XAA ALTERNATING PATTERN - REFERENCE RETURN
template <typename F, typename M = u64>
F &
ralternating_aa(F &s, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0xAA;
  return s;
};

// FILL WITH 0XAA ALTERNATING PATTERN - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
calternating_aa(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0xAA;
  return src;
};

// FILL WITH 0XAA ALTERNATING PATTERN - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcalternating_aa(F &s) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0xAA;
  return s;
};

// FILL WITH 0X55 ALTERNATING PATTERN - RUNTIME COUNT
template <typename F, typename M = u64>
F *
alternating_55(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x55;
  return src;
};

// FILL WITH 0X55 ALTERNATING PATTERN - REFERENCE RETURN
template <typename F, typename M = u64>
F &
ralternating_55(F &s, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x55;
  return s;
};

// FILL WITH 0X55 ALTERNATING PATTERN - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
calternating_55(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x55;
  return src;
};

// FILL WITH 0X55 ALTERNATING PATTERN - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcalternating_55(F &s) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x55;
  return s;
};

// FILL WITH HIGH BIT 0X80 - RUNTIME COUNT
template <typename F, typename M = u64>
F *
high_bit(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x80;
  return src;
};

// FILL WITH HIGH BIT 0X80 - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rhigh_bit(F &s, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x80;
  return s;
};

// FILL WITH HIGH BIT 0X80 - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
chigh_bit(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x80;
  return src;
};

// FILL WITH HIGH BIT 0X80 - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rchigh_bit(F &s) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x80;
  return s;
};

// FILL WITH LOW BIT 0X01 - RUNTIME COUNT
template <typename F, typename M = u64>
F *
low_bit(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x01;
  return src;
};

// FILL WITH LOW BIT 0X01 - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rlow_bit(F &s, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0x01;
  return s;
};

// FILL WITH LOW BIT 0X01 - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
clow_bit(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x01;
  return src;
};

// FILL WITH LOW BIT 0X01 - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rclow_bit(F &s) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0x01;
  return s;
};

// SET ALL BITS TO 1 (SAME AS FULL) - RUNTIME COUNT
template <typename F, typename M = u64>
F *
set(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = static_cast<F>(~static_cast<F>(0));
  return src;
};

// SET ALL BITS TO 1 - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rset(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] = static_cast<F>(~static_cast<F>(0));
  return s;
};

// SET ALL BITS TO 1 - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cset(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = static_cast<F>(~static_cast<F>(0));
  return src;
};

// SET ALL BITS TO 1 - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcset(F &s) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] = static_cast<F>(~static_cast<F>(0));
  return s;
};

// CLEAR ALL BITS TO 0 (SAME AS ZERO) - RUNTIME COUNT
template <typename F, typename M = u64>
F *
clear(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = 0;
  return src;
};

// CLEAR ALL BITS TO 0 - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rclear(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] = 0;
  return s;
};

// CLEAR ALL BITS TO 0 - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cclear(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = 0;
  return src;
};

// CLEAR ALL BITS TO 0 - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcclear(F &s) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] = 0;
  return s;
};

// APPLY CONSTANT MASK - RUNTIME COUNT
template <typename F, typename M = u64>
F *
mask(F *src, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = static_cast<F>(m);
  return src;
};

// APPLY CONSTANT MASK - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rmask(F &s, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] = static_cast<F>(m);
  return s;
};

// APPLY CONSTANT MASK - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cmask(F *src, const u8 m) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = static_cast<F>(m);
  return src;
};

// APPLY CONSTANT MASK - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcmask(F &s, const u8 m) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] = static_cast<F>(m);
  return s;
};

// BITWISE INVERT/NOT - RUNTIME COUNT
template <typename F, typename M = u64>
F *
invert(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] = ~src[n];
  return src;
};

// BITWISE INVERT/NOT ALIAS
template <typename F, typename M = u64>
F *
not_(F *src, const M cnt) noexcept
{
  return invert(src, cnt);
};

// BITWISE INVERT - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rinvert(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] = ~s[n];
  return s;
};

// BITWISE INVERT - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cinvert(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] = ~src[n];
  return src;
};

// BITWISE INVERT - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcinvert(F &s) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] = ~s[n];
  return s;
};

// BITWISE AND MASK - RUNTIME COUNT
template <typename F, typename M = u64>
F *
and_mask(F *src, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] &= m;
  return src;
};

// BITWISE AND MASK - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rand_mask(F &s, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] &= m;
  return s;
};

// BITWISE AND MASK - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cand_mask(F *src, const u8 m) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] &= m;
  return src;
};

// BITWISE AND MASK - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcand_mask(F &s, const u8 m) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] &= m;
  return s;
};

// BITWISE OR MASK - RUNTIME COUNT
template <typename F, typename M = u64>
F *
or_mask(F *src, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] |= m;
  return src;
};

// BITWISE OR MASK - REFERENCE RETURN
template <typename F, typename M = u64>
F &
ror_mask(F &s, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] |= m;
  return s;
};

// BITWISE OR MASK - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cor_mask(F *src, const u8 m) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] |= m;
  return src;
};

// BITWISE OR MASK - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcor_mask(F &s, const u8 m) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] |= m;
  return s;
};

// BITWISE XOR MASK - RUNTIME COUNT
template <typename F, typename M = u64>
F *
xor_mask(F *src, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] ^= m;
  return src;
};

// BITWISE XOR MASK - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rxor_mask(F &s, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] ^= m;
  return s;
};

// BITWISE XOR MASK - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cxor_mask(F *src, const u8 m) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] ^= m;
  return src;
};

// BITWISE XOR MASK - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcxor_mask(F &s, const u8 m) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] ^= m;
  return s;
};

// INCREMENT EACH ELEMENT - RUNTIME COUNT
template <typename F, typename M = u64>
F *
increment(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    ++src[n];
  return src;
};

// INCREMENT EACH ELEMENT - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rincrement(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    ++s[n];
  return s;
};

// INCREMENT EACH ELEMENT - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cincrement(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    ++src[n];
  return src;
};

// INCREMENT EACH ELEMENT - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcincrement(F &s) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    ++s[n];
  return s;
};

// DECREMENT EACH ELEMENT - RUNTIME COUNT
template <typename F, typename M = u64>
F *
decrement(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    --src[n];
  return src;
};

// DECREMENT EACH ELEMENT - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rdecrement(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    --s[n];
  return s;
};

// DECREMENT EACH ELEMENT - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cdecrement(F *src) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    --src[n];
  return src;
};

// DECREMENT EACH ELEMENT - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcdecrement(F &s) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    --s[n];
  return s;
};

// ADD CONSTANT VALUE TO EACH ELEMENT - RUNTIME COUNT
template <typename F, typename M = u64>
F *
add(F *src, const u8 v, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] += v;
  return src;
};

// ADD CONSTANT VALUE - REFERENCE RETURN
template <typename F, typename M = u64>
F &
radd(F &s, const u8 v, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] += v;
  return s;
};

// ADD CONSTANT VALUE - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
cadd(F *src, const u8 v) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] += v;
  return src;
};

// ADD CONSTANT VALUE - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcadd(F &s, const u8 v) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] += v;
  return s;
};

// SUBTRACT CONSTANT VALUE FROM EACH ELEMENT - RUNTIME COUNT
template <typename F, typename M = u64>
F *
sub(F *src, const u8 v, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    src[n] -= v;
  return src;
};

// SUBTRACT CONSTANT VALUE - REFERENCE RETURN
template <typename F, typename M = u64>
F &
rsub(F &s, const u8 v, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ )
    s[n] -= v;
  return s;
};

// SUBTRACT CONSTANT VALUE - COMPILE-TIME CONSTANT COUNT
template <u64 M, typename F>
constexpr F *
csub(F *src, const u8 v) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    src[n] -= v;
  return src;
};

// SUBTRACT CONSTANT VALUE - COMPILE-TIME CONSTANT COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcsub(F &s, const u8 v) noexcept
{
  for ( u64 n = 0; n < M; n++ )
    s[n] -= v;
  return s;
};

// MEMORY OBFUSCATION (XOR WITH 0X15)
template <typename F>
F *
memfrob(F *src, u64 n) noexcept
{
  F *a = src;
  while ( n-- > 0 )
    *a++ ^= 0x15;
  return a;
}
};
