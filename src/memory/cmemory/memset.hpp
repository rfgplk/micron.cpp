//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../attributes.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../addr.hpp"

#include "../../simd/intrin.hpp"
#include "../../simd/memory.hpp"

#include "bits.hpp"

namespace micron
{

namespace __unroll
{

// assign val into each index position
template <typename T, usize... I>
inline __attribute__((always_inline)) void
assign(T *src, const T val, micron::index_sequence<I...>) noexcept
{
  ((src[I] = val), ...);
}

template <typename T, usize... I>
inline __attribute__((always_inline)) void
vassign(volatile T *src, const T val, micron::index_sequence<I...>) noexcept
{
  ((src[I] = val), ...);
}

template <typename T, usize... I>
inline __attribute__((always_inline)) void
set_all(T *src, micron::index_sequence<I...>) noexcept
{
  ((src[I] = static_cast<T>(~static_cast<T>(0))), ...);
}

template <typename T, usize... I>
inline __attribute__((always_inline)) void
invert(T *src, micron::index_sequence<I...>) noexcept
{
  ((src[I] = ~src[I]), ...);
}

template <typename T, usize... I>
inline __attribute__((always_inline)) void
and_mask(T *src, const u8 m, micron::index_sequence<I...>) noexcept
{
  ((src[I] &= static_cast<T>(m)), ...);
}

template <typename T, usize... I>
inline __attribute__((always_inline)) void
or_mask(T *src, const u8 m, micron::index_sequence<I...>) noexcept
{
  ((src[I] |= static_cast<T>(m)), ...);
}

template <typename T, usize... I>
inline __attribute__((always_inline)) void
xor_mask(T *src, const u8 m, micron::index_sequence<I...>) noexcept
{
  ((src[I] ^= static_cast<T>(m)), ...);
}

template <typename T, usize... I>
inline __attribute__((always_inline)) void
increment(T *src, micron::index_sequence<I...>) noexcept
{
  ((++src[I]), ...);
}

template <typename T, usize... I>
inline __attribute__((always_inline)) void
decrement(T *src, micron::index_sequence<I...>) noexcept
{
  ((--src[I]), ...);
}

template <typename T, usize... I>
inline __attribute__((always_inline)) void
add(T *src, const u8 v, micron::index_sequence<I...>) noexcept
{
  ((src[I] += static_cast<T>(v)), ...);
}

template <typename T, usize... I>
inline __attribute__((always_inline)) void
sub(T *src, const u8 v, micron::index_sequence<I...>) noexcept
{
  ((src[I] -= static_cast<T>(v)), ...);
}

};     // namespace __unroll

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//memsets

// BASIC MEMSET - RUNTIME COUNT
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
memset(F *s, const byte in, const u64 cnt) noexcept
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
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return reinterpret_cast<F *>(src);
}

// MEMSET WITH REFERENCE RETURN
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
F &
rmemset(F &s, const byte in, const u64 cnt) noexcept
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
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return s;
}

// CONSTEXPR MEMSET
template <typename F>
constexpr F *
constexpr_memset(F *src, const byte in, const u64 cnt) noexcept
{
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return src;
}

// COMPILE-TIME CONSTANT MEMSET - TEMPLATE COUNT AND VALUE
template <byte in, u64 cnt, typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
memset(F *s) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  __unroll::assign(src, in, micron::make_index_sequence<cnt>{});
  return reinterpret_cast<F *>(src);
}

// COMPILE-TIME CONSTANT MEMSET - TEMPLATE COUNT ONLY
template <u64 M, typename F>
__attribute__((nonnull)) F *
cmemset(F *s, const byte in) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return reinterpret_cast<F *>(src);
}

// COMPILE-TIME CONSTANT MEMSET WITH REFERENCE RETURN
template <u64 M, typename F>
F &
rcmemset(F &s, const byte in) noexcept
{
  byte *src = reinterpret_cast<byte *>(&s);
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return s;
}

// SECURE COMPILE-TIME CONSTANT MEMSET
template <u64 M, typename F>
__attribute__((nonnull)) F *
scmemset(F *s, const byte in) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(s);
  __unroll::vassign(src, in, micron::make_index_sequence<M>{});
  __mem_barrier();
  return s;
}

// SECURE COMPILE-TIME CONSTANT MEMSET WITH REFERENCE RETURN
template <u64 M, typename F>
F &
rscmemset(F &s, const byte in) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(&s);
  __unroll::vassign(src, in, micron::make_index_sequence<M>{});
  __mem_barrier();
  return s;
}

// MANUALLY UNROLLED MEMSET - fixed byte widths
template <typename F>
inline F *
memset_8b(F *src, int in) noexcept
{
  byte *mem = reinterpret_cast<byte *>(src);
  __unroll::assign(mem, static_cast<byte>(in), micron::make_index_sequence<8>{});
  return src;
}

template <typename F>
inline F *
memset_16b(F *src, int in) noexcept
{
  byte *mem = reinterpret_cast<byte *>(src);
  __unroll::assign(mem, static_cast<byte>(in), micron::make_index_sequence<16>{});
  return src;
}

template <typename F>
inline F *
memset_32b(F *src, int in) noexcept
{
  byte *mem = reinterpret_cast<byte *>(src);
  __unroll::assign(mem, static_cast<byte>(in), micron::make_index_sequence<32>{});
  return src;
}

