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
    if ( *p == '\0' ) return p - str;
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

template<typename T>
[[gnu::always_inline]] static inline usize
find_first_elem(const T *p, usize len, T ch) noexcept
{
  usize i = 0;
#if defined(__micron_x86_avx2)
  if constexpr ( sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ) {
    constexpr usize EPV = 32 / sizeof(T);
    __m256i needle;
    if constexpr ( sizeof(T) == 1 )
      needle = _mm256_set1_epi8(static_cast<char>(ch));
    else if constexpr ( sizeof(T) == 2 )
      needle = _mm256_set1_epi16(static_cast<short>(ch));
    else
      needle = _mm256_set1_epi32(static_cast<int>(ch));
    for ( ; i + EPV <= len; i += EPV ) {
      __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p + i));
      __m256i cmp;
      if constexpr ( sizeof(T) == 1 )
        cmp = _mm256_cmpeq_epi8(v, needle);
      else if constexpr ( sizeof(T) == 2 )
        cmp = _mm256_cmpeq_epi16(v, needle);
      else
        cmp = _mm256_cmpeq_epi32(v, needle);
      unsigned m = static_cast<unsigned>(_mm256_movemask_epi8(cmp));
      if ( m ) return i + (static_cast<usize>(__builtin_ctz(m)) / sizeof(T));
    }
  }
#endif
  for ( ; i < len; ++i )
    if ( p[i] == ch ) return i;
  return len;
}

template<typename T>
[[gnu::always_inline]] static inline usize
find_last_elem(const T *p, usize len, T ch) noexcept
{
#if defined(__micron_x86_avx2)
  if constexpr ( sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ) {
    constexpr usize EPV = 32 / sizeof(T);
    __m256i needle;
    if constexpr ( sizeof(T) == 1 )
      needle = _mm256_set1_epi8(static_cast<char>(ch));
    else if constexpr ( sizeof(T) == 2 )
      needle = _mm256_set1_epi16(static_cast<short>(ch));
    else
      needle = _mm256_set1_epi32(static_cast<int>(ch));
    usize i = len;
    while ( i >= EPV ) {
      i -= EPV;
      __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p + i));
      __m256i cmp;
      if constexpr ( sizeof(T) == 1 )
        cmp = _mm256_cmpeq_epi8(v, needle);
      else if constexpr ( sizeof(T) == 2 )
        cmp = _mm256_cmpeq_epi16(v, needle);
      else
        cmp = _mm256_cmpeq_epi32(v, needle);
      unsigned m = static_cast<unsigned>(_mm256_movemask_epi8(cmp));
      if ( m ) return i + (static_cast<usize>(31 - __builtin_clz(m)) / sizeof(T));
    }
    for ( usize j = i; j-- > 0; )
      if ( p[j] == ch ) return j;
    return len;
  }
#endif
  for ( usize j = len; j-- > 0; )
    if ( p[j] == ch ) return j;
  return len;
}

template<typename T>
[[gnu::always_inline]] static inline usize
count_elem(const T *p, usize len, T ch) noexcept
{
  usize cnt = 0;
  usize i = 0;
#if defined(__micron_x86_avx2)
  if constexpr ( sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ) {
    constexpr usize EPV = 32 / sizeof(T);
    __m256i needle;
    if constexpr ( sizeof(T) == 1 )
      needle = _mm256_set1_epi8(static_cast<char>(ch));
    else if constexpr ( sizeof(T) == 2 )
      needle = _mm256_set1_epi16(static_cast<short>(ch));
    else
      needle = _mm256_set1_epi32(static_cast<int>(ch));
    for ( ; i + EPV <= len; i += EPV ) {
      __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p + i));
      __m256i cmp;
      if constexpr ( sizeof(T) == 1 )
        cmp = _mm256_cmpeq_epi8(v, needle);
      else if constexpr ( sizeof(T) == 2 )
        cmp = _mm256_cmpeq_epi16(v, needle);
      else
        cmp = _mm256_cmpeq_epi32(v, needle);
      unsigned m = static_cast<unsigned>(_mm256_movemask_epi8(cmp));
      cnt += static_cast<usize>(__builtin_popcount(m)) / sizeof(T);
    }
  }
#endif
  for ( ; i < len; ++i )
    if ( p[i] == ch ) ++cnt;
  return cnt;
}

