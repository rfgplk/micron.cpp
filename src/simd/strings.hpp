#pragma once

#include "simd.hpp"
#include "types.hpp"

#include "bitwise.hpp"

namespace micron
{
namespace simd
{

inline usize
sse_strlen(const char *str)
{
#if defined(__micron_x86_sse2)
  const char *p = str;
  i128 zero = _mm_setzero_si128();

  while ( reinterpret_cast<uintptr_t>(p) & 0xF ) {
    if ( *p == '\0' ) return static_cast<usize>(p - str);
    ++p;
  }

  for ( ;; ) {
    i128 chunk = _mm_load_si128(reinterpret_cast<const i128 *>(p));
    i128 cmp = _mm_cmpeq_epi8(chunk, zero);
    int mask = _mm_movemask_epi8(cmp);
    if ( mask != 0 ) {
      return static_cast<usize>((p - str) + __builtin_ctz(static_cast<unsigned int>(mask)));
    }
    p += 16;
  }
#else
  const char *p = str;
  while ( *p ) ++p;
  return static_cast<usize>(p - str);
#endif
}

template<typename T>
[[gnu::always_inline]] static inline usize
__simd_find_byte(const T *p, usize len, T ch) noexcept
{
  if ( len == 0 ) return len;
#if defined(__micron_x86_avx2)
  return micron::simd::find_first_set_256(p, len, static_cast<char>(ch));
#else
  return micron::simd::find_first_set_128(p, len, static_cast<char>(ch));
#endif
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// element scans

template<typename T>
[[gnu::always_inline]] static inline auto
__elem_bits(T ch) noexcept
{
  if constexpr ( sizeof(T) == 1 )
    return __builtin_bit_cast(u8, ch);
  else if constexpr ( sizeof(T) == 2 )
    return __builtin_bit_cast(u16, ch);
  else if constexpr ( sizeof(T) == 4 )
    return __builtin_bit_cast(u32, ch);
  else
    return __builtin_bit_cast(u64, ch);
}

template<typename T>
concept __scan_width = (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// lane primitives

#if defined(__micron_x86_avx2)
#define __micron_scan_lanes 1

template<typename T> inline constexpr usize __lane_elems = 32 / sizeof(T);

using __lane_bits_t = unsigned;

template<typename T>
[[gnu::always_inline]] static inline __m256i
__lane_load(const T *q) noexcept
{
  return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(q));
}

template<typename T>
[[gnu::always_inline]] static inline __m256i
__lane_bcast(T ch) noexcept
{
  const auto nb = __elem_bits(ch);
  if constexpr ( sizeof(T) == 1 )
    return _mm256_set1_epi8(static_cast<char>(nb));
  else if constexpr ( sizeof(T) == 2 )
    return _mm256_set1_epi16(static_cast<short>(nb));
  else if constexpr ( sizeof(T) == 4 )
    return _mm256_set1_epi32(static_cast<int>(nb));
  else
    return _mm256_set1_epi64x(static_cast<long long>(nb));
}

template<typename T>
[[gnu::always_inline]] static inline __m256i
__lane_cmpeq(__m256i v, __m256i needle) noexcept
{
  if constexpr ( sizeof(T) == 1 )
    return _mm256_cmpeq_epi8(v, needle);
  else if constexpr ( sizeof(T) == 2 )
    return _mm256_cmpeq_epi16(v, needle);
  else if constexpr ( sizeof(T) == 4 )
    return _mm256_cmpeq_epi32(v, needle);
  else
    return _mm256_cmpeq_epi64(v, needle);
}

[[gnu::always_inline]] static inline __m256i
__lane_zero() noexcept
{
  return _mm256_setzero_si256();
}

[[gnu::always_inline]] static inline __m256i
__lane_or(__m256i a, __m256i b) noexcept
{
  return _mm256_or_si256(a, b);
}

[[gnu::always_inline]] static inline __lane_bits_t
__lane_bits(__m256i cmp) noexcept
{
  return static_cast<unsigned>(_mm256_movemask_epi8(cmp));
}

// all 32 movemask bits are lanes here, so a bare invert is exact
[[gnu::always_inline]] static inline __lane_bits_t
__lane_bits_ne(__m256i cmp) noexcept
{
  return ~__lane_bits(cmp);
}

#elif defined(__micron_x86_sse2)
#define __micron_scan_lanes 1

template<typename T> inline constexpr usize __lane_elems = 16 / sizeof(T);

using __lane_bits_t = unsigned;

template<typename T>
[[gnu::always_inline]] static inline __m128i
__lane_load(const T *q) noexcept
{
  return _mm_loadu_si128(reinterpret_cast<const __m128i *>(q));
}

template<typename T>
[[gnu::always_inline]] static inline __m128i
__lane_bcast(T ch) noexcept
{
  const auto nb = __elem_bits(ch);
  if constexpr ( sizeof(T) == 1 )
    return _mm_set1_epi8(static_cast<char>(nb));
  else if constexpr ( sizeof(T) == 2 )
    return _mm_set1_epi16(static_cast<short>(nb));
  else if constexpr ( sizeof(T) == 4 )
    return _mm_set1_epi32(static_cast<int>(nb));
  else
    return _mm_set1_epi64x(static_cast<long long>(nb));
}

template<typename T>
[[gnu::always_inline]] static inline __m128i
__lane_cmpeq(__m128i v, __m128i needle) noexcept
{
  if constexpr ( sizeof(T) == 1 )
    return _mm_cmpeq_epi8(v, needle);
  else if constexpr ( sizeof(T) == 2 )
    return _mm_cmpeq_epi16(v, needle);
  else if constexpr ( sizeof(T) == 4 )
    return _mm_cmpeq_epi32(v, needle);
  else {
#if defined(__micron_x86_sse4_1)
    return _mm_cmpeq_epi64(v, needle);
#else
    // SSE2 has no 64-bit compare
    const __m128i h = _mm_cmpeq_epi32(v, needle);
    return _mm_and_si128(h, _mm_shuffle_epi32(h, 0xB1));
#endif
  }
}

[[gnu::always_inline]] static inline __m128i
__lane_zero() noexcept
{
  return _mm_setzero_si128();
}

[[gnu::always_inline]] static inline __m128i
__lane_or(__m128i a, __m128i b) noexcept
{
  return _mm_or_si128(a, b);
}

[[gnu::always_inline]] static inline __lane_bits_t
__lane_bits(__m128i cmp) noexcept
{
  return static_cast<unsigned>(_mm_movemask_epi8(cmp));
}

// only the low 16 movemask bits are lanes; mask before inverting or ctz finds bit 16
[[gnu::always_inline]] static inline __lane_bits_t
__lane_bits_ne(__m128i cmp) noexcept
{
  return (~__lane_bits(cmp)) & 0xFFFFu;
}

#elif defined(__micron_arch_arm_any) && defined(__micron_arm_neon)
#define __micron_scan_lanes 1

template<typename T> inline constexpr usize __lane_elems = 16 / sizeof(T);

using __lane_bits_t = u64;

// the 8-byte arms are AArch64-only
template<typename T>
[[gnu::always_inline]] static inline auto
__lane_load(const T *q) noexcept
{
  if constexpr ( sizeof(T) == 1 )
    return vld1q_u8(reinterpret_cast<const u8 *>(q));
  else if constexpr ( sizeof(T) == 2 )
    return vld1q_u16(reinterpret_cast<const u16 *>(q));
  else if constexpr ( sizeof(T) == 4 )
    return vld1q_u32(reinterpret_cast<const u32 *>(q));
#if defined(__micron_arch_arm64)
  else
    return vld1q_u64(reinterpret_cast<const u64 *>(q));
#endif
}

template<typename T>
[[gnu::always_inline]] static inline auto
__lane_bcast(T ch) noexcept
{
  const auto nb = __elem_bits(ch);
  if constexpr ( sizeof(T) == 1 )
    return vdupq_n_u8(nb);
  else if constexpr ( sizeof(T) == 2 )
    return vdupq_n_u16(nb);
  else if constexpr ( sizeof(T) == 4 )
    return vdupq_n_u32(nb);
#if defined(__micron_arch_arm64)
  else
    return vdupq_n_u64(nb);
#endif
}

template<typename T, typename V>
[[gnu::always_inline]] static inline uint8x16_t
__lane_cmpeq(V v, V needle) noexcept
{
  if constexpr ( sizeof(T) == 1 )
    return vceqq_u8(v, needle);
  else if constexpr ( sizeof(T) == 2 )
    return vreinterpretq_u8_u16(vceqq_u16(v, needle));
  else if constexpr ( sizeof(T) == 4 )
    return vreinterpretq_u8_u32(vceqq_u32(v, needle));
#if defined(__micron_arch_arm64)
  else
    return vreinterpretq_u8_u64(vceqq_u64(v, needle));
#endif
}

[[gnu::always_inline]] static inline uint8x16_t
__lane_zero() noexcept
{
  return vdupq_n_u8(0);
}

[[gnu::always_inline]] static inline uint8x16_t
__lane_or(uint8x16_t a, uint8x16_t b) noexcept
{
  return vorrq_u8(a, b);
}

[[gnu::always_inline]] static inline __lane_bits_t
__lane_bits(uint8x16_t cmp) noexcept
{
  return vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmp), 4)), 0);
}