template <typename F>
inline F *
memset_64b(F *src, int in) noexcept
{
  byte *mem = reinterpret_cast<byte *>(src);
  __unroll::assign(mem, static_cast<byte>(in), micron::make_index_sequence<64>{});
  return src;
}

// SAFE MEMSET - RUNTIME COUNT
template <typename F, u64 alignment = alignof(F)>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
smemset(F *s, const byte in, const u64 cnt) noexcept
{
  if ( s == nullptr ) return nullptr;
  if ( !__is_aligned_to(s, alignment) ) return nullptr;
  if ( !__is_valid_address(s, cnt) ) return nullptr;
  byte *src = reinterpret_cast<byte *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return reinterpret_cast<F *>(src);
}

// SAFE MEMSET WITH REFERENCE RETURN
template <typename F, u64 alignment = alignof(F)>
  requires(!micron::is_null_pointer_v<F>)
bool
rsmemset(F &s, const byte in, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(s, alignment) ) return false;
  if ( !__is_valid_address(s, cnt) ) return false;
  byte *src = reinterpret_cast<byte *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return true;
}

// SAFE COMPILE-TIME CONSTANT MEMSET - TEMPLATE COUNT ONLY
template <u64 M, typename F, u64 alignment = alignof(F)>
__attribute__((nonnull)) F *
scmemset_safe(F *s, const byte in) noexcept
{
  if ( s == nullptr ) return nullptr;
  if ( !__is_aligned_to(s, alignment) ) return nullptr;
  if ( !__is_valid_address(s, M) ) return nullptr;
  byte *src = reinterpret_cast<byte *>(s);
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return reinterpret_cast<F *>(src);
}

// SAFE COMPILE-TIME CONSTANT MEMSET WITH REFERENCE RETURN
template <u64 M, typename F, u64 alignment = alignof(F)>
bool
rscmemset_safe(F &s, const byte in) noexcept
{
  if ( !__is_aligned_to(s, alignment) ) return false;
  if ( !__is_valid_address(s, M) ) return false;
  byte *src = reinterpret_cast<byte *>(&s);
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bytesets

// BASIC BYTESET - RUNTIME COUNT
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
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
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return reinterpret_cast<F *>(src);
}

// BYTESET ALIAS
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
bset(F *s, const byte in, const u64 cnt) noexcept
{
  return byteset(s, in, cnt);
}

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
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return s;
}

// BYTESET ALIAS WITH REFERENCE RETURN
template <typename F>
  requires(!micron::is_null_pointer_v<F>)
F &
rbset(F &s, const byte in, const u64 cnt) noexcept
{
  return rbyteset(s, in, cnt);
}

// COMPILE-TIME CONSTANT BYTESET - TEMPLATE COUNT ONLY
template <u64 N, typename F>
__attribute__((nonnull)) F *
cbyteset(F *s, const byte in) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  __unroll::assign(src, in, micron::make_index_sequence<N>{});
  return reinterpret_cast<F *>(src);
}

// COMPILE-TIME CONSTANT BYTESET ALIAS
template <u64 N, typename F>
__attribute__((nonnull)) F *
cbset(F *s, const byte in) noexcept
{
  return cbyteset<N, F>(s, in);
}

// COMPILE-TIME CONSTANT BYTESET WITH REFERENCE RETURN
template <u64 N, typename F>
F &
rcbyteset(F &s, const byte in) noexcept
{
  byte *src = reinterpret_cast<byte *>(&s);
  __unroll::assign(src, in, micron::make_index_sequence<N>{});
  return s;
}

// COMPILE-TIME CONSTANT BYTESET ALIAS WITH REFERENCE RETURN
template <u64 N, typename F>
F &
rcbset(F &s, const byte in) noexcept
{
  return rcbyteset<N, F>(s, in);
}

// SECURE COMPILE-TIME CONSTANT BYTESET
template <u64 N, typename F>
__attribute__((nonnull)) F *
scbyteset(F *s, const byte in) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(s);
  __unroll::vassign(src, in, micron::make_index_sequence<N>{});
  __mem_barrier();
  return s;
}

// SECURE COMPILE-TIME CONSTANT BYTESET ALIAS
template <u64 N, typename F>
__attribute__((nonnull)) F *
scbset(F *s, const byte in) noexcept
{
  return scbyteset<N, F>(s, in);
}

// SECURE COMPILE-TIME CONSTANT BYTESET WITH REFERENCE RETURN
template <u64 N, typename F>
F &
rscbyteset(F &s, const byte in) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(&s);
  __unroll::vassign(src, in, micron::make_index_sequence<N>{});
  __mem_barrier();
  return s;
}

// SECURE COMPILE-TIME CONSTANT BYTESET ALIAS WITH REFERENCE RETURN
template <u64 N, typename F>
F &
rscbset(F &s, const byte in) noexcept
{
  return rscbyteset<N, F>(s, in);
}

// COMPILE-TIME CONSTANT BYTESET - TEMPLATE COUNT AND VALUE
template <byte in, u64 cnt, typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
byteset(F *s) noexcept
{
  byte *src = reinterpret_cast<byte *>(s);
  __unroll::assign(src, in, micron::make_index_sequence<cnt>{});
  return reinterpret_cast<F *>(src);
}

