//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../defs.hpp"

#if defined(__micron_arch_x86_any)
#include "../simd/memory.hpp"
#endif

#include "../math/generic.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"

#include "../except.hpp"
#include "../type_traits.hpp"

namespace micron
{

// NOTE: these distance functions only calculate the difference between contiguous memory pointers, they don't (and
// likely never will) support generalized iterators

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// distances

template <typename T, typename F>
  requires micron::is_pointer_v<T> && micron::is_pointer_v<F>
inline size_t
distance(T a, F b)     // distance between two pointers, can be of different types
{
  return (reinterpret_cast<word *>(b) - reinterpret_cast<word *>(a));
}

template <typename T, typename F>
  requires micron::is_pointer_v<T> && micron::is_pointer_v<F>
inline size_t
adistance(T a, F b)     // absolute distance between two pointers, can be of different types
{
  return math::abs(reinterpret_cast<word *>(b) - reinterpret_cast<word *>(a));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// destroys

template <typename T>
inline void
destroy(T &mem)
  requires(!micron::is_array<T>::value)
{
  if constexpr ( micron::is_class<T>::value ) {
    mem.~T();
  } else if constexpr ( micron::is_pointer<T>::value && !micron::is_null_pointer<T>::value && !micron::is_array<T>::value ) {
    delete mem;
  }
}

template <typename T, size_t N>
inline void
destroy(T &mem)
  requires micron::is_array<T>::value
{
  if constexpr ( micron::is_class<T>::value ) {
    for ( size_t i = 0; i < N; i++ )
      mem[i].~T();
  } else if constexpr ( micron::is_pointer<T>::value && !micron::is_null_pointer<T>::value ) {
    delete[] mem;
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// overwrite

template <typename T, typename F>
void
overwrite(T &src, F &dest)
{
  micron::bytecpy(&src, &dest, dest.size());
  // NOTE: yes it's supposed to look like this
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// zeros

template <size_t N, typename T>
void
zero(T *src)
{
  czero<N>(src);
}

template <typename T, size_t M>
void
zero(char (*ptr)[M])
{
  czero<M>(ptr);
}

template <typename T>
void
zero(T *src)
{
  bset(src, 0x0, sizeof(T));
}

template <typename T>
inline bool
is_zero(const T *src)
{
  if ( src == nullptr )
    exc<except::library_error>("micron::is_zero invalid pointer.");
  const byte *b_ptr = reinterpret_cast<const byte *>(src);
  for ( size_t i = 0; i < sizeof(T); i++ )
    if ( *b_ptr++ > 0x0 )
      return false;
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// copy_n

template <typename N, typename T, typename F>
F *
copy_n(const T *restrict src, F *restrict dst, const N cnt)
{
  if ( src == nullptr or dst == nullptr )
    exc<except::library_error>("micron::copy_n invalid pointers.");
#if __micron_x86_simd_width >= 256
  if ( cnt % 32 == 0 )
    simd::memcpy256(dst, src, cnt);
  else if ( cnt % 16 == 0 )
    simd::memcpy128(dst, src, cnt);
  else
    memcpy(dst, src, cnt);
#elif __micron_x86_simd_width >= 128
  if ( cnt % 16 == 0 )
    simd::memcpy128(dst, src, cnt);
  else
    memcpy(dst, src, cnt);
#else
  memcpy(dst, src, cnt);
#endif
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// copy

template <size_t N, typename T, typename F>
F *
copy(const T *restrict src, F *restrict dst)
{
  if ( src == nullptr or dst == nullptr )
    exc<except::library_error>("micron::copy invalid pointers.");
#if __micron_x86_simd_width >= 256
  if constexpr ( N <= 32 )
    _memcpy_32(dst, src, N);
  else if constexpr ( N % 32 == 0 )
    simd::memcpy256(dst, src, N);
  else if constexpr ( N % 16 == 0 )
    simd::memcpy128(dst, src, N);
  else
    cmemcpy<N>(dst, src);
#elif __micron_x86_simd_width >= 128
  if constexpr ( N <= 32 )
    _memcpy_32(dst, src, N);
  else if constexpr ( N % 16 == 0 )
    simd::memcpy128(dst, src, N);
  else
    cmemcpy<N>(dst, src);
#else
  cmemcpy<N>(dst, src);
#endif
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// copy

template <size_t N, typename T, typename F>
F &
copy(const T &restrict src, F &restrict dst)
{
#if __micron_x86_simd_width >= 256
  if constexpr ( N <= 32 )
    _memcpy_32(dst, src, N);
  else if constexpr ( N % 32 == 0 )
    simd::rmemcpy256(dst, src, N);
  else if constexpr ( N % 16 == 0 )
    simd::rmemcpy128(dst, src, N);
  else
    crmemcpy<N>(dst, src);
#elif __micron_x86_simd_width >= 128
  if constexpr ( N <= 32 )
    _memcpy_32(dst, src, N);
  else if constexpr ( N % 16 == 0 )
    simd::rmemcpy128(dst, src, N);
  else
    crmemcpy<N>(dst, src);
#else
  crmemcpy<N>(dst, src);
#endif
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// copy

template <size_t N, typename T, typename F>
F *
copy(T *restrict start_src, T *restrict end_src, F *restrict dest)
{
  if ( (end_src - start_src) < 0 )
    exc<except::library_error>("micron::copy invalid pointers.");
  const size_t len = static_cast<size_t>(end_src - start_src);
#if __micron_x86_simd_width >= 256
  if ( N % 32 == 0 )
    simd::memcpy256(dest, start_src, len);
  else if ( N % 16 == 0 )
    simd::memcpy128(dest, start_src, len);
  else
    memcpy(dest, start_src, len);
#elif __micron_x86_simd_width >= 128
  if ( N % 16 == 0 )
    simd::memcpy128(dest, start_src, len);
  else
    memcpy(dest, start_src, len);
#else
  memcpy(dest, start_src, len);
#endif
  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ccopys

template <size_t N, typename T, typename F>
F *
ccopy(const T *restrict src, F *restrict dst)
{
  if ( src == nullptr or dst == nullptr )
    exc<except::library_error>("micron::ccopy invalid pointers.");
  if constexpr ( micron::is_class<T>::value ) {
    for ( size_t i = 0; i < N; i++ )
      dst[i] = src[i];
    return dst;
  }
#if __micron_x86_simd_width >= 256
  if constexpr ( N <= 32 )
    _memcpy_32(dst, src, N);
  else if constexpr ( N % 32 == 0 )
    simd::memcpy256(dst, src, N);
  else if constexpr ( N % 16 == 0 )
    simd::memcpy128(dst, src, N);
  else
    cmemcpy<N>(dst, src);
#elif __micron_x86_simd_width >= 128
  if constexpr ( N <= 32 )
    _memcpy_32(dst, src, N);
  else if constexpr ( N % 16 == 0 )
    simd::memcpy128(dst, src, N);
  else
    cmemcpy<N>(dst, src);
#else
  cmemcpy<N>(dst, src);
#endif
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scopys

template <typename T>
void
scopy(const T &restrict src, T &restrict dst)
{
  auto smlr = src.size() < dst.size() ? src.size() : dst.size();
  if constexpr ( micron::is_class_v<T> or micron::is_object_v<T> ) {
    for ( size_t i = 0; i < smlr; i++ )
      dst[i] = src[i];
    return;
  }
  smlr = smlr * sizeof(typename T::value_type);
#if __micron_x86_simd_width >= 256
  if ( smlr <= 32 )
    _memcpy_32(dst, src, smlr);
  else if ( smlr % 32 == 0 )
    simd::memcpy256(dst, src, smlr);
  else if ( smlr % 16 == 0 )
    simd::memcpy128(dst, src, smlr);
  else
    memcpy(dst, src, smlr);
#elif __micron_x86_simd_width >= 128
  if ( smlr <= 32 )
    _memcpy_32(dst, src, smlr);
  else if ( smlr % 16 == 0 )
    simd::memcpy128(dst, src, smlr);
  else
    memcpy(dst, src, smlr);
#else
  memcpy(dst, src, smlr);
#endif
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cmoves

template <size_t N, typename T, typename F>
F *
cmove(const T *restrict src, F *restrict dst)
{
  if ( src == nullptr or dst == nullptr )
    exc<except::library_error>("micron::cmove invalid pointers.");
  if constexpr ( micron::is_class<T>::value ) {
    for ( size_t i = 0; i < N; i++ )
      dst[i] = micron::move(src[i]);
    return dst;
  }
#if __micron_x86_simd_width >= 256
  if constexpr ( N <= 32 )
    _memcpy_32(dst, src, N);
  else if constexpr ( N % 32 == 0 )
    simd::memcpy256(dst, src, N);
  else if constexpr ( N % 16 == 0 )
    simd::memcpy128(dst, src, N);
  else
    cmemcpy<N>(dst, src);
#elif __micron_x86_simd_width >= 128
  if constexpr ( N <= 32 )
    _memcpy_32(dst, src, N);
  else if constexpr ( N % 16 == 0 )
    simd::memcpy128(dst, src, N);
  else
    cmemcpy<N>(dst, src);
#else
  cmemcpy<N>(dst, src);
#endif
  czero<N>(src);
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cmoves

template <typename T, typename F>
void
cmove(T &&src, const F &dst)
{
  dst = micron::move(src);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// move_ns

template <typename N, typename T, typename F>
F *
move_n(T *restrict src, F *restrict dst, const N cnt)
{
  if ( src == nullptr or dst == nullptr )
    exc<except::library_error>("micron::move_n invalid pointers.");
#if __micron_x86_simd_width >= 256
  if ( cnt % 32 == 0 )
    simd::memcpy256(dst, src, cnt);
  else if ( cnt % 16 == 0 )
    simd::memcpy128(dst, src, cnt);
  else
    memcpy(dst, src, cnt);
#elif __micron_x86_simd_width >= 128
  if ( cnt % 16 == 0 )
    simd::memcpy128(dst, src, cnt);
  else
    memcpy(dst, src, cnt);
#else
  memcpy(dst, src, cnt);
#endif
  byteset(src, 0x0, cnt);
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// set_ns

template <typename T, typename N>
T *
set_n(T *restrict dst, const byte val, const N cnt)
{
  if ( dst == nullptr )
    exc<except::library_error>("micron::set_n invalid pointer.");
#if __micron_x86_simd_width >= 256
  if ( cnt % 32 == 0 )
    memset256(dst, val, cnt);
  else if ( cnt % 16 == 0 )
    memset128(dst, val, cnt);
  else
    memset(dst, val, cnt);
#elif __micron_x86_simd_width >= 128
  if ( cnt % 16 == 0 )
    memset128(dst, val, cnt);
  else
    memset(dst, val, cnt);
#else
  memset(dst, val, cnt);
#endif
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cset_ns

template <size_t N, typename T>
T *
cset_n(T *restrict dst, const byte val)
{
  if ( dst == nullptr )
    exc<except::library_error>("micron::cset_n invalid pointer.");
#if __micron_x86_simd_width >= 256
  if constexpr ( N % 32 == 0 )
    memset256(dst, val, N);
  else if constexpr ( N % 16 == 0 )
    memset128(dst, val, N);
  else
    cmemset<N>(dst, val);
#elif __micron_x86_simd_width >= 128
  if constexpr ( N % 16 == 0 )
    memset128(dst, val, N);
  else
    cmemset<N>(dst, val);
#else
  cmemset<N>(dst, val);
#endif
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fill_ns

template <typename T, typename N>
T *
fill_n(T *restrict dst, const T val, const N cnt)
{
  if ( dst == nullptr )
    exc<except::library_error>("micron::fill_n invalid pointer.");
  if constexpr ( micron::is_class_v<T> ) {
    for ( N i = 0; i < cnt; i++ )
      dst[i] = val;
    return dst;
  }
  typeset(dst, val, cnt);
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cfill_ns

template <size_t N, typename T>
T *
cfill_n(T *restrict dst, const T val)
{
  if ( dst == nullptr )
    exc<except::library_error>("micron::cfill_n invalid pointer.");
  if constexpr ( micron::is_class_v<T> ) {
    for ( size_t i = 0; i < N; i++ )
      dst[i] = val;
    return dst;
  }
  ctypeset<N>(dst, val);
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// zero_ns

template <typename T, typename N>
T *
zero_n(T *restrict dst, const N cnt)
{
  if ( dst == nullptr )
    exc<except::library_error>("micron::zero_n invalid pointer.");
#if __micron_x86_simd_width >= 256
  if ( cnt % 32 == 0 )
    memset256(dst, 0x0, cnt);
  else if ( cnt % 16 == 0 )
    memset128(dst, 0x0, cnt);
  else
    memset(dst, 0x0, cnt);
#elif __micron_x86_simd_width >= 128
  if ( cnt % 16 == 0 )
    memset128(dst, 0x0, cnt);
  else
    memset(dst, 0x0, cnt);
#else
  memset(dst, 0x0, cnt);
#endif
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// czero_ns

template <size_t N, typename T>
T *
czero_n(T *restrict dst)
{
  if ( dst == nullptr )
    exc<except::library_error>("micron::czero_n invalid pointer.");
#if __micron_x86_simd_width >= 256
  if constexpr ( N % 32 == 0 )
    memset256(dst, 0x0, N);
  else if constexpr ( N % 16 == 0 )
    memset128(dst, 0x0, N);
  else
    cmemset<N>(dst, 0x0);
#elif __micron_x86_simd_width >= 128
  if constexpr ( N % 16 == 0 )
    memset128(dst, 0x0, N);
  else
    cmemset<N>(dst, 0x0);
#else
  cmemset<N>(dst, 0x0);
#endif
  return dst;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compare_ns

template <typename N, typename T>
i64
compare_n(const T *restrict src, const T *restrict dst, const N cnt)
{
  if ( src == nullptr or dst == nullptr )
    exc<except::library_error>("micron::compare_n invalid pointers.");
#if __micron_x86_simd_width >= 256
  if ( cnt % 32 == 0 )
    return simd::memcmp256(src, dst, cnt);
  else if ( cnt % 16 == 0 )
    return simd::memcmp128(src, dst, cnt);
  else
    return bytecmp(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dst), cnt);
#elif __micron_x86_simd_width >= 128
  if ( cnt % 16 == 0 )
    return simd::memcmp128(src, dst, cnt);
  else
    return bytecmp(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dst), cnt);
#else
  return bytecmp(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dst), cnt);
#endif
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ccompare_ns

template <size_t N, typename T>
i64
ccompare_n(const T *restrict src, const T *restrict dst)
{
  if ( src == nullptr or dst == nullptr )
    exc<except::library_error>("micron::ccompare_n invalid pointers.");
#if __micron_x86_simd_width >= 256
  if constexpr ( N % 32 == 0 )
    return simd::memcmp256(src, dst, N);
  else if constexpr ( N % 16 == 0 )
    return simd::memcmp128(src, dst, N);
  else
    return cmemcmp<N, byte>(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dst));
#elif __micron_x86_simd_width >= 128
  if constexpr ( N % 16 == 0 )
    return simd::memcmp128(src, dst, N);
  else
    return cmemcmp<N, byte>(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dst));
#else
  return cmemcmp<N, byte>(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dst));
#endif
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// equal_ns

template <typename N, typename T>
bool
equal_n(const T *restrict src, const T *restrict dst, const N cnt)
{
  return compare_n(src, dst, cnt) == 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cequal_ns

template <size_t N, typename T>
bool
cequal_n(const T *restrict src, const T *restrict dst)
{
  return ccompare_n<N>(src, dst) == 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// swap_ns

template <typename N, typename T>
void
swap_n(T *restrict a, T *restrict b, const N cnt)
{
  if ( a == nullptr or b == nullptr )
    exc<except::library_error>("micron::swap_n invalid pointers.");
  byte tmp[32];
  byte *pa = reinterpret_cast<byte *>(a);
  byte *pb = reinterpret_cast<byte *>(b);
  N remaining = cnt;
  while ( remaining >= 32 ) {
    _memcpy_32(tmp, pa, 32);
    _memcpy_32(pa, pb, 32);
    _memcpy_32(pb, tmp, 32);
    pa += 32;
    pb += 32;
    remaining -= 32;
  }
  for ( N i = 0; i < remaining; i++ ) {
    byte t = pa[i];
    pa[i] = pb[i];
    pb[i] = t;
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cswap_ns

template <size_t N, typename T>
void
cswap_n(T *restrict a, T *restrict b)
{
  if ( a == nullptr or b == nullptr )
    exc<except::library_error>("micron::cswap_n invalid pointers.");
  byte tmp[N];
  cmemcpy<N>(tmp, reinterpret_cast<const byte *>(a));
  cmemcpy<N>(reinterpret_cast<byte *>(a), reinterpret_cast<const byte *>(b));
  cmemcpy<N>(reinterpret_cast<byte *>(b), tmp);
}

};     // namespace micron
