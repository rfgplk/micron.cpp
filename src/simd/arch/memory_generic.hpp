//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../types.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// generic (scalar) mem* backend
//
// the arch-free mirror of memory_{amd64,arm32,arm64}.hpp. Selected by __micron_simd_generic, so it
// serves four situations: the generic arch tier, an x86 build below SSE2, an ARM build without
// NEON, and MICRON_NO_SIMD on any arch (a kernel module, where the vector unit exists but is not
// ours to touch).
//
// Semantics are byte-identical to the NEON/SSE backends: `count` is in ELEMENTS of T, the block
// granularity stays 16 bytes so rmemcpy128's bytes/16 arithmetic matches, memcmp* returns the
// signed byte difference, memchr*/memrchr*/memmem* return a pointer or nullptr, and mempcpy*
// returns d + bytes.
//
// WARNING: no __attribute__((vector_size)) type appears here, and none may. A generic vector is
// legal as memory but passing or returning one by value is a hard error under -mno-sse ("SSE
// register return with SSE disabled") and under -mgeneral-regs-only. Those are precisely the flag
// sets this backend exists to serve. Everything below moves machine words.
//
// WARNING: every bulk loop is __micron_no_loop_idiom. GCC's -ftree-loop-distribute-patterns turns
// a byte loop into a call to memcpy/memset -- inside the implementation of memcpy that is either
// infinite recursion or a silent bounce to the weak byte-loop stub in start.cpp. Fixed-size
// __builtin_memcpy is safe (it always expands inline) and is what the block helpers use; the
// residual byte loops must not be allowed to re-form a libcall. start.cpp's own mem* stubs carry
// the same attribute for the same reason.

#if defined(__micron_compiler_gcc) && !defined(__micron_compiler_clang)
#define __micron_no_loop_idiom __attribute__((optimize("-fno-tree-loop-distribute-patterns")))
#else
#define __micron_no_loop_idiom
#endif