// COMPILE-TIME CONSTANT BYTESET ALIAS - TEMPLATE COUNT AND VALUE
template <byte in, u64 cnt, typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
bset(F *s) noexcept
{
  return byteset<in, cnt, F>(s);
}

// MANUALLY UNROLLED BYTESET - fixed byte widths
template <typename F>
inline F *
byteset_8b(F *src, byte in) noexcept
{
  byte *mem = reinterpret_cast<byte *>(src);
  __unroll::assign(mem, in, micron::make_index_sequence<8>{});
  return src;
}

template <typename F>
inline F *
bset_8b(F *src, byte in) noexcept
{
  return byteset_8b(src, in);
}

template <typename F>
inline F *
byteset_16b(F *src, byte in) noexcept
{
  byte *mem = reinterpret_cast<byte *>(src);
  __unroll::assign(mem, in, micron::make_index_sequence<16>{});
  return src;
}

template <typename F>
inline F *
bset_16b(F *src, byte in) noexcept
{
  return byteset_16b(src, in);
}

template <typename F>
inline F *
byteset_32b(F *src, byte in) noexcept
{
  byte *mem = reinterpret_cast<byte *>(src);
  __unroll::assign(mem, in, micron::make_index_sequence<32>{});
  return src;
}

template <typename F>
inline F *
bset_32b(F *src, byte in) noexcept
{
  return byteset_32b(src, in);
}

template <typename F>
inline F *
byteset_64b(F *src, byte in) noexcept
{
  byte *mem = reinterpret_cast<byte *>(src);
  __unroll::assign(mem, in, micron::make_index_sequence<64>{});
  return src;
}

template <typename F>
inline F *
bset_64b(F *src, byte in) noexcept
{
  return byteset_64b(src, in);
}

// SAFE BYTESET - RUNTIME COUNT
template <typename F, u64 alignment = 1>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
sbyteset(F *s, const byte in, const u64 cnt) noexcept
{
  if ( s == nullptr ) return nullptr;
  if ( !__is_aligned_to(s, alignment) ) return nullptr;
  if ( !__is_valid_address(s, cnt) ) return nullptr;
  byte *src = reinterpret_cast<byte *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return reinterpret_cast<F *>(src);
}

// SAFE BYTESET ALIAS
template <typename F, u64 alignment = 1>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
sbset(F *s, const byte in, const u64 cnt) noexcept
{
  return sbyteset<F, alignment>(s, in, cnt);
}

// SAFE BYTESET WITH REFERENCE RETURN
template <typename F, u64 alignment = 1>
  requires(!micron::is_null_pointer_v<F>)
bool
rsbyteset(F &s, const byte in, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(s, alignment) ) return false;
  if ( !__is_valid_address(s, cnt) ) return false;
  byte *src = reinterpret_cast<byte *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return true;
}

// SAFE BYTESET ALIAS WITH REFERENCE RETURN
template <typename F, u64 alignment = 1>
  requires(!micron::is_null_pointer_v<F>)
bool
rsbset(F &s, const byte in, const u64 cnt) noexcept
{
  return rsbyteset<F, alignment>(s, in, cnt);
}

// SAFE COMPILE-TIME CONSTANT BYTESET
template <u64 N, typename F, u64 alignment = 1>
__attribute__((nonnull)) F *
scbyteset_safe(F *s, const byte in) noexcept
{
  if ( s == nullptr ) return nullptr;
  if ( !__is_aligned_to(s, alignment) ) return nullptr;
  if ( !__is_valid_address(s, N) ) return nullptr;
  byte *src = reinterpret_cast<byte *>(s);
  __unroll::assign(src, in, micron::make_index_sequence<N>{});
  return reinterpret_cast<F *>(src);
}

// SAFE COMPILE-TIME CONSTANT BYTESET ALIAS
template <u64 N, typename F, u64 alignment = 1>
__attribute__((nonnull)) F *
scbset_safe(F *s, const byte in) noexcept
{
  return scbyteset_safe<N, F, alignment>(s, in);
}

// SAFE COMPILE-TIME CONSTANT BYTESET WITH REFERENCE RETURN
template <u64 N, typename F, u64 alignment = 1>
bool
rscbyteset_safe(F &s, const byte in) noexcept
{
  if ( !__is_aligned_to(s, alignment) ) return false;
  if ( !__is_valid_address(s, N) ) return false;
  byte *src = reinterpret_cast<byte *>(&s);
  __unroll::assign(src, in, micron::make_index_sequence<N>{});
  return true;
}

