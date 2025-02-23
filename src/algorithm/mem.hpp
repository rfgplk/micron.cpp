#pragma once

#include "../except.hpp"
#include "../math/generic.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"

#include <type_traits>

namespace micron
{

// NOTE: these distance functions only calculate the difference between contiguous memory pointers, they don't (and
// likely never will) support generalized iterators

template <typename T, typename F>
  requires std::is_pointer_v<T> && std::is_pointer_v<F>
inline size_t
distance(T a, F b)     // distance between two pointers, can be of different types
{
  return (reinterpret_cast<word *>(b) - reinterpret_cast<word *>(a));
}

template <typename T, typename F>
  requires std::is_pointer_v<T> && std::is_pointer_v<F>
inline size_t
adistance(T a, F b)     // absolute distance between two pointers, can be of different types
{
  return math::abs(reinterpret_cast<word *>(b) - reinterpret_cast<word *>(a));
}
template <typename T, typename F>
void overwrite(T& src, F& dest)
{
  micron::bytecpy(&src, &dest, dest.size());
  // NOTE: yes it's supposed to look like this
}

template <typename T>
inline void
destroy(T &mem)
  requires(!std::is_array<T>::value)
{
  if constexpr ( std::is_class<T>::value ) {
    mem.~T();
  } else if constexpr ( std::is_pointer<T>::value && !std::is_null_pointer<T>::value && !std::is_array<T>::value ) {
    delete mem;
  }
}

template <typename T, size_t N>
inline void
destroy(T &mem)
  requires std::is_array<T>::value
{
  if constexpr ( std::is_class<T>::value ) {
    for ( size_t i = 0; i < N; i++ )
      mem[i].~T();
  } else if constexpr ( std::is_pointer<T>::value && !std::is_null_pointer<T>::value ) {
    delete[] mem;
  }
}

template <typename N, typename T, typename F>
F *
copy_n(const T *restrict src, F *restrict dst, const N cnt)
{
  // if (cnt <= 32)
  //   _memcpy_32(dst, src, cnt);
  if ( cnt % 16 == 0 and cnt % 32 != 0 )
    memcpy128(dst, src, cnt);
  else if ( cnt % 32 == 0 )
    memcpy256(dst, src, cnt);
  else if ( cnt % 32 != 0 and cnt % 16 != 0 )
    memcpy(dst, src, cnt);
  return dst;
}
template <size_t N, typename T>
void
zero(const T *src)
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
  const byte *b_ptr = reinterpret_cast<const byte *>(src);
  for ( size_t i = 0; i < sizeof(T); i++ )
    if ( *b_ptr++ > 0x0 )
      return false;
  return true;
}

template <size_t N, typename T, typename F>
F *
cmove(const T *restrict src, F *restrict dst)
{
  if constexpr ( std::is_class<T>::value ) {
    for ( size_t i = 0; i < N; i++ )
      dst[i] = micron::move(src[i]);
  } else {
    if constexpr ( N <= 32 )
      _memcpy_32(dst, src, N);
    if constexpr ( N % 16 == 0 and N % 32 != 0 )
      memcpy128(dst, src, N);
    else if constexpr ( N % 32 == 0 )
      memcpy256(dst, src, N);
    else if constexpr ( N % 32 != 0 and N % 16 != 0 )
      cmemcpy<N>(dst, src);
    czero<N>(src);
  }
  return dst;
}
template <typename T, typename F>
void
cmove(T &&src, const F &dst)
{
  dst = micron::move(src);
}

// copy with const;
template <size_t N, typename T, typename F>
F *
ccopy(const T *restrict src, F *restrict dst)
{
  if constexpr ( std::is_class<T>::value ) {
    for ( size_t i = 0; i < N; i++ )
      dst[i] = src[i];
  } else {
    if constexpr ( N <= 32 )
      _memcpy_32(dst, src, N);
    if constexpr ( N % 16 == 0 and N % 32 != 0 )
      memcpy128(dst, src, N);
    else if constexpr ( N % 32 == 0 )
      memcpy256(dst, src, N);
    else if constexpr ( N % 32 != 0 and N % 16 != 0 )
      cmemcpy<N>(dst, src);
  }
  return dst;
}

// smart copy, if src and dst are objects, perform a deep copy assign call
template <typename T>
void
scopy(const T &restrict src, T &restrict dst)
{
  auto smlr = src.size() < dst.size() ? src.size() : dst.size();
  if constexpr ( std::is_class_v<T> or std::is_object_v<T> ) {
    for ( size_t i = 0; i < smlr; i++ )
      dst[i] = src[i];
    return;
  }
  smlr = smlr * sizeof(typename T::value_type);
  if ( smlr <= 32 )
    _memcpy_32(dst, src, smlr);
  else if ( smlr % 16 == 0 and smlr % 32 != 0 )
    memcpy128(dst, src, smlr);
  else if ( smlr % 32 == 0 )
    memcpy256(dst, src, smlr);
  else if ( smlr % 32 != 0 and smlr % 16 != 0 )
    memcpy(dst, src, smlr);
}

// micron eqv
template <size_t N, typename T, typename F>
F *
copy(const T *restrict src, F *restrict dst)
{
  if constexpr ( N <= 32 )
    _memcpy_32(dst, src, N);
  if constexpr ( N % 16 == 0 and N % 32 != 0 )
    memcpy128(dst, src, N);
  else if constexpr ( N % 32 == 0 )
    memcpy256(dst, src, N);
  else if constexpr ( N % 32 != 0 and N % 16 != 0 )
    cmemcpy<N>(dst, src);
  return dst;
}
template <size_t N, typename T, typename F>
F&
copy(const T&restrict src, F&restrict dst)
{
  if constexpr ( N <= 32 )
    _memcpy_32(dst, src, N);
  if constexpr ( N % 16 == 0 and N % 32 != 0 )
    rmemcpy128(dst, src, N);
  else if constexpr ( N % 32 == 0 )
    rmemcpy256(dst, src, N);
  else if constexpr ( N % 32 != 0 and N % 16 != 0 )
    crmemcpy<N>(dst, src);
  return dst;
}
// C++ standard, from ptr, to ptr, dest pointer
template <size_t N, typename T, typename F>
F *
copy(T *restrict start_src, T *restrict end_src, F *restrict dest)
{
  if ( (end_src - start_src) < 0 )
    throw except::library_error("micron::copy invalid pointers.");
  if ( N % 16 == 0 && N % 32 != 0 )
    memcpy128(dest, start_src, end_src - start_src);
  else if ( N % 32 == 0 )
    memcpy256(dest, start_src, end_src - start_src);
  else
    memcpy(dest, start_src, end_src - start_src);
  return dest;
}
};     // namespace micron