// all 64 bits of the narrowed word are lanes (4 per byte)
[[gnu::always_inline]] static inline __lane_bits_t
__lane_bits_ne(uint8x16_t cmp) noexcept
{
  return ~__lane_bits(cmp);
}

#endif

#if defined(__micron_scan_lanes)

#if defined(__micron_arch_arm_any) && !defined(__micron_arch_arm64)
template<typename T>
concept __lane_ok = (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4);
#else
template<typename T>
concept __lane_ok = __scan_width<T>;
#endif

// mask bit index -> element index
template<typename T>
[[gnu::always_inline]] static inline usize
__lane_lowest(__lane_bits_t m) noexcept
{
#if defined(__micron_arch_arm_any)
  return (static_cast<usize>(__builtin_ctzll(m)) >> 2) / sizeof(T);
#else
  return static_cast<usize>(__builtin_ctz(m)) / sizeof(T);
#endif
}

template<typename T>
[[gnu::always_inline]] static inline usize
__lane_highest(__lane_bits_t m) noexcept
{
#if defined(__micron_arch_arm_any)
  return ((63u - static_cast<usize>(__builtin_clzll(m))) >> 2) / sizeof(T);
#else
  return static_cast<usize>(31 - __builtin_clz(m)) / sizeof(T);
#endif
}