// SAFE COMPILE-TIME CONSTANT BYTESET ALIAS WITH REFERENCE RETURN
template <u64 N, typename F, u64 alignment = 1>
bool
rscbset_safe(F &s, const byte in) noexcept
{
  return rscbyteset_safe<N, F, alignment>(s, in);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bzeros

// RUNTIME COUNT
template <typename M = u64>
byte *
bzero(byte *src, const M cnt) noexcept
{
  memset<byte>(src, 0x0, cnt);
  return src;
}

// COMPILE-TIME COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
  requires(micron::is_fundamental_v<F> or micron::is_pointer_v<F>)
constexpr F &
cbzero(F &_src) noexcept
{
  byte *src = reinterpret_cast<byte *>(&_src);
  __unroll::assign(src, byte(0x0), micron::make_index_sequence<M>{});
  return _src;
}

// COMPILE-TIME COUNT
template <u64 M, typename F>
  requires(micron::is_fundamental_v<F> or micron::is_pointer_v<F>)
constexpr F *
cbzero(F *_src) noexcept
{
  byte *src = reinterpret_cast<byte *>(_src);
  __unroll::assign(src, byte(0x0), micron::make_index_sequence<M>{});
  return reinterpret_cast<F *>(src);
}

// SECURE COMPILE-TIME COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
  requires(micron::is_fundamental_v<F> or micron::is_pointer_v<F>)
constexpr F &
scbzero(F &_src) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(&_src);
  __unroll::vassign(src, byte(0x0), micron::make_index_sequence<M>{});
  __mem_barrier();
  return _src;
}

// SECURE COMPILE-TIME COUNT
template <u64 M, typename F>
  requires(micron::is_fundamental_v<F> or micron::is_pointer_v<F>)
constexpr F *
scbzero(F *_src) noexcept
{
  volatile byte *src = reinterpret_cast<volatile byte *>(_src);
  __unroll::vassign(src, byte(0x0), micron::make_index_sequence<M>{});
  __mem_barrier();
  return _src;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// typesets

// BASIC TYPESET - RUNTIME COUNT
template <typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
typeset(F *s, const T in, const u64 cnt) noexcept
{
  T *src = reinterpret_cast<T *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return reinterpret_cast<F *>(src);
}

// TYPESET WITH REFERENCE RETURN
template <typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
F &
rtypeset(F &s, const T in, const u64 cnt) noexcept
{
  T *src = reinterpret_cast<T *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return s;
}

// COMPILE-TIME CONSTANT TYPESET - TEMPLATE COUNT ONLY
template <u64 M, typename T, typename F>
__attribute__((nonnull)) F *
ctypeset(F *s, const T in) noexcept
{
  T *src = reinterpret_cast<T *>(s);
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return reinterpret_cast<F *>(src);
}

// COMPILE-TIME CONSTANT TYPESET WITH REFERENCE RETURN
template <u64 M, typename T, typename F>
F &
rctypeset(F &s, const T in) noexcept
{
  T *src = reinterpret_cast<T *>(&s);
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return s;
}

// SECURE COMPILE-TIME CONSTANT TYPESET
template <u64 M, typename T, typename F>
__attribute__((nonnull)) F *
sctypeset(F *s, const T in) noexcept
{
  volatile T *src = reinterpret_cast<volatile T *>(s);
  __unroll::vassign(src, in, micron::make_index_sequence<M>{});
  __mem_barrier();
  return s;
}

// SECURE COMPILE-TIME CONSTANT TYPESET WITH REFERENCE RETURN
template <u64 M, typename T, typename F>
F &
rsctypeset(F &s, const T in) noexcept
{
  volatile T *src = reinterpret_cast<volatile T *>(&s);
  __unroll::vassign(src, in, micron::make_index_sequence<M>{});
  __mem_barrier();
  return s;
}

// SAFE TYPESET - RUNTIME COUNT
template <typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) F *
stypeset(F *s, const T in, const u64 cnt) noexcept
{
  if ( s == nullptr ) return nullptr;
  if ( !__is_aligned_to(s, alignment) ) return nullptr;
  if ( !__is_valid_address(s, cnt) ) return nullptr;
  T *src = reinterpret_cast<T *>(s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return reinterpret_cast<F *>(src);
}

// SAFE TYPESET WITH REFERENCE RETURN
template <typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
bool
rstypeset(F &s, const T in, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(s, alignment) ) return false;
  if ( !__is_valid_address(s, cnt) ) return false;
  T *src = reinterpret_cast<T *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return true;
}

// SAFE COMPILE-TIME CONSTANT TYPESET
template <u64 M, typename T, typename F, u64 alignment = alignof(T)>
__attribute__((nonnull)) F *
sctypeset_safe(F *s, const T in) noexcept
{
  if ( s == nullptr ) return nullptr;
  if ( !__is_aligned_to(s, alignment) ) return nullptr;
  if ( !__is_valid_address(s, M) ) return nullptr;
  T *src = reinterpret_cast<T *>(s);
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return reinterpret_cast<F *>(src);
}