template<typename T>
[[gnu::always_inline]] static inline usize
find_first_of_elem(const T *p, usize len, const T *chars, usize k, usize pos) noexcept
{
  usize i = pos;
  if ( k == 0 ) return len;
#if defined(__micron_x86_avx2)
  if constexpr ( sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ) {
    constexpr usize EPV = 32 / sizeof(T);
    for ( ; i + EPV <= len; i += EPV ) {
      __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p + i));
      __m256i any = _mm256_setzero_si256();
      for ( usize c = 0; c < k; ++c ) {
        __m256i needle, cmp;
        if constexpr ( sizeof(T) == 1 ) {
          needle = _mm256_set1_epi8(static_cast<char>(chars[c]));
          cmp = _mm256_cmpeq_epi8(v, needle);
        } else if constexpr ( sizeof(T) == 2 ) {
          needle = _mm256_set1_epi16(static_cast<short>(chars[c]));
          cmp = _mm256_cmpeq_epi16(v, needle);
        } else {
          needle = _mm256_set1_epi32(static_cast<int>(chars[c]));
          cmp = _mm256_cmpeq_epi32(v, needle);
        }
        any = _mm256_or_si256(any, cmp);
      }
      unsigned m = static_cast<unsigned>(_mm256_movemask_epi8(any));
      if ( m ) return i + (static_cast<usize>(__builtin_ctz(m)) / sizeof(T));
    }
  }
#endif
  for ( ; i < len; ++i )
    for ( usize c = 0; c < k; ++c )
      if ( p[i] == chars[c] ) return i;
  return len;
}

#if defined(__micron_x86_avx2)
template<typename T>
[[gnu::always_inline]] static inline __m256i
__any_of_chunk(const T *q, const T *chars, usize k) noexcept
{
  __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(q));
  __m256i any = _mm256_setzero_si256();
  for ( usize c = 0; c < k; ++c ) {
    __m256i needle, cmp;
    if constexpr ( sizeof(T) == 1 ) {
      needle = _mm256_set1_epi8(static_cast<char>(chars[c]));
      cmp = _mm256_cmpeq_epi8(v, needle);
    } else if constexpr ( sizeof(T) == 2 ) {
      needle = _mm256_set1_epi16(static_cast<short>(chars[c]));
      cmp = _mm256_cmpeq_epi16(v, needle);
    } else {
      needle = _mm256_set1_epi32(static_cast<int>(chars[c]));
      cmp = _mm256_cmpeq_epi32(v, needle);
    }
    any = _mm256_or_si256(any, cmp);
  }
  return any;
}
#endif

template<typename T>
[[gnu::always_inline]] static inline usize
find_first_not_of_elem(const T *p, usize len, const T *chars, usize k, usize pos) noexcept
{
  usize i = pos;
  if ( k == 0 ) return (pos < len) ? pos : len;
#if defined(__micron_x86_avx2)
  if constexpr ( sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ) {
    constexpr usize EPV = 32 / sizeof(T);
    for ( ; i + EPV <= len; i += EPV ) {
      __m256i any = __any_of_chunk<T>(p + i, chars, k);
      __m256i notany = _mm256_xor_si256(any, _mm256_set1_epi8(static_cast<char>(0xFF)));
      unsigned m = static_cast<unsigned>(_mm256_movemask_epi8(notany));
      if ( m ) return i + (static_cast<usize>(__builtin_ctz(m)) / sizeof(T));
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
#if defined(__micron_x86_avx2)
  if constexpr ( sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ) {
    constexpr usize EPV = 32 / sizeof(T);
    usize i = len;
    while ( i >= EPV ) {
      i -= EPV;
      __m256i any = __any_of_chunk<T>(p + i, chars, k);
      unsigned m = static_cast<unsigned>(_mm256_movemask_epi8(any));
      if ( m ) return i + (static_cast<usize>(31 - __builtin_clz(m)) / sizeof(T));
    }
    for ( usize j = i; j-- > 0; )
      for ( usize c = 0; c < k; ++c )
        if ( p[j] == chars[c] ) return j;
    return len;
  }
#endif
  for ( usize j = len; j-- > 0; )
    for ( usize c = 0; c < k; ++c )
      if ( p[j] == chars[c] ) return j;
  return len;
}

template<typename T>
[[gnu::always_inline]] static inline usize
find_last_not_of_elem(const T *p, usize len, const T *chars, usize k) noexcept
{
  if ( k == 0 ) return (len ? len - 1 : len);
#if defined(__micron_x86_avx2)
  if constexpr ( sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ) {
    constexpr usize EPV = 32 / sizeof(T);
    usize i = len;
    while ( i >= EPV ) {
      i -= EPV;
      __m256i any = __any_of_chunk<T>(p + i, chars, k);
      __m256i notany = _mm256_xor_si256(any, _mm256_set1_epi8(static_cast<char>(0xFF)));
      unsigned m = static_cast<unsigned>(_mm256_movemask_epi8(notany));
      if ( m ) return i + (static_cast<usize>(31 - __builtin_clz(m)) / sizeof(T));
    }
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
#endif
  for ( usize j = len; j-- > 0; ) {
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