template<typename T>
[[gnu::always_inline]] static inline usize
__lane_popcount(__lane_bits_t m) noexcept
{
#if defined(__micron_arch_arm_any)
  return (static_cast<usize>(__builtin_popcountll(m)) >> 2) / sizeof(T);
#else
  return static_cast<usize>(__builtin_popcount(m)) / sizeof(T);
#endif
}

#endif

template<typename T>
[[gnu::always_inline]] static inline usize
find_first_elem(const T *p, usize len, T ch) noexcept
{
  usize i = 0;
#if defined(__micron_scan_lanes)
  if constexpr ( __lane_ok<T> ) {
    constexpr usize EPV = __lane_elems<T>;
    const auto needle = __lane_bcast(ch);
    for ( ; i + EPV <= len; i += EPV ) {
      const __lane_bits_t m = __lane_bits(__lane_cmpeq<T>(__lane_load(p + i), needle));
      if ( m ) return i + __lane_lowest<T>(m);
    }
  }
#endif
  for ( ; i < len; ++i )
    if ( p[i] == ch ) return i;
  return len;
}

// first element not equal to ch
template<typename T>
[[gnu::always_inline]] static inline usize
find_first_ne_elem(const T *p, usize len, T ch) noexcept
{
  usize i = 0;
#if defined(__micron_scan_lanes)
  if constexpr ( __lane_ok<T> ) {
    constexpr usize EPV = __lane_elems<T>;
    const auto needle = __lane_bcast(ch);
    for ( ; i + EPV <= len; i += EPV ) {
      const __lane_bits_t m = __lane_bits_ne(__lane_cmpeq<T>(__lane_load(p + i), needle));
      if ( m ) return i + __lane_lowest<T>(m);
    }
  }
#endif
  for ( ; i < len; ++i )
    if ( !(p[i] == ch) ) return i;
  return len;
}