namespace micron
{
namespace simd
{

namespace __gen
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// block primitives
//
// fixed-size __builtin_memcpy only: the size is a constant on every call, so it expands to
// loads/stores and never becomes a libcall

__attribute__((always_inline)) inline void
__blk_copy_8(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  __builtin_memcpy(d, s, 8);
}

__attribute__((always_inline)) inline void
__blk_copy_16(u8 *d, const u8 *s) noexcept
{
  __builtin_memcpy(d, s, 16);
}

__attribute__((always_inline)) inline void
__blk_copy_32(u8 *d, const u8 *s) noexcept
{
  __builtin_memcpy(d, s, 32);
}

__attribute__((always_inline)) inline void
__blk_copy_64(u8 *d, const u8 *s) noexcept
{
  __builtin_memcpy(d, s, 64);
}

__attribute__((always_inline)) inline void
__blk_store_w(u8 *d, const u64 w) noexcept
{
  __builtin_memcpy(d, &w, 8);
}

__attribute__((always_inline)) inline u64
__blk_load_w(const u8 *s) noexcept
{
  u64 w;
  __builtin_memcpy(&w, s, 8);
  return w;
}

// splat one byte across a machine word
__attribute__((always_inline)) inline constexpr u64
__splat_byte(const u8 v) noexcept
{
  return static_cast<u64>(v) * 0x0101010101010101ull;
}

// rotate an 8-byte pattern right by `bytes` so that position k of the rotated word carries the
// byte the unrotated pattern would have placed at absolute offset k + bytes
__attribute__((always_inline)) inline u64
__rot_word(const u64 w, const u32 bytes) noexcept
{
  const u32 r = (bytes & 7u) * 8u;
  return r ? ((w >> r) | (w << (64u - r))) : w;
}

// first differing byte of two words, as the signed difference the mem* contract requires.
// only called when the words are known unequal
__attribute__((always_inline)) inline i64
__word_diff(const u64 a, const u64 b) noexcept
{
  const u64 x = a ^ b;
#if defined(__micron_endian_little)
  const u32 idx = static_cast<u32>(__builtin_ctzll(x)) >> 3;
#else
  const u32 idx = static_cast<u32>(__builtin_clzll(x)) >> 3;
#endif
  const u32 sh = idx * 8u;
  const u8 ba = static_cast<u8>((a >> sh) & 0xffu);
  const u8 bb = static_cast<u8>((b >> sh) & 0xffu);
  return static_cast<i64>(static_cast<unsigned>(ba)) - static_cast<i64>(static_cast<unsigned>(bb));
}

// zero-byte detector: nonzero iff some byte of w is zero (Mycroft's bit trick)
__attribute__((always_inline)) inline constexpr u64
__has_zero_byte(const u64 w) noexcept
{
  return (w - 0x0101010101010101ull) & ~w & 0x8080808080808080ull;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the scalar workhorses; everything public below funnels through these

__micron_no_loop_idiom inline void
__copy_fwd(u8 *__restrict d, const u8 *__restrict s, u64 n) noexcept
{
  while ( n >= 64 ) {
    __blk_copy_64(d, s);
    d += 64;
    s += 64;
    n -= 64;
  }
  if ( n >= 32 ) {
    __blk_copy_32(d, s);
    d += 32;
    s += 32;
    n -= 32;
  }
  if ( n >= 16 ) {
    __blk_copy_16(d, s);
    d += 16;
    s += 16;
    n -= 16;
  }
  if ( n >= 8 ) {
    __blk_copy_8(d, s);
    d += 8;
    s += 8;
    n -= 8;
  }
  while ( n-- ) *d++ = *s++;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the MOVE workhorses -- overlap-safe, and separate from the copy pair for one reason
//
// WARNING: __builtin_memcpy is UNDEFINED on overlapping ranges, and it does not promise to read a
// whole block before writing any of it. Using it for the block step of a memmove corrupts the tail
// whenever the overlap is shorter than the block: measured as a byte-shifted result from delta=1,
// n>=129 (tests/rigor/simd_generic.cpp, "memmove: every overlap delta"). The SSE and NEON backends
// never had this problem because a vector block copy IS a load into a register followed by a store.
//
// So each block below is staged through a local buffer: read all B bytes, then write all B bytes.
// GCC keeps the buffer in registers at -O2, which is the same shape the vector backends emit. With
// that, forward is correct for d < s and backward for d > s at any overlap distance: the block we
// are about to clobber has already been read, and every byte still owed lies outside the range we
// write.

template<u64 B>
__attribute__((always_inline)) inline void
__blk_move(u8 *d, const u8 *s) noexcept
{
  u8 t[B];
  __builtin_memcpy(t, s, B);
  __builtin_memcpy(d, t, B);
}

__micron_no_loop_idiom inline void
__move_fwd(u8 *d, const u8 *s, u64 n) noexcept
{
  while ( n >= 32 ) {
    __blk_move<32>(d, s);
    d += 32;
    s += 32;
    n -= 32;
  }
  if ( n >= 16 ) {
    __blk_move<16>(d, s);
    d += 16;
    s += 16;
    n -= 16;
  }
  if ( n >= 8 ) {
    __blk_move<8>(d, s);
    d += 8;
    s += 8;
    n -= 8;
  }
  while ( n-- ) *d++ = *s++;
}

__micron_no_loop_idiom inline void
__move_bwd(u8 *d, const u8 *s, u64 n) noexcept
{
  d += n;
  s += n;
  while ( n >= 32 ) {
    d -= 32;
    s -= 32;
    n -= 32;
    __blk_move<32>(d, s);
  }
  if ( n >= 16 ) {
    d -= 16;
    s -= 16;
    n -= 16;
    __blk_move<16>(d, s);
  }
  if ( n >= 8 ) {
    d -= 8;
    s -= 8;
    n -= 8;
    __blk_move<8>(d, s);
  }
  while ( n ) {
    --d;
    --s;
    --n;
    *d = *s;
  }
}

__micron_no_loop_idiom inline void
__set_fwd(u8 *__restrict d, const u8 v, u64 n) noexcept
{
  const u64 w = __splat_byte(v);
  while ( n >= 32 ) {
    __blk_store_w(d, w);
    __blk_store_w(d + 8, w);
    __blk_store_w(d + 16, w);
    __blk_store_w(d + 24, w);
    d += 32;
    n -= 32;
  }
  while ( n >= 8 ) {
    __blk_store_w(d, w);
    d += 8;
    n -= 8;
  }
  while ( n-- ) *d++ = v;
}

// splat the 8-byte pattern `w` so byte k of the region carries ((u8 *)&w)[k % 8]
__micron_no_loop_idiom inline void
__wordset_fwd(u8 *__restrict d, const u64 w, u64 n) noexcept
{
  u64 i = 0;
  while ( i + 8 <= n ) {
    __blk_store_w(d + i, w);
    i += 8;
  }
  if ( i < n ) {
    u8 tail[8];
    __builtin_memcpy(tail, &w, 8);
    for ( u64 k = 0; i < n; ++i, ++k ) d[i] = tail[k];
  }
}

__micron_no_loop_idiom inline i64
__cmp_fwd(const u8 *s, const u8 *d, u64 n) noexcept
{
  u64 i = 0;
  while ( i + 8 <= n ) {
    const u64 a = __blk_load_w(s + i);
    const u64 b = __blk_load_w(d + i);
    if ( a != b ) return __word_diff(a, b);
    i += 8;
  }
  for ( ; i < n; ++i )
    if ( s[i] != d[i] ) return static_cast<i64>(static_cast<unsigned>(s[i])) - static_cast<i64>(static_cast<unsigned>(d[i]));
  return 0;
}

};      // namespace __gen

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcpy

template<typename T>
__attribute__((nonnull)) T *
memcpy128(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memcpy128 requires trivially copyable type");
  __gen::__copy_fwd(reinterpret_cast<u8 *>(dest), reinterpret_cast<const u8 *>(src), count * sizeof(T));
  return dest;
}

template<typename T>
__attribute__((nonnull)) T *
amemcpy128(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemcpy128 requires trivially copyable type");
  __gen::__copy_fwd(reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 16)),
                    reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 16)), count * sizeof(T));
  return dest;
}

