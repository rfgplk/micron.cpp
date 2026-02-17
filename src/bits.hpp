//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "type_traits.hpp"
#include "types.hpp"

namespace micron
{

template <size_t N>
constexpr inline word __attribute__((always_inline))
repeat_bytes(void)
{
  return ((word)-1 / 0xFF) * N;
}

template <typename T = word>
constexpr inline word __attribute__((always_inline))
has_zero(T a)
{
  // from bithacks
  word b = repeat_bytes<0x01>();
  word c = repeat_bytes<0x80>();
  return (((a - b) & ~a & c) != 0);
}

// check if bytes are equal
template <typename T = word>
constexpr inline word __attribute__((always_inline))
byte_eq(T a, T b)
{
  // from bithacks
  return (has_zero<word>(a ^ b) != 0);
}

inline word __attribute__((always_inline))
get_byte(word a, u32 ind)
{
  return (a >> (ind * byte_width));
}

template <typename T>
bool
aligned_256(const T *ptr)
{
  return !(reinterpret_cast<uintptr_t>(ptr) & 31);
}

template <typename T>
bool
aligned_64(const T *ptr)
{
  return !(reinterpret_cast<uintptr_t>(ptr) & 7);
}

template <typename T>
bool
aligned_32(const T *ptr)
{
  return !(reinterpret_cast<uintptr_t>(ptr) & 3);
}

template <typename T>
bool
aligned_16(const T *ptr)
{
  return !(reinterpret_cast<uintptr_t>(ptr) & 1);
}

template <typename T>
bool
aligned(const T *ptr)
{
  return !(reinterpret_cast<uintptr_t>(ptr) & (micron::alignment_of<T>::value - 1));
}

static const unsigned char BitReverseTable256[256] = {
#define R2(n) n, n + 2 * 64, n + 1 * 64, n + 3 * 64
#define R4(n) R2(n), R2(n + 2 * 16), R2(n + 1 * 16), R2(n + 3 * 16)
#define R6(n) R4(n), R4(n + 2 * 4), R4(n + 1 * 4), R4(n + 3 * 4)
  R6(0), R6(2), R6(1), R6(3)
};

template <typename T>
inline __attribute__((always_inline)) T
rotl32(const T x, const i8 r)
{
  if constexpr ( __has_builtin(__builtin_rotateleft32) )
    return __builtin_rotateleft32(x, r);
  else
    return (x << r) | (x >> (32 - r));
}

template <typename T>
inline __attribute__((always_inline)) T
rotl64(const T x, const i8 r)
{
  if constexpr ( __has_builtin(__builtin_rotateleft32) )
    return __builtin_rotateleft64(x, r);
  else
    return (x << r) | (x >> (64 - r));
}

template <typename T>
constexpr T
reverse_bits(T t)
{
  T c;
  unsigned char *p = (unsigned char *)&t;
  unsigned char *q = (unsigned char *)&c;
  q[3] = BitReverseTable256[p[0]];
  q[2] = BitReverseTable256[p[1]];
  q[1] = BitReverseTable256[p[2]];
  q[0] = BitReverseTable256[p[3]];
  return c;
}

template <typename T>
  requires(sizeof(T) == 1)
constexpr T
reverse_bits_byte(T t)
{
  return (t * 0x0202020202ULL & 0x010884422010ULL) % 1023;
}

template <typename T>
inline constexpr T
flip(T t)
{
  t ^= 0xFF;
  return t;
}

template <int x, typename T>
constexpr bool
set_bit(T t)     // is bit set
{
  return (t | (1 << x));
}

template <int x, typename T>
constexpr bool
bit(T t)     // is bit set
{
  return (t & (1 << x));
}

template <typename T>
constexpr int
bitcount(T x) noexcept     // seriously bitcount is a way more reasonable name, what even is POPcount where are the pops
{
  if constexpr ( micron::is_same_v<T, int> or micron::is_same_v<T, char> or micron::is_same_v<T, unsigned char>
                 or micron::is_same_v<T, unsigned char> or micron::is_same_v<T, unsigned int> or micron::is_same_v<T, short>
                 or micron::is_same_v<T, unsigned short> )
    return __builtin_popcount(x);
  if constexpr ( micron::is_same_v<T, long> or micron::is_same_v<T, unsigned long> )
    return __builtin_popcountl(x);
  if constexpr ( micron::is_same_v<T, long long> or micron::is_same_v<T, unsigned long long> )
    return __builtin_popcountll(x);
}

template <typename T>
constexpr int
popcount(T x) noexcept     // for stl compat
{
  return bitcount(x);
}

template <typename T>
constexpr bool
has_any_bit(T x)
{
  return bitcount(x);
}

template <typename T>
constexpr bool
has_single_bit(T x)
{
  return bitcount(x) == 1;
}

template <typename T>
constexpr int
countl_zero(T x) noexcept
{
  // TODO: expand with numerics
  if constexpr ( micron::is_same_v<T, int> )
    return __builtin_clz(x);
  if constexpr ( micron::is_same_v<T, long> )
    return __builtin_clzl(x);
  if constexpr ( micron::is_same_v<T, long long> )
    return __builtin_clzll(x);
}

template <typename T>
constexpr int
countr_zero(T x) noexcept
{
  // TODO: expand with numerics
  if constexpr ( micron::is_same_v<T, int> )
    return __builtin_ctz(x);
  if constexpr ( micron::is_same_v<T, long> )
    return __builtin_ctzl(x);
  if constexpr ( micron::is_same_v<T, long long> )
    return __builtin_ctzll(x);
  return 0;
}

template <typename T>
constexpr int
countr_one(T x) noexcept
{
  return countl_zero((T)~x);
}
};