// SAFE COMPILE-TIME CONSTANT TYPESET WITH REFERENCE RETURN
template <u64 M, typename T, typename F, u64 alignment = alignof(T)>
bool
rsctypeset_safe(F &s, const T in) noexcept
{
  if ( !__is_aligned_to(s, alignment) ) return false;
  if ( !__is_valid_address(s, M) ) return false;
  T *src = reinterpret_cast<T *>(&s);
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wordsets

// BASIC WORDSET - RUNTIME COUNT
word *
wordset(word *src, const word in, const u64 cnt) noexcept
{
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return src;
}

// WORDSET WITH REFERENCE RETURN
word &
rwordset(word &s, const word in, const u64 cnt) noexcept
{
  word *src = &s;
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return s;
}

// COMPILE-TIME CONSTANT WORDSET - TEMPLATE COUNT ONLY
template <u64 M>
word *
cwordset(word *src, const word in) noexcept
{
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return src;
}

// COMPILE-TIME CONSTANT WORDSET WITH REFERENCE RETURN
template <u64 M>
word &
rcwordset(word &s, const word in) noexcept
{
  word *src = reinterpret_cast<word *>(&s);
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return s;
}

// SECURE COMPILE-TIME CONSTANT WORDSET
template <u64 M>
word *
scwordset(word *src, const word in) noexcept
{
  volatile word *vsrc = reinterpret_cast<volatile word *>(src);
  __unroll::vassign(vsrc, in, micron::make_index_sequence<M>{});
  __mem_barrier();
  return src;
}

// SECURE COMPILE-TIME CONSTANT WORDSET WITH REFERENCE RETURN
template <u64 M>
word &
rscwordset(word &s, const word in) noexcept
{
  volatile word *src = reinterpret_cast<volatile word *>(&s);
  __unroll::vassign(src, in, micron::make_index_sequence<M>{});
  __mem_barrier();
  return s;
}

// COMPILE-TIME CONSTANT WORDSET - TEMPLATE COUNT AND VALUE
template <word in, u64 cnt>
word *
wordset(word *src) noexcept
{
  __unroll::assign(src, in, micron::make_index_sequence<cnt>{});
  return src;
}

// MANUALLY UNROLLED WORDSET - compile-time value, fixed widths
template <word in>
inline word *
wordset_4w(word *src) noexcept
{
  __unroll::assign(src, in, micron::make_index_sequence<4>{});
  return src;
}

template <word in>
inline word *
wordset_8w(word *src) noexcept
{
  __unroll::assign(src, in, micron::make_index_sequence<8>{});
  return src;
}

template <word in>
inline word *
wordset_16w(word *src) noexcept
{
  __unroll::assign(src, in, micron::make_index_sequence<16>{});
  return src;
}

template <word in>
inline word *
wordset_32w(word *src) noexcept
{
  __unroll::assign(src, in, micron::make_index_sequence<32>{});
  return src;
}

// SAFE WORDSET - RUNTIME COUNT
template <u64 alignment = alignof(word)>
word *
swordset(word *src, const word in, const u64 cnt) noexcept
{
  if ( src == nullptr ) return nullptr;
  if ( !__is_aligned_to(src, alignment) ) return nullptr;
  if ( !__is_valid_address(src, cnt) ) return nullptr;
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return src;
}

// SAFE WORDSET WITH REFERENCE RETURN
template <u64 alignment = alignof(word)>
bool
rswordset(word &s, const word in, const u64 cnt) noexcept
{
  if ( !__is_aligned_to_r(s, alignment) ) return false;
  if ( !__is_valid_address(s, cnt) ) return false;
  word *src = reinterpret_cast<word *>(&s);
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      src[n] = in;
      src[n + 1] = in;
      src[n + 2] = in;
      src[n + 3] = in;
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) src[n] = in;
  return true;
}

// SAFE COMPILE-TIME CONSTANT WORDSET
template <u64 M, u64 alignment = alignof(word)>
word *
scwordset_safe(word *src, const word in) noexcept
{
  if ( src == nullptr ) return nullptr;
  if ( !__is_aligned_to(src, alignment) ) return nullptr;
  if ( !__is_valid_address(src, M) ) return nullptr;
  __unroll::assign(src, in, micron::make_index_sequence<M>{});
  return src;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bitwise

// ZERO - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
zero(F *src, const M cnt) noexcept
{
  byteset(src, 0x0, sizeof(F) * cnt);
  return src;
}

template <typename F, typename M = u64>
constexpr F *
constexpr_zero(F *src, const M cnt) noexcept
{
  constexpr_memset(src, 0x0, cnt);
  return src;
}

template <typename F, typename M = u64>
F &
rzero(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] = 0x0;
  return s;
}

// ZERO - COMPILE-TIME COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
  requires(micron::is_fundamental_v<F> or micron::is_pointer_v<F>
           or micron::is_trivially_constructible_v<F> && micron::is_trivially_destructible_v<F>)
constexpr F &
czero(F &src) noexcept
{
  cbyteset<M * sizeof(F)>(micron::addr(src), 0x0);
  return src;
}

// ZERO - COMPILE-TIME COUNT
template <u64 M, typename F>
  requires(micron::is_fundamental_v<F> or micron::is_pointer_v<F>
           or micron::is_trivially_constructible_v<F> && micron::is_trivially_destructible_v<F>)
constexpr F *
czero(F *src) noexcept
{
  cbyteset<M * sizeof(F)>(src, 0x0);
  return reinterpret_cast<F *>(src);
}

// SECURE ZERO - COMPILE-TIME COUNT WITH REFERENCE RETURN (single-element overload)
template <u64 M, typename F>
  requires(micron::is_fundamental_v<F> or micron::is_pointer_v<F>)
constexpr F &
sczero(F &s) noexcept
{
  volatile F *src = reinterpret_cast<volatile F *>(&s);
  __unroll::vassign(src, F(0), micron::make_index_sequence<M>{});
  __mem_barrier();
  return s;
}

// SECURE ZERO - COMPILE-TIME COUNT
template <u64 M, typename F>
  requires(micron::is_fundamental_v<F> or micron::is_pointer_v<F>)
