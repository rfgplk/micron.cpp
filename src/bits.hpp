//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "type_traits.hpp"
#include "types.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// bytewise operations go here

namespace micron
{

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bits

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
  word b = repeat_bytes<0x01>();
  word c = repeat_bytes<0x80>();
  return (((a - b) & ~a & c) != 0);
}

template <typename T = word>
constexpr inline word __attribute__((always_inline))
byte_eq(T a, T b)
{
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

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rotrs

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
  if constexpr ( __has_builtin(__builtin_rotateleft64) )
    return __builtin_rotateleft64(x, r);
  else
    return (x << r) | (x >> (64 - r));
}

template <typename T>
inline __attribute__((always_inline)) T
rotr32(const T x, const i8 r)
{
  if constexpr ( __has_builtin(__builtin_rotateright32) )
    return __builtin_rotateright32(x, r);
  else
    return (x >> r) | (x << (32 - r));
}

template <typename T>
inline __attribute__((always_inline)) T
rotr64(const T x, const i8 r)
{
  if constexpr ( __has_builtin(__builtin_rotateright64) )
    return __builtin_rotateright64(x, r);
  else
    return (x >> r) | (x << (64 - r));
}

template <typename T>
inline __attribute__((always_inline)) T
rotl16(const T x, const i8 r)
{
  return static_cast<T>(((x << r) | (x >> (16 - r))) & 0xFFFF);
}

template <typename T>
inline __attribute__((always_inline)) T
rotr16(const T x, const i8 r)
{
  return static_cast<T>(((x >> r) | (x << (16 - r))) & 0xFFFF);
}

template <typename T>
inline __attribute__((always_inline)) T
rotl8(const T x, const i8 r)
{
  return static_cast<T>(((x << r) | (x >> (8 - r))) & 0xFF);
}

template <typename T>
inline __attribute__((always_inline)) T
rotr8(const T x, const i8 r)
{
  return static_cast<T>(((x >> r) | (x << (8 - r))) & 0xFF);
}

template <typename T>
inline __attribute__((always_inline)) T
rotl(const T x, const i8 r)
{
  constexpr int w = sizeof(T) * 8;
  const i8 s = r & (w - 1);
  if ( s == 0 ) return x;
  return static_cast<T>((x << s) | (x >> (w - s)));
}