template<typename T>
[[gnu::always_inline]] static inline usize
find_last_elem(const T *p, usize len, T ch) noexcept
{
  usize i = len;
#if defined(__micron_scan_lanes)
  if constexpr ( __lane_ok<T> ) {
    constexpr usize EPV = __lane_elems<T>;
    const auto needle = __lane_bcast(ch);
    while ( i >= EPV ) {
      i -= EPV;
      const __lane_bits_t m = __lane_bits(__lane_cmpeq<T>(__lane_load(p + i), needle));
      if ( m ) return i + __lane_highest<T>(m);
    }
  }
#endif
  for ( usize j = i; j-- > 0; )
    if ( p[j] == ch ) return j;
  return len;
}

template<typename T>
[[gnu::always_inline]] static inline usize
count_elem(const T *p, usize len, T ch) noexcept
{
  usize cnt = 0;
  usize i = 0;
#if defined(__micron_scan_lanes)
  if constexpr ( __lane_ok<T> ) {
    constexpr usize EPV = __lane_elems<T>;
    const auto needle = __lane_bcast(ch);
    for ( ; i + EPV <= len; i += EPV ) cnt += __lane_popcount<T>(__lane_bits(__lane_cmpeq<T>(__lane_load(p + i), needle)));
  }
#endif
  for ( ; i < len; ++i )
    if ( p[i] == ch ) ++cnt;
  return cnt;
}

// %%%%%%%%%%%%%%%%%%%%%%%
// set membership

#if defined(__micron_scan_lanes)
template<typename T>
[[gnu::always_inline]] static inline auto
__any_of_chunk(const T *q, const T *chars, usize k) noexcept
{
  const auto v = __lane_load(q);
  auto any = __lane_zero();
  for ( usize c = 0; c < k; ++c ) any = __lane_or(any, __lane_cmpeq<T>(v, __lane_bcast<T>(chars[c])));
  return any;
}
#endif

template<typename T>
[[gnu::always_inline]] static inline usize
find_first_of_elem(const T *p, usize len, const T *chars, usize k, usize pos) noexcept
{
  usize i = pos;
  if ( k == 0 ) return len;
#if defined(__micron_scan_lanes)
  if constexpr ( __lane_ok<T> ) {
    constexpr usize EPV = __lane_elems<T>;
    for ( ; i + EPV <= len; i += EPV ) {
      const __lane_bits_t m = __lane_bits(__any_of_chunk(p + i, chars, k));
      if ( m ) return i + __lane_lowest<T>(m);
    }
  }
#endif
  for ( ; i < len; ++i )
    for ( usize c = 0; c < k; ++c )
      if ( p[i] == chars[c] ) return i;
  return len;
}

template<typename T>
[[gnu::always_inline]] static inline usize
find_first_not_of_elem(const T *p, usize len, const T *chars, usize k, usize pos) noexcept
{
  usize i = pos;
  if ( k == 0 ) return (pos < len) ? pos : len;
#if defined(__micron_scan_lanes)
  if constexpr ( __lane_ok<T> ) {
    constexpr usize EPV = __lane_elems<T>;
    for ( ; i + EPV <= len; i += EPV ) {
      const __lane_bits_t m = __lane_bits_ne(__any_of_chunk(p + i, chars, k));
      if ( m ) return i + __lane_lowest<T>(m);
    }
  }
#endif
  for ( ; i < len; ++i ) {
    bool in = false;
    for ( usize c = 0; c < k; ++c )
      if ( p[i] == chars[c] ) {
        in = true;
        break;
      }
    if ( !in ) return i;
  }
  return len;
}

template<typename T>
[[gnu::always_inline]] static inline usize
find_last_of_elem(const T *p, usize len, const T *chars, usize k) noexcept
{
  if ( k == 0 ) return len;
  usize i = len;
#if defined(__micron_scan_lanes)
  if constexpr ( __lane_ok<T> ) {
    constexpr usize EPV = __lane_elems<T>;
    while ( i >= EPV ) {
      i -= EPV;
      const __lane_bits_t m = __lane_bits(__any_of_chunk(p + i, chars, k));
      if ( m ) return i + __lane_highest<T>(m);
    }
  }
#endif
  for ( usize j = i; j-- > 0; )
    for ( usize c = 0; c < k; ++c )
      if ( p[j] == chars[c] ) return j;
  return len;
}