constexpr F *
sczero(F *s) noexcept
{
  volatile F *src = reinterpret_cast<volatile F *>(s);
  __unroll::vassign(src, F(0), micron::make_index_sequence<M>{});
  __mem_barrier();
  return s;
}

// MANUALLY UNROLLED ZERO - fixed element widths
template <typename F>
inline F *
zero_4(F *src) noexcept
{
  __unroll::assign(src, F(0), micron::make_index_sequence<4>{});
  return src;
}

template <typename F>
inline F *
zero_8(F *src) noexcept
{
  __unroll::assign(src, F(0), micron::make_index_sequence<8>{});
  return src;
}

template <typename F>
inline F *
zero_16(F *src) noexcept
{
  __unroll::assign(src, F(0), micron::make_index_sequence<16>{});
  return src;
}

template <typename F>
inline F *
zero_32(F *src) noexcept
{
  __unroll::assign(src, F(0), micron::make_index_sequence<32>{});
  return src;
}

// FULL (0xFF) - RUNTIME COUNT
template <typename F, typename M = u64>
constexpr F *
full(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = 0xFF;
  return reinterpret_cast<F *>(src);
}

template <typename F, typename M = u64>
constexpr F &
rfull(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] = 0xFF;
  return s;
}

// FULL - COMPILE-TIME COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rcfull(F &s) noexcept
{
  __unroll::assign(&s, static_cast<F>(0xFF), micron::make_index_sequence<M>{});
  return s;
}

// FULL - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cfull(F *src) noexcept
{
  __unroll::assign(src, static_cast<F>(0xFF), micron::make_index_sequence<M>{});
  return reinterpret_cast<F *>(src);
}

// SECURE FULL - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
scfull(F *s) noexcept
{
  volatile F *src = reinterpret_cast<volatile F *>(s);
  __unroll::vassign(src, static_cast<F>(0xFF), micron::make_index_sequence<M>{});
  __mem_barrier();
  return s;
}

// SECURE FULL - COMPILE-TIME COUNT WITH REFERENCE RETURN
template <u64 M, typename F>
constexpr F &
rscfull(F &s) noexcept
{
  volatile F *src = reinterpret_cast<volatile F *>(&s);
  __unroll::vassign(src, static_cast<F>(0xFF), micron::make_index_sequence<M>{});
  __mem_barrier();
  return s;
}

// MANUALLY UNROLLED FULL - fixed element widths
template <typename F>
inline F *
full_4(F *src) noexcept
{
  __unroll::assign(src, static_cast<F>(0xFF), micron::make_index_sequence<4>{});
  return src;
}

template <typename F>
inline F *
full_8(F *src) noexcept
{
  __unroll::assign(src, static_cast<F>(0xFF), micron::make_index_sequence<8>{});
  return src;
}

template <typename F>
inline F *
full_16(F *src) noexcept
{
  __unroll::assign(src, static_cast<F>(0xFF), micron::make_index_sequence<16>{});
  return src;
}

template <typename F>
inline F *
full_32(F *src) noexcept
{
  __unroll::assign(src, static_cast<F>(0xFF), micron::make_index_sequence<32>{});
  return src;
}

// SAFE ZERO - RUNTIME COUNT
template <typename F, typename M = u64, u64 alignment = alignof(F)>
__attribute__((nonnull)) F *
szero(F *src, const M cnt) noexcept
{
  if ( src == nullptr ) return nullptr;
  if ( !__is_aligned_to(src, alignment) ) return nullptr;
  if ( !__is_valid_address(src, cnt) ) return nullptr;
  for ( M n = 0; n < cnt; n++ ) src[n] = 0x0;
  return src;
}

template <typename F, typename M = u64, u64 alignment = alignof(F)>
bool
rszero(F &s, const M cnt) noexcept
{
  if ( !__is_aligned_to(s, alignment) ) return false;
  if ( !__is_valid_address(s, cnt) ) return false;
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ ) src[n] = 0x0;
  return true;
}

// SAFE COMPILE-TIME CONSTANT ZERO
template <u64 M, typename F, u64 alignment = alignof(F)>
__attribute__((nonnull)) F *
sczero(F *src) noexcept
{
  if ( src == nullptr ) return nullptr;
  if ( !__is_aligned_to(src, alignment) ) return nullptr;
  if ( !__is_valid_address(src, M) ) return nullptr;
  __unroll::assign(src, F(0), micron::make_index_sequence<M>{});
  return src;
}

// SAFE COMPILE-TIME CONSTANT ZERO WITH REFERENCE RETURN
template <u64 M, typename F, u64 alignment = alignof(F)>
bool
rsczero(F &s) noexcept
{
  if ( !__is_aligned_to(s, alignment) ) return false;
  if ( !__is_valid_address(s, M) ) return false;
  F *src = reinterpret_cast<F *>(&s);
  __unroll::assign(src, F(0), micron::make_index_sequence<M>{});
  return true;
}

// SAFE FULL - RUNTIME COUNT
template <typename F, typename M = u64, u64 alignment = alignof(F)>
__attribute__((nonnull)) F *
sfull(F *src, const M cnt) noexcept
{
  if ( src == nullptr ) return nullptr;
  if ( !__is_aligned_to(src, alignment) ) return nullptr;
  if ( !__is_valid_address(src, cnt) ) return nullptr;
  for ( M n = 0; n < cnt; n++ ) src[n] = 0xFF;
  return src;
}