template <typename T>
inline __attribute__((always_inline)) T
rotr(const T x, const i8 r)
{
  constexpr int w = sizeof(T) * 8;
  const i8 s = r & (w - 1);
  if ( s == 0 ) return x;
  return static_cast<T>((x >> s) | (x << (w - s)));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reversals

template <typename T>
constexpr T
reverse_bits(T t)
{
  T c{};
  unsigned char *p = reinterpret_cast<unsigned char *>(&t);
  unsigned char *q = reinterpret_cast<unsigned char *>(&c);
  for ( size_t i = 0; i < sizeof(T); ++i ) q[sizeof(T) - 1 - i] = BitReverseTable256[p[i]];
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
constexpr T
reverse_bits_per_byte(T x) noexcept
{
  T result = T(0);
  for ( size_t b = 0; b < sizeof(T); ++b ) {
    u8 byte = static_cast<u8>(x >> (b * 8));
    byte = static_cast<u8>((byte * 0x0202020202ULL & 0x010884422010ULL) % 1023);
    result |= T(byte) << (b * 8);
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bitwise

template <typename T>
  requires(micron::is_arithmetic_v<T>)
inline constexpr T
flip(T t)
{
  t ^= 0xFF;
  return t;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
inline constexpr T
invert(T t)
{
  return ~t;
}

template <typename T = word>
  requires(micron::is_arithmetic_v<T>)
constexpr T
mask_low(int n) noexcept
{
  if ( n == 0 ) return T(0);
  if ( n >= static_cast<int>(sizeof(T) * 8) ) return ~T(0);
  return (T(1) << n) - T(1);
}

template <typename T = word>
  requires(micron::is_arithmetic_v<T>)
constexpr T
mask_high(int n) noexcept
{
  return ~mask_low<T>(static_cast<int>(sizeof(T) * 8) - n);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
bits_range(T x, int lo, int len) noexcept
{
  return (x >> lo) & mask_low<T>(len);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
deposit_range(T dst, T val, int lo, int len) noexcept
{
  T m = mask_low<T>(len) << lo;
  return (dst & ~m) | ((val << lo) & m);
}

template <int x, typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
set_bit(T t)
{
  return t | (T(1) << x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
set_bit(T t, int x)
{
  return t | (T(1) << x);
}

template <int x, typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr bool
bit(T t)
{
  return (t & (T(1) << x)) != 0;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr bool
bit(T t, int x)
{
  return (t & (T(1) << x)) != 0;
}

template <int x, typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
clear_bit(T t)
{
  return t & ~(T(1) << x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
clear_bit(T t, int x)
{
  return t & ~(T(1) << x);
}

template <int x, typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
toggle_bit(T t)
{
  return t ^ (T(1) << x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
toggle_bit(T t, int x)
{
  return t ^ (T(1) << x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
copy_bit(T dst, T src, int dst_pos, int src_pos) noexcept
{
  T b = (src >> src_pos) & T(1);
  return (dst & ~(T(1) << dst_pos)) | (b << dst_pos);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
assign_bit(T t, int x, bool cond) noexcept
{
  return cond ? set_bit(t, x) : clear_bit(t, x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
swap_bits(T x, int i, int j) noexcept
{
  T d = ((x >> i) ^ (x >> j)) & T(1);
  return x ^ ((d << i) | (d << j));
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr int
bitcount(T x) noexcept
{
  if constexpr ( micron::is_same_v<T, int> || micron::is_same_v<T, char> || micron::is_same_v<T, unsigned char>
                 || micron::is_same_v<T, unsigned int> || micron::is_same_v<T, short> || micron::is_same_v<T, unsigned short> )
    return __builtin_popcount(static_cast<unsigned int>(x));
  else if constexpr ( micron::is_same_v<T, long> || micron::is_same_v<T, unsigned long> )
    return __builtin_popcountl(static_cast<unsigned long>(x));
  else if constexpr ( micron::is_same_v<T, long long> || micron::is_same_v<T, unsigned long long> )
    return __builtin_popcountll(static_cast<unsigned long long>(x));
  else
    return __builtin_popcountll(static_cast<unsigned long long>(x));
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr int
popcount(T x) noexcept
{
  return bitcount(x);
}     // STL compat

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr bool
has_any_bit(T x)
{
  return bitcount(x) != 0;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr bool
has_single_bit(T x)
{
  return bitcount(x) == 1;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr bool
has_n_bits(T x, int n)
{
  return bitcount(x) == n;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr bool
has_even_bits(T x)
{
  return (bitcount(x) & 1) == 0;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr int
parity(T x) noexcept
{
  if constexpr ( micron::is_same_v<T, unsigned long long> || micron::is_same_v<T, long long> )
    return __builtin_parityll(static_cast<unsigned long long>(x));
  else if constexpr ( micron::is_same_v<T, unsigned long> || micron::is_same_v<T, long> )
    return __builtin_parityl(static_cast<unsigned long>(x));
  else
    return __builtin_parity(static_cast<unsigned int>(x));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// countls

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr int
countl_zero(T x) noexcept
{
  if ( x == 0 ) return static_cast<int>(sizeof(T) * 8);
  if constexpr ( micron::is_same_v<T, int> || micron::is_same_v<T, unsigned int> )
    return __builtin_clz(static_cast<unsigned int>(x));
  else if constexpr ( micron::is_same_v<T, long> || micron::is_same_v<T, unsigned long> )
    return __builtin_clzl(static_cast<unsigned long>(x));
  else if constexpr ( micron::is_same_v<T, long long> || micron::is_same_v<T, unsigned long long> )
    return __builtin_clzll(static_cast<unsigned long long>(x));
  else     // narrow types: promote then adjust for padding
    return __builtin_clzll(static_cast<unsigned long long>(x)) - static_cast<int>((sizeof(unsigned long long) - sizeof(T)) * 8);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr int
countr_zero(T x) noexcept
{
  if ( x == 0 ) return static_cast<int>(sizeof(T) * 8);
  if constexpr ( micron::is_same_v<T, int> || micron::is_same_v<T, unsigned int> )
    return __builtin_ctz(static_cast<unsigned int>(x));
  else if constexpr ( micron::is_same_v<T, long> || micron::is_same_v<T, unsigned long> )
    return __builtin_ctzl(static_cast<unsigned long>(x));
  else if constexpr ( micron::is_same_v<T, long long> || micron::is_same_v<T, unsigned long long> )
    return __builtin_ctzll(static_cast<unsigned long long>(x));
  else
    return __builtin_ctzll(static_cast<unsigned long long>(x));
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr int
countr_one(T x) noexcept
{
  return countr_zero(static_cast<T>(~x));
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr int
countl_one(T x) noexcept
{
  return countl_zero(static_cast<T>(~x));
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr int
bit_width(T x) noexcept
{
  if ( x == 0 ) return 0;
  return static_cast<int>(sizeof(T) * 8) - countl_zero(x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr int
log2_floor(T x) noexcept
{
  return bit_width(x) - 1;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr int
log2_ceil(T x) noexcept
{
  if ( x <= 1 ) return 0;
  return bit_width(static_cast<T>(x - 1));
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr bool
is_power_of_two(T x) noexcept
{
  return x > 0 && has_single_bit(x);
}

// Largest power of 2 <= x
template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
bit_floor(T x) noexcept
{
  if ( x == 0 ) return 0;
  return T(1) << log2_floor(x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
bit_ceil(T x) noexcept
{
  if ( x <= 1 ) return 1;
  return T(1) << log2_ceil(x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
align_up(T x, T a) noexcept
{
  return (x + a - 1) & ~(a - 1);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
align_down(T x, T a) noexcept
{
  return x & ~(a - 1);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
lowest_set_bit(T x) noexcept
{
  return x & static_cast<T>(-static_cast<typename micron::make_signed<T>::type>(x));
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
clear_lowest_set_bit(T x) noexcept
{
  return x & (x - 1);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
smear_lowest_set_bit(T x) noexcept
{
  return x | (x - 1);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
highest_set_bit(T x) noexcept
{
  if ( x == 0 ) return 0;
  return T(1) << log2_floor(x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
fill_from_highest(T x) noexcept
{
  if ( x == 0 ) return 0;
  return mask_low<T>(log2_floor(x) + 1);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
nth_set_bit(T x, int n) noexcept
{
  for ( int i = 0; i < n; ++i ) x = clear_lowest_set_bit(x);
  return lowest_set_bit(x);
}

inline __attribute__((always_inline)) u16
bswap16(u16 x) noexcept
{
  return __builtin_bswap16(x);
}

inline __attribute__((always_inline)) u32
bswap32(u32 x) noexcept
{
  return __builtin_bswap32(x);
}

inline __attribute__((always_inline)) u64
bswap64(u64 x) noexcept
{
  return __builtin_bswap64(x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
inline __attribute__((always_inline)) T
bswap(T x) noexcept
{
  if constexpr ( sizeof(T) == 1 )
    return x;
  else if constexpr ( sizeof(T) == 2 )
    return static_cast<T>(__builtin_bswap16(static_cast<u16>(x)));
  else if constexpr ( sizeof(T) == 4 )
    return static_cast<T>(__builtin_bswap32(static_cast<u32>(x)));
  else if constexpr ( sizeof(T) == 8 )
    return static_cast<T>(__builtin_bswap64(static_cast<u64>(x)));
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
inline T
to_big_endian(T x) noexcept
{
  return bswap(x);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
inline T
to_little_endian(T x) noexcept
{
  return x;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
inline T
from_big_endian(T x) noexcept
{
  return bswap(x);
}

template <typename T>
  requires(sizeof(T) == 1)
constexpr T
swap_nibbles(T x) noexcept
{
  return static_cast<T>(((x & 0x0F) << 4) | ((x & 0xF0) >> 4));
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
low_nibble(T x) noexcept
{
  return x & T(0x0F);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
high_nibble(T x) noexcept
{
  return (x >> 4) & T(0x0F);
}

// Swap bytes i and j within a multi-byte word
template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
swap_bytes(T x, int i, int j) noexcept
{
  u8 bi = static_cast<u8>(x >> (i * 8));
  u8 bj = static_cast<u8>(x >> (j * 8));
  T result = x;
  result &= ~((T(0xFF) << (i * 8)) | (T(0xFF) << (j * 8)));
  result |= (T(bj) << (i * 8)) | (T(bi) << (j * 8));
  return result;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
merge_bits(T a, T b, T mask) noexcept
{
  return a ^ ((a ^ b) & mask);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
select(bool cond, T a, T b) noexcept
{
  return b ^ (static_cast<T>(-static_cast<T>(cond)) & (a ^ b));
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
sign_extend(T x, int b) noexcept
{
  T mask = T(1) << b;
  return (x ^ mask) - mask;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
average(T a, T b) noexcept
{
  return (a & b) + ((a ^ b) >> 1);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
average_ceil(T a, T b) noexcept
{
  return (a | b) - ((a ^ b) >> 1);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
abs_diff(T a, T b) noexcept
{
  return (a > b) ? (a - b) : (b - a);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
saturate_bits(T x, int n) noexcept
{
  return x & mask_low<T>(n);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
saturating_add(T a, T b) noexcept
{
  T result = a + b;
  return (result < a) ? ~T(0) : result;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
saturating_sub(T a, T b) noexcept
{
  return (a > b) ? (a - b) : T(0);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr bool
add_overflows(T a, T b) noexcept
{
  return b > (~T(0) - a);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr bool
sub_underflows(T a, T b) noexcept
{
  return b > a;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr bool
mul_overflows(T a, T b) noexcept
{
  if ( a == 0 ) return false;
  return b > (~T(0) / a);
}

constexpr u32
spread_bits_16(u16 x) noexcept
{
  u32 v = x;
  v = (v | (v << 8)) & 0x00FF00FFu;
  v = (v | (v << 4)) & 0x0F0F0F0Fu;
  v = (v | (v << 2)) & 0x33333333u;
  v = (v | (v << 1)) & 0x55555555u;
  return v;
}

constexpr u16
compact_bits_16(u32 x) noexcept
{
  x &= 0x55555555u;
  x = (x | (x >> 1)) & 0x33333333u;
  x = (x | (x >> 2)) & 0x0F0F0F0Fu;
  x = (x | (x >> 4)) & 0x00FF00FFu;
  x = (x | (x >> 8)) & 0x0000FFFFu;
  return static_cast<u16>(x);
}

constexpr u32
interleave_bits(u16 x, u16 y) noexcept
{
  return spread_bits_16(x) | (spread_bits_16(y) << 1);
}

constexpr void
deinterleave_bits(u32 z, u16 &x, u16 &y) noexcept
{
  x = compact_bits_16(z);
  y = compact_bits_16(z >> 1);
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
pext(T x, T mask) noexcept
{
  T result = 0, out = 1;
  while ( mask ) {
    if ( x & lowest_set_bit(mask) ) result |= out;
    mask &= mask - 1;
    out <<= 1;
  }
  return result;
}

template <typename T>
  requires(micron::is_arithmetic_v<T>)
constexpr T
pdep(T x, T mask) noexcept
{
  T result = 0, src = 1;
  while ( mask ) {
    if ( x & src ) result |= lowest_set_bit(mask);
    mask &= mask - 1;
    src <<= 1;
  }
  return result;
}

};     // namespace micron