// no non-temporal store without an ISA that has one; the cache-bypass hint degrades to a plain copy
template<typename T>
__attribute__((nonnull)) T *
ntmemcpy128(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  return memcpy128<T>(dest, src, count);
}

template<typename F, typename D>
F &
rmemcpy128(F &__restrict dest, const D &__restrict src, const u64 cnt) noexcept
{
  __gen::__copy_fwd(reinterpret_cast<u8 *>(&dest), reinterpret_cast<const u8 *>(&src), cnt * sizeof(D));
  return dest;
}

template<typename T>
__attribute__((nonnull)) T *
mempcpy128(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "mempcpy128 requires trivially copyable type");
  const u64 bytes = count * sizeof(T);
  auto *d = reinterpret_cast<u8 *>(dest);
  __gen::__copy_fwd(d, reinterpret_cast<const u8 *>(src), bytes);
  return reinterpret_cast<T *>(d + bytes);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memmove

template<typename T>
__attribute__((nonnull)) T *
memmove128(T *dest, const T *src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memmove128 requires trivially copyable type");
  auto *d = reinterpret_cast<u8 *>(dest);
  const auto *s = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);
  if ( d == s or bytes == 0 ) return dest;
  if ( d < s )
    __gen::__move_fwd(d, s, bytes);
  else
    __gen::__move_bwd(d, s, bytes);
  return dest;
}

template<typename T>
__attribute__((nonnull)) T *
amemmove128(T *dest, const T *src, const u64 count) noexcept
{
  return memmove128<T>(dest, src, count);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memset

template<typename T>
__attribute__((nonnull)) T *
memset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  __gen::__set_fwd(reinterpret_cast<u8 *>(src), in, count * sizeof(T));
  return src;
}

template<typename T>
__attribute__((nonnull)) T *
amemset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  __gen::__set_fwd(reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 16)), in, count * sizeof(T));
  return src;
}

template<typename T>
__attribute__((nonnull)) T *
ntmemset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  return memset128<T>(src, in, count);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wordset - splat a u64 across the buffer (bytes count)

__attribute__((nonnull)) static inline u8 *
wordset128(u8 *__restrict src, const u64 in, const u64 bytes) noexcept
{
  __gen::__wordset_fwd(src, in, bytes);
  return src;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcmp

template<typename T>
__attribute__((nonnull)) i64
memcmp128(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  return __gen::__cmp_fwd(reinterpret_cast<const u8 *>(src), reinterpret_cast<const u8 *>(dest), count * sizeof(T));
}

template<typename T>
__attribute__((nonnull)) i64
amemcmp128(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  return __gen::__cmp_fwd(reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 16)),
                          reinterpret_cast<const u8 *>(__builtin_assume_aligned(dest, 16)), count * sizeof(T));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memchr - byte search