template<typename T>
[[gnu::always_inline]] static inline usize
find_last_not_of_elem(const T *p, usize len, const T *chars, usize k) noexcept
{
  if ( k == 0 ) return (len ? len - 1 : len);
  usize i = len;
#if defined(__micron_scan_lanes)
  if constexpr ( __lane_ok<T> ) {
    constexpr usize EPV = __lane_elems<T>;
    while ( i >= EPV ) {
      i -= EPV;
      const __lane_bits_t m = __lane_bits_ne(__any_of_chunk(p + i, chars, k));
      if ( m ) return i + __lane_highest<T>(m);
    }
  }
#endif
  for ( usize j = i; j-- > 0; ) {
    bool in = false;
    for ( usize c = 0; c < k; ++c )
      if ( p[j] == chars[c] ) {
        in = true;
        break;
      }
    if ( !in ) return j;
  }
  return len;
}

template<typename T>
[[gnu::always_inline]] static inline usize
find_substr_elem(const T *h, usize hlen, const T *n, usize nlen) noexcept
{
  if ( nlen == 0 ) return 0;
  if ( nlen > hlen ) return hlen;
  const T first = n[0];
  usize i = 0;
  while ( i + nlen <= hlen ) {
    usize f = find_first_elem(h + i, hlen - i, first);
    if ( f == hlen - i ) return hlen;
    i += f;
    if ( i + nlen > hlen ) return hlen;
    usize j = 1;
    for ( ; j < nlen; ++j )
      if ( h[i + j] != n[j] ) break;
    if ( j == nlen ) return i;
    ++i;
  }
  return hlen;
}

template<typename T>
[[gnu::always_inline]] static inline int
lexcmp_elem(const T *a, usize alen, const T *b, usize blen) noexcept
{
  const usize common = alen < blen ? alen : blen;
  for ( usize i = 0; i < common; ++i ) {
    if ( a[i] < b[i] ) return -1;
    if ( a[i] > b[i] ) return 1;
  }
  if ( alen < blen ) return -1;
  if ( alen > blen ) return 1;
  return 0;
}

template<typename T>
[[gnu::always_inline]] static inline void
to_lower_elem(T *p, usize len) noexcept
{
  usize i = 0;
#if defined(__micron_x86_avx2)
  if constexpr ( sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ) {
    constexpr usize EPV = 32 / sizeof(T);
    for ( ; i + EPV <= len; i += EPV ) {
      __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p + i));
      __m256i ge, le, delta, mask;
      if constexpr ( sizeof(T) == 1 ) {
        ge = _mm256_cmpgt_epi8(v, _mm256_set1_epi8('A' - 1));
        le = _mm256_cmpgt_epi8(_mm256_set1_epi8('Z' + 1), v);
        mask = _mm256_and_si256(ge, le);
        delta = _mm256_and_si256(mask, _mm256_set1_epi8(0x20));
      } else if constexpr ( sizeof(T) == 2 ) {
        ge = _mm256_cmpgt_epi16(v, _mm256_set1_epi16('A' - 1));
        le = _mm256_cmpgt_epi16(_mm256_set1_epi16('Z' + 1), v);
        mask = _mm256_and_si256(ge, le);
        delta = _mm256_and_si256(mask, _mm256_set1_epi16(0x20));
      } else {
        ge = _mm256_cmpgt_epi32(v, _mm256_set1_epi32('A' - 1));
        le = _mm256_cmpgt_epi32(_mm256_set1_epi32('Z' + 1), v);
        mask = _mm256_and_si256(ge, le);
        delta = _mm256_and_si256(mask, _mm256_set1_epi32(0x20));
      }
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(p + i), _mm256_add_epi8(v, delta));
    }
  }
#endif
  for ( ; i < len; ++i ) {
    auto c = static_cast<unsigned long>(p[i]);
    if ( c >= 'A' && c <= 'Z' ) p[i] = static_cast<T>(c + 0x20);
  }
}