template <typename F, typename M = u64, u64 alignment = alignof(F)>
bool
rsfull(F &s, const M cnt) noexcept
{
  if ( !__is_aligned_to(s, alignment) ) return false;
  if ( !__is_valid_address(s, cnt) ) return false;
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ ) src[n] = 0xFF;
  return true;
}

// SAFE COMPILE-TIME CONSTANT FULL
template <u64 M, typename F, u64 alignment = alignof(F)>
__attribute__((nonnull)) F *
scfull_safe(F *src) noexcept
{
  if ( src == nullptr ) return nullptr;
  if ( !__is_aligned_to(src, alignment) ) return nullptr;
  if ( !__is_valid_address(src, M) ) return nullptr;
  __unroll::assign(src, static_cast<F>(0xFF), micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F, u64 alignment = alignof(F)>
bool
rscfull_safe(F &s) noexcept
{
  if ( !__is_aligned_to(s, alignment) ) return false;
  if ( !__is_valid_address(s, M) ) return false;
  F *src = reinterpret_cast<F *>(&s);
  __unroll::assign(src, static_cast<F>(0xFF), micron::make_index_sequence<M>{});
  return true;
}

// FILL WITH 0x01 - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
one(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = 0x01;
  return src;
}

template <typename F, typename M = u64>
F &
rone(F &s, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ ) src[n] = 0x01;
  return s;
}

// FILL WITH 0x01 - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cone(F *src) noexcept
{
  __unroll::assign(src, static_cast<F>(0x01), micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcone(F &s) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  __unroll::assign(src, static_cast<F>(0x01), micron::make_index_sequence<M>{});
  return s;
}

// FILL WITH ARBITRARY PATTERN - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
pattern(F *src, const u8 p, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = p;
  return src;
}

template <typename F, typename M = u64>
F &
rpattern(F &s, const u8 p, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ ) src[n] = p;
  return s;
}

// FILL WITH ARBITRARY PATTERN - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cpattern(F *src, const u8 p) noexcept
{
  __unroll::assign(src, static_cast<F>(p), micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcpattern(F &s, const u8 p) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  __unroll::assign(src, static_cast<F>(p), micron::make_index_sequence<M>{});
  return s;
}

// ALTERNATING 0xAA - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
alternating_aa(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = 0xAA;
  return src;
}

template <typename F, typename M = u64>
F &
ralternating_aa(F &s, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ ) src[n] = 0xAA;
  return s;
}

// ALTERNATING 0xAA - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
calternating_aa(F *src) noexcept
{
  __unroll::assign(src, static_cast<F>(0xAA), micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcalternating_aa(F &s) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  __unroll::assign(src, static_cast<F>(0xAA), micron::make_index_sequence<M>{});
  return s;
}

// ALTERNATING 0x55 - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
alternating_55(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = 0x55;
  return src;
}

template <typename F, typename M = u64>
F &
ralternating_55(F &s, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ ) src[n] = 0x55;
  return s;
}

// ALTERNATING 0x55 - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
calternating_55(F *src) noexcept
{
  __unroll::assign(src, static_cast<F>(0x55), micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcalternating_55(F &s) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  __unroll::assign(src, static_cast<F>(0x55), micron::make_index_sequence<M>{});
  return s;
}

// HIGH BIT 0x80 - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
high_bit(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = 0x80;
  return src;
}

template <typename F, typename M = u64>
F &
rhigh_bit(F &s, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ ) src[n] = 0x80;
  return s;
}

// HIGH BIT 0x80 - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
chigh_bit(F *src) noexcept
{
  __unroll::assign(src, static_cast<F>(0x80), micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rchigh_bit(F &s) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  __unroll::assign(src, static_cast<F>(0x80), micron::make_index_sequence<M>{});
  return s;
}

// LOW BIT 0x01 - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
low_bit(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = 0x01;
  return src;
}

template <typename F, typename M = u64>
F &
rlow_bit(F &s, const M cnt) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  for ( M n = 0; n < cnt; n++ ) src[n] = 0x01;
  return s;
}

// LOW BIT 0x01 - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
clow_bit(F *src) noexcept
{
  __unroll::assign(src, static_cast<F>(0x01), micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rclow_bit(F &s) noexcept
{
  F *src = reinterpret_cast<F *>(&s);
  __unroll::assign(src, static_cast<F>(0x01), micron::make_index_sequence<M>{});
  return s;
}

// SET ALL BITS TO 1 - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
set(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = static_cast<F>(~static_cast<F>(0));
  return src;
}

template <typename F, typename M = u64>
F &
rset(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] = static_cast<F>(~static_cast<F>(0));
  return s;
}

// SET ALL BITS TO 1 - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cset(F *src) noexcept
{
  __unroll::set_all(src, micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcset(F &s) noexcept
{
  __unroll::set_all(&s, micron::make_index_sequence<M>{});
  return s;
}

// CLEAR ALL BITS TO 0 - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
clear(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = 0;
  return src;
}

template <typename F, typename M = u64>
F &
rclear(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] = 0;
  return s;
}

// CLEAR ALL BITS TO 0 - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cclear(F *src) noexcept
{
  __unroll::assign(src, F(0), micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcclear(F &s) noexcept
{
  __unroll::assign(&s, F(0), micron::make_index_sequence<M>{});
  return s;
}

// APPLY CONSTANT MASK - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
mask(F *src, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = static_cast<F>(m);
  return src;
}

template <typename F, typename M = u64>
F &
rmask(F &s, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] = static_cast<F>(m);
  return s;
}

// APPLY CONSTANT MASK - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cmask(F *src, const u8 m) noexcept
{
  __unroll::assign(src, static_cast<F>(m), micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcmask(F &s, const u8 m) noexcept
{
  __unroll::assign(&s, static_cast<F>(m), micron::make_index_sequence<M>{});
  return s;
}

// BITWISE INVERT - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
invert(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] = ~src[n];
  return src;
}

template <typename F, typename M = u64>
__attribute__((nonnull)) F *
not_(F *src, const M cnt) noexcept
{
  return invert(src, cnt);
}

template <typename F, typename M = u64>
F &
rinvert(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] = ~s[n];
  return s;
}

// BITWISE INVERT - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cinvert(F *src) noexcept
{
  __unroll::invert(src, micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcinvert(F &s) noexcept
{
  __unroll::invert(&s, micron::make_index_sequence<M>{});
  return s;
}

// BITWISE AND MASK - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
and_mask(F *src, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] &= m;
  return src;
}

template <typename F, typename M = u64>
F &
rand_mask(F &s, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] &= m;
  return s;
}

// BITWISE AND MASK - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cand_mask(F *src, const u8 m) noexcept
{
  __unroll::and_mask(src, m, micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcand_mask(F &s, const u8 m) noexcept
{
  __unroll::and_mask(&s, m, micron::make_index_sequence<M>{});
  return s;
}

// BITWISE OR MASK - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
or_mask(F *src, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] |= m;
  return src;
}

template <typename F, typename M = u64>
F &
ror_mask(F &s, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] |= m;
  return s;
}

// BITWISE OR MASK - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cor_mask(F *src, const u8 m) noexcept
{
  __unroll::or_mask(src, m, micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcor_mask(F &s, const u8 m) noexcept
{
  __unroll::or_mask(&s, m, micron::make_index_sequence<M>{});
  return s;
}

// BITWISE XOR MASK - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
xor_mask(F *src, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] ^= m;
  return src;
}

template <typename F, typename M = u64>
F &
rxor_mask(F &s, const u8 m, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] ^= m;
  return s;
}

// BITWISE XOR MASK - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cxor_mask(F *src, const u8 m) noexcept
{
  __unroll::xor_mask(src, m, micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcxor_mask(F &s, const u8 m) noexcept
{
  __unroll::xor_mask(&s, m, micron::make_index_sequence<M>{});
  return s;
}

// INCREMENT - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
increment(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) ++src[n];
  return src;
}

template <typename F, typename M = u64>
F &
rincrement(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) ++s[n];
  return s;
}

// INCREMENT - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cincrement(F *src) noexcept
{
  __unroll::increment(src, micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcincrement(F &s) noexcept
{
  __unroll::increment(&s, micron::make_index_sequence<M>{});
  return s;
}

// DECREMENT - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
decrement(F *src, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) --src[n];
  return src;
}

template <typename F, typename M = u64>
F &
rdecrement(F &s, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) --s[n];
  return s;
}

// DECREMENT - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cdecrement(F *src) noexcept
{
  __unroll::decrement(src, micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcdecrement(F &s) noexcept
{
  __unroll::decrement(&s, micron::make_index_sequence<M>{});
  return s;
}

// ADD CONSTANT - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
add(F *src, const u8 v, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] += v;
  return src;
}

template <typename F, typename M = u64>
F &
radd(F &s, const u8 v, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] += v;
  return s;
}

// ADD CONSTANT - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
cadd(F *src, const u8 v) noexcept
{
  __unroll::add(src, v, micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcadd(F &s, const u8 v) noexcept
{
  __unroll::add(&s, v, micron::make_index_sequence<M>{});
  return s;
}

// SUBTRACT CONSTANT - RUNTIME COUNT
template <typename F, typename M = u64>
__attribute__((nonnull)) F *
sub(F *src, const u8 v, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) src[n] -= v;
  return src;
}

template <typename F, typename M = u64>
F &
rsub(F &s, const u8 v, const M cnt) noexcept
{
  for ( M n = 0; n < cnt; n++ ) s[n] -= v;
  return s;
}

// SUBTRACT CONSTANT - COMPILE-TIME COUNT
template <u64 M, typename F>
constexpr F *
csub(F *src, const u8 v) noexcept
{
  __unroll::sub(src, v, micron::make_index_sequence<M>{});
  return src;
}

template <u64 M, typename F>
constexpr F &
rcsub(F &s, const u8 v) noexcept
{
  __unroll::sub(&s, v, micron::make_index_sequence<M>{});
  return s;
}

// MEMORY OBFUSCATION (XOR WITH 0x15)
template <typename F>
__attribute__((nonnull)) F *
memfrob(F *src, u64 n) noexcept
{
  F *a = src;
  while ( n-- > 0 ) *a++ ^= 0x15;
  return a;
}

};     // namespace micron