template<typename T>
__attribute__((nonnull)) T *
memchr128(const T *src, u8 c, const u64 count) noexcept
{
  const auto *p = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 key = __gen::__splat_byte(c);

  u64 i = 0;
  while ( i + 8 <= bytes ) {
    const u64 w = __gen::__blk_load_w(p + i) ^ key;
    if ( __gen::__has_zero_byte(w) ) {
      const u64 limit = i + 8;
      for ( ; i < limit; ++i )
        if ( p[i] == c ) return const_cast<T *>(reinterpret_cast<const T *>(p + i));
    }
    i += 8;
  }
  for ( ; i < bytes; ++i )
    if ( p[i] == c ) return const_cast<T *>(reinterpret_cast<const T *>(p + i));

  return nullptr;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memrchr - reverse byte search

template<typename T>
__attribute__((nonnull)) T *
memrchr128(const T *src, u8 c, const u64 count) noexcept
{
  const auto *p = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 key = __gen::__splat_byte(c);

  u64 i = bytes;
  while ( i >= 8 ) {
    const u64 base = i - 8;
    const u64 w = __gen::__blk_load_w(p + base) ^ key;
    if ( __gen::__has_zero_byte(w) ) {
      for ( u64 j = i; j > base; --j )
        if ( p[j - 1] == c ) return const_cast<T *>(reinterpret_cast<const T *>(p + j - 1));
    }
    i = base;
  }
  while ( i > 0 ) {
    --i;
    if ( p[i] == c ) return const_cast<T *>(reinterpret_cast<const T *>(p + i));
  }

  return nullptr;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memmem - substring search

template<typename T>
T *
memmem128(const T *hay, const u64 hlen, const T *nee, const u64 nlen) noexcept
{
  const auto *h = reinterpret_cast<const u8 *>(hay);
  const auto *ne = reinterpret_cast<const u8 *>(nee);
  const u64 hbytes = hlen * sizeof(T);
  const u64 nbytes = nlen * sizeof(T);

  if ( nbytes == 0 ) return const_cast<T *>(hay);
  if ( nbytes > hbytes ) return nullptr;

  const u64 limit = hbytes - nbytes + 1;
  const u8 first = ne[0];

  for ( u64 i = 0; i < limit; ++i ) {
    if ( h[i] != first ) continue;
    if ( nbytes == 1 ) return const_cast<T *>(reinterpret_cast<const T *>(h + i));
    if ( __gen::__cmp_fwd(h + i + 1, ne + 1, nbytes - 1) == 0 ) return const_cast<T *>(reinterpret_cast<const T *>(h + i));
  }

  return nullptr;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 256- and 512-bit entry points
//
// On a real ISA these name a register width. Here they name only the alignment the caller promises;
// the work is the same word loop, so each forwards to the 128 form rather than duplicating it.
// They exist so micron::memcpy256 and friends stay declared on this tier -- cmemory.hpp re-exports
// them into micron::, and dropping the names would silently break user code that a wider build
// compiles fine.

#define __MICRON_GEN_WIDE(W, A)                                                                                                            \
  template<typename T>                                                                                                                     \
  __attribute__((nonnull)) T *memcpy##W(T *__restrict d, const T *__restrict s, const u64 n) noexcept                                       \
  {                                                                                                                                        \
    return memcpy128<T>(d, s, n);                                                                                                          \
  }                                                                                                                                        \
  template<typename T> __attribute__((nonnull)) T *amemcpy##W(T *__restrict d, const T *__restrict s, const u64 n) noexcept                 \
  {                                                                                                                                        \
    __gen::__copy_fwd(reinterpret_cast<u8 *>(__builtin_assume_aligned(d, A)),                                                               \
                      reinterpret_cast<const u8 *>(__builtin_assume_aligned(s, A)), n * sizeof(T));                                         \
    return d;                                                                                                                              \
  }                                                                                                                                        \
  template<typename T> __attribute__((nonnull)) T *ntmemcpy##W(T *__restrict d, const T *__restrict s, const u64 n) noexcept                \
  {                                                                                                                                        \
    return memcpy128<T>(d, s, n);                                                                                                          \
  }                                                                                                                                        \
  template<typename F, typename D> F &rmemcpy##W(F &__restrict d, const D &__restrict s, const u64 n) noexcept                              \
  {                                                                                                                                        \
    return rmemcpy128<F, D>(d, s, n);                                                                                                       \
  }                                                                                                                                        \
  template<typename T> __attribute__((nonnull)) T *memmove##W(T *d, const T *s, const u64 n) noexcept                                       \
  {                                                                                                                                        \
    return memmove128<T>(d, s, n);                                                                                                          \
  }                                                                                                                                        \
  template<typename T> __attribute__((nonnull)) T *amemmove##W(T *d, const T *s, const u64 n) noexcept                                      \
  {                                                                                                                                        \
    return memmove128<T>(d, s, n);                                                                                                          \
  }                                                                                                                                        \
  template<typename T> __attribute__((nonnull)) T *memset##W(T *__restrict d, const u8 v, const u64 n) noexcept                             \
  {                                                                                                                                        \
    return memset128<T>(d, v, n);                                                                                                           \
  }                                                                                                                                        \
  template<typename T> __attribute__((nonnull)) T *amemset##W(T *__restrict d, const u8 v, const u64 n) noexcept                            \
  {                                                                                                                                        \
    __gen::__set_fwd(reinterpret_cast<u8 *>(__builtin_assume_aligned(d, A)), v, n * sizeof(T));                                             \
    return d;                                                                                                                              \
  }                                                                                                                                        \
  template<typename T> __attribute__((nonnull)) T *ntmemset##W(T *__restrict d, const u8 v, const u64 n) noexcept                           \
  {                                                                                                                                        \
    return memset128<T>(d, v, n);                                                                                                           \
  }                                                                                                                                        \
  template<typename T> __attribute__((nonnull)) i64 memcmp##W(const T *__restrict a, const T *__restrict b, const u64 n) noexcept           \
  {                                                                                                                                        \
    return memcmp128<T>(a, b, n);                                                                                                           \
  }                                                                                                                                        \
  template<typename T> __attribute__((nonnull)) i64 amemcmp##W(const T *__restrict a, const T *__restrict b, const u64 n) noexcept          \
  {                                                                                                                                        \
    return __gen::__cmp_fwd(reinterpret_cast<const u8 *>(__builtin_assume_aligned(a, A)),                                                   \
                            reinterpret_cast<const u8 *>(__builtin_assume_aligned(b, A)), n * sizeof(T));                                   \
  }

__MICRON_GEN_WIDE(256, 32)
__MICRON_GEN_WIDE(512, 64)

#undef __MICRON_GEN_WIDE

// the search/scan family exists at 256 only, mirroring the amd64 backend's surface

template<typename T>
__attribute__((nonnull)) T *
memchr256(const T *src, u8 c, const u64 count) noexcept
{
  return memchr128<T>(src, c, count);
}

template<typename T>
__attribute__((nonnull)) T *
memrchr256(const T *src, u8 c, const u64 count) noexcept
{
  return memrchr128<T>(src, c, count);
}

template<typename T>
T *
memmem256(const T *hay, const u64 hlen, const T *nee, const u64 nlen) noexcept
{
  return memmem128<T>(hay, hlen, nee, nlen);
}

template<typename T>
__attribute__((nonnull)) T *
mempcpy256(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  return mempcpy128<T>(dest, src, count);
}

__attribute__((nonnull)) static inline u8 *
wordset256(u8 *__restrict src, const u64 in, const u64 bytes) noexcept
{
  return wordset128(src, in, bytes);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bulk entry points
//
// cmemory's size ladders call these above their small-copy tier. The NEON/SSE backends split
// aligned/unaligned and temporal/non-temporal here; a scalar target has neither distinction to
// make, so all of them route to the same word loop.

__attribute__((nonnull)) inline u8 *
__memset_bulk(u8 *__restrict d, const u8 v, const u64 n) noexcept
{
  __gen::__set_fwd(d, v, n);
  return d;
}

__attribute__((nonnull)) inline u8 *
__wordset_bulk(u8 *__restrict d, const u64 w, const u64 n) noexcept
{
  __gen::__wordset_fwd(d, w, n);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memcpy_bulk(u8 *__restrict d, const u8 *__restrict s, const u64 n) noexcept
{
  __gen::__copy_fwd(d, s, n);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memmove_bulk_fwd(u8 *d, const u8 *s, const u64 n) noexcept
{
  __gen::__move_fwd(d, s, n);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memmove_bulk_bwd(u8 *d, const u8 *s, const u64 n) noexcept
{
  __gen::__move_bwd(d, s, n);
  return d;
}

};      // namespace simd
};      // namespace micron