template<typename T>
[[gnu::always_inline]] static inline void
to_upper_elem(T *p, usize len) noexcept
{
  usize i = 0;
#if defined(__micron_x86_avx2)
  if constexpr ( sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ) {
    constexpr usize EPV = 32 / sizeof(T);
    for ( ; i + EPV <= len; i += EPV ) {
      __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p + i));
      __m256i ge, le, delta, mask;
      if constexpr ( sizeof(T) == 1 ) {
        ge = _mm256_cmpgt_epi8(v, _mm256_set1_epi8('a' - 1));
        le = _mm256_cmpgt_epi8(_mm256_set1_epi8('z' + 1), v);
        mask = _mm256_and_si256(ge, le);
        delta = _mm256_and_si256(mask, _mm256_set1_epi8(0x20));
      } else if constexpr ( sizeof(T) == 2 ) {
        ge = _mm256_cmpgt_epi16(v, _mm256_set1_epi16('a' - 1));
        le = _mm256_cmpgt_epi16(_mm256_set1_epi16('z' + 1), v);
        mask = _mm256_and_si256(ge, le);
        delta = _mm256_and_si256(mask, _mm256_set1_epi16(0x20));
      } else {
        ge = _mm256_cmpgt_epi32(v, _mm256_set1_epi32('a' - 1));
        le = _mm256_cmpgt_epi32(_mm256_set1_epi32('z' + 1), v);
        mask = _mm256_and_si256(ge, le);
        delta = _mm256_and_si256(mask, _mm256_set1_epi32(0x20));
      }
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(p + i), _mm256_sub_epi8(v, delta));
    }
  }
#endif
  for ( ; i < len; ++i ) {
    auto c = static_cast<unsigned long>(p[i]);
    if ( c >= 'a' && c <= 'z' ) p[i] = static_cast<T>(c - 0x20);
  }
}

template<typename T>
[[gnu::always_inline]] static inline void
reverse_elem(T *p, usize len) noexcept
{
  for ( usize a = 0, b = (len ? len - 1 : 0); a < b; ++a, --b ) {
    T t = p[a];
    p[a] = p[b];
    p[b] = t;
  }
}

enum class __byte_op { __xor, __and, __or };

template<__byte_op Op>
[[gnu::always_inline]] static inline byte
__apply_byte_op(byte x, byte y) noexcept
{
  if constexpr ( Op == __byte_op::__xor )
    return static_cast<byte>(x ^ y);
  else if constexpr ( Op == __byte_op::__and )
    return static_cast<byte>(x & y);
  else
    return static_cast<byte>(x | y);
}

template<__byte_op Op>
[[gnu::always_inline]] static inline void
__bytes_cycle(byte *dst, const byte *a, usize an, const byte *b, usize bn) noexcept
{
  if ( bn == 0 ) {
    for ( usize i = 0; i < an; ++i ) dst[i] = a[i];
    return;
  }
  usize i = 0;
#if defined(__micron_x86_avx2)
  if ( bn >= an ) {
    for ( ; i + 32 <= an; i += 32 ) {
      __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a + i));
      __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(b + i));
      __m256i vr;
      if constexpr ( Op == __byte_op::__xor )
        vr = _mm256_xor_si256(va, vb);
      else if constexpr ( Op == __byte_op::__and )
        vr = _mm256_and_si256(va, vb);
      else
        vr = _mm256_or_si256(va, vb);
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst + i), vr);
    }
  }
#endif
  for ( ; i < an; ++i ) dst[i] = __apply_byte_op<Op>(a[i], b[i % bn]);
}

[[gnu::always_inline]] static inline void
xor_bytes_cycle(byte *dst, const byte *a, usize an, const byte *b, usize bn) noexcept
{
  __bytes_cycle<__byte_op::__xor>(dst, a, an, b, bn);
}

[[gnu::always_inline]] static inline void
and_bytes_cycle(byte *dst, const byte *a, usize an, const byte *b, usize bn) noexcept
{
  __bytes_cycle<__byte_op::__and>(dst, a, an, b, bn);
}

[[gnu::always_inline]] static inline void
or_bytes_cycle(byte *dst, const byte *a, usize an, const byte *b, usize bn) noexcept
{
  __bytes_cycle<__byte_op::__or>(dst, a, an, b, bn);
}

};      // namespace simd

};      // namespace micron
