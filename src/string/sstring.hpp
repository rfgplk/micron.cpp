//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../type_traits.hpp"

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../memory/memory.hpp"
#include "../pointer.hpp"
#include "../span.hpp"

#include "../memory_block.hpp"

#include "../simd/bitwise.hpp"
#include "../simd/intrin.hpp"

#include "unitypes.hpp"

namespace micron
{
// string on the stack, inplace (sstring means stackstring), interally SIMD dispatched
template<usize N, is_scalar_literal T = schar, bool Sf = true>
struct alignas(N * sizeof(T) >= 32 ? 32 : (N * sizeof(T) >= 16 ? 16 : alignof(T))) sstring {
  using category_type = string_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef T value_type;
  typedef T *iterator;
  typedef const T *const_iterator;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef usize size_type;
  typedef T &reference;
  typedef const T &const_reference;

  T memory[N];
  size_type length;

private:
  inline size_type
  __rfind_substr(const T *needle, size_type needle_len, size_type pos = npos) const noexcept
  {
    if ( needle_len == 0 ) return (pos == npos || pos > length) ? length : pos;
    if ( needle_len > length ) return npos;
    size_type limit = (pos == npos || pos > length - needle_len) ? length - needle_len : pos;
    for ( size_type i = limit + 1; i-- > 0; ) {
      size_type j = 0;
      while ( j < needle_len && memory[i + j] == needle[j] ) ++j;
      if ( j == needle_len ) return i;
    }
    return npos;
  }

  inline sstring &
  __replace_impl(size_type pos, size_type cnt, const T *with, size_type with_len)
  {
    if constexpr ( Sf ) {
      if ( length - cnt + with_len >= N ) exc<except::library_error>("micron::sstring replace() result exceeds capacity");
    }
    size_type tail = length - (pos + cnt);
    micron::bytemove(&memory[pos + with_len], &memory[pos + cnt], tail);
    if ( with_len < cnt ) micron::typeset<T>(&memory[pos + with_len + tail], 0x0, cnt - with_len);
    if ( with_len > 0 ) micron::memcpy(&memory[pos], with, with_len);
    length = length - cnt + with_len;
    return *this;
  }

  inline __attribute__((always_inline)) bool
  __null_check(const void *ptr) const noexcept
  {
    return (ptr == nullptr);
  }

  inline __attribute__((always_inline)) bool
  __index_check(size_type n) const noexcept
  {
    return (n >= length);
  }

  inline __attribute__((always_inline)) bool
  __size_check(size_type cnt) const noexcept
  {
    return (length + cnt >= N);
  }

  inline __attribute__((always_inline)) bool
  __range_pos_cnt(size_type pos, size_type cnt) const noexcept
  {
    return (pos > length or cnt > length or (pos + cnt) > length);
  }

  inline __attribute__((always_inline)) bool
  __capacity_exceed(size_type n) const noexcept
  {
    return (n > N);
  }

  inline __attribute__((always_inline)) bool
  __iterator_check(const T *itr) const noexcept
  {
    return (itr < &memory[0] or itr > &memory[length]);
  }

  inline __attribute__((always_inline)) bool
  __erase_ind_check(size_type ind, size_type cnt) const noexcept
  {
    return (ind >= length or cnt > (length - ind));
  }

  inline __attribute__((always_inline)) bool
  __erase_iter_check(const T *itr, size_type cnt) const noexcept
  {
    return (itr < &memory[0] or itr > &memory[length] or static_cast<max_t>(cnt) > (&memory[length] - itr));
  }

  inline __attribute__((always_inline)) bool
  __iter_substr_check(const T *start, const T *end_) const noexcept
  {
    return (start < &memory[0] or end_ > &memory[length] or start > end_);
  }

  inline __attribute__((always_inline)) bool
  __at_iter_check(const T *itr) const noexcept
  {
    auto diff = itr - &memory[0];
    return (diff < 0 or static_cast<size_type>(diff) > length);
  }

  inline __attribute__((always_inline)) bool
  __insert_ind_check(size_type ind, size_type cnt) const noexcept
  {
    return (ind > length or cnt > N - length);
  }

  inline __attribute__((always_inline)) bool
  __insert_iter_check(const T *itr, size_type required) const noexcept
  {
    return (itr < &memory[0] or itr > &memory[length] or length + required >= N);
  }

  template<auto Fn, typename E, typename... Args>
  inline __attribute__((always_inline)) void
  __safety_check(const char *msg, Args &&...args) const
  {
    if constexpr ( Sf == true ) {
      if ( (this->*Fn)(micron::forward<Args>(args)...) ) {
        exc<E>(msg);
      }
    }
  }

  inline __attribute__((always_inline)) int
  __lexcmp(const T *a, size_type alen, const T *b, size_type blen) const noexcept
  {
    const size_type common = alen < blen ? alen : blen;
    if ( common ) {
      const i64 d = micron::memcmp<byte>(reinterpret_cast<const byte *>(a), reinterpret_cast<const byte *>(b), common);
      if ( d != 0 ) return d < 0 ? -1 : 1;
    }
    if ( alen < blen ) return -1;
    if ( alen > blen ) return 1;
    return 0;
  }

  [[gnu::always_inline]] static inline size_type
  __simd_find_byte(const T *p, size_type len, T ch) noexcept
  {
    if ( len == 0 ) return npos;
#if defined(__micron_x86_avx2)
    const size_type i = micron::simd::find_first_set_256(p, len, static_cast<char>(ch));
#else
    const size_type i = micron::simd::find_first_set_128(p, len, static_cast<char>(ch));
#endif
    return i == len ? npos : i;
  }

  [[gnu::always_inline]] static inline size_type
  __simd_rfind_byte(const T *p, size_type len, T ch) noexcept
  {
    if ( len == 0 ) return npos;
#if defined(__micron_x86_avx2)
    auto *r = micron::simd::memrchr256(reinterpret_cast<const byte *>(p), static_cast<u8>(ch), len);
#else
    auto *r = micron::simd::memrchr128(reinterpret_cast<const byte *>(p), static_cast<u8>(ch), len);
#endif
    return r == nullptr ? npos : static_cast<size_type>(reinterpret_cast<const T *>(r) - p);
  }

  template<size_type K>
  [[gnu::always_inline]] static inline size_type
  __simd_find_first_of_small(const T *p, size_type len, const T *chars, size_type pos) noexcept
  {
    static_assert(K >= 1 && K <= 8);
    size_type i = pos;
#if defined(__micron_x86_avx2)
    __m256i cv[K];
    for ( size_type k = 0; k < K; ++k ) cv[k] = _mm256_set1_epi8(static_cast<char>(chars[k]));
    for ( ; i + 32 <= len; i += 32 ) {
      __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p + i));
      __m256i any = _mm256_cmpeq_epi8(v, cv[0]);
      for ( size_type k = 1; k < K; ++k ) any = _mm256_or_si256(any, _mm256_cmpeq_epi8(v, cv[k]));
      u32 m = static_cast<u32>(_mm256_movemask_epi8(any));
      if ( m ) return i + __builtin_ctz(m);
    }
#endif
#if defined(__micron_x86_sse2)
    __m128i cv128[K];
    for ( size_type k = 0; k < K; ++k ) cv128[k] = _mm_set1_epi8(static_cast<char>(chars[k]));
    for ( ; i + 16 <= len; i += 16 ) {
      __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p + i));
      __m128i any = _mm_cmpeq_epi8(v, cv128[0]);
      for ( size_type k = 1; k < K; ++k ) any = _mm_or_si128(any, _mm_cmpeq_epi8(v, cv128[k]));
      u32 m = static_cast<u32>(_mm_movemask_epi8(any));
      if ( m ) return i + __builtin_ctz(m);
    }
#elif defined(__micron_arm_neon)
    micron::simd::__bits::uint8x16_t cv_n[K];
    for ( size_type k = 0; k < K; ++k ) cv_n[k] = vdupq_n_u8(static_cast<u8>(chars[k]));
    for ( ; i + 16 <= len; i += 16 ) {
      auto v = vld1q_u8(reinterpret_cast<const u8 *>(p + i));
      auto any = vceqq_u8(v, cv_n[0]);
      for ( size_type k = 1; k < K; ++k ) any = vorrq_u8(any, vceqq_u8(v, cv_n[k]));
      u32 m = micron::simd::__neon_movemask_u8(any);
      if ( m ) return i + __builtin_ctz(m);
    }
#endif
    for ( ; i < len; ++i )
      for ( size_type k = 0; k < K; ++k )
        if ( p[i] == chars[k] ) return i;
    return npos;
  }

  [[gnu::always_inline]] static inline void
  __build_charset_bitmap(byte (&bm)[32], const T *chars, size_type k) noexcept
  {
    for ( int i = 0; i < 32; ++i ) bm[i] = 0;
    for ( size_type j = 0; j < k; ++j ) {
      u8 c = static_cast<u8>(chars[j]);
      bm[c >> 3] |= static_cast<byte>(1u << (c & 7));
    }
  }

  [[gnu::always_inline]] static inline bool
  __charset_test(const byte (&bm)[32], u8 c) noexcept
  {
    return (bm[c >> 3] & static_cast<byte>(1u << (c & 7))) != 0;
  }

public:
  constexpr ~sstring()
  {
    if constexpr ( Sf ) {
      micron::czero<N>(&memory[0]);
    }
  }

  constexpr sstring()
  {
    micron::czero<N>(&memory[0]);
    length = 0;
  };

  constexpr sstring(const char *str)
  {
    micron::constexpr_zero(&memory[0], N);
    size_type sz = strlen(str);
    if ( sz > N ) exc<except::library_error>("sstring::sstring() const char* too large.");
    micron::memcpy(&memory[0], &str[0], sz + 1);
    length = sz;
  };

  constexpr sstring(char *ptr)
  {
    size_type n = micron::strlen(ptr);
    if ( n >= N ) exc<except::library_error>("sstring::sstring(): char* too large.");
    micron::constexpr_zero(&memory[0], N);
    micron::memcpy(&memory[0], ptr, n);
    length = n;
  }

  template<size_type M, typename F> constexpr sstring(const F (&str)[M])
  {
    static_assert(N >= M, "micron::sstring sstring(cconst) too large.");
    micron::memcpy(&memory[0], &str[0], M);
    length = M - 1;      // cut null
  };

  // allow construction from - to iterator (be careful!)

  constexpr sstring(iterator __start, iterator __end)
  {
    if ( __start >= __end ) exc<except::library_error>("micron::sstring sstring() wrong iterators");
    if ( static_cast<usize>(__end - __start) + 1 > N ) exc<except::library_error>("sstring(iter,iter): range too large.");
    micron::constexpr_zero(&memory[0], N);
    micron::memcpy(&memory[0], __start, __end - __start);
    length = __end - __start;
  };

  constexpr sstring(const_iterator __start, const_iterator __end)
  {
    if ( __start >= __end ) exc<except::library_error>("micron::sstring sstring() wrong iterators");
    if ( static_cast<usize>(__end - __start) + 1 > N ) exc<except::library_error>("sstring(iter,iter): range too large.");
    micron::constexpr_zero(&memory[0], N);
    micron::memcpy(&memory[0], __start, __end - __start);
    length = __end - __start;
  };

  template<size_type M, typename F, bool X = Sf> constexpr sstring(const sstring<M, F, X> &o)
  {
    micron::constexpr_zero(&memory[0], N);
    if ( o.empty() ) {
      length = 0;
      return;
    }
    if constexpr ( N < M ) {
      micron::memcpy(&memory[0], &o.memory[0], N);
    } else if constexpr ( N >= M ) {
      micron::memcpy(&memory[0], &o.memory[0], M);
    }
    if ( o.length > N )
      length = N;
    else
      length = o.length;
  };

  template<is_string S> constexpr sstring(const S &o)
  {
    micron::constexpr_zero(&memory[0], N);
    micron::memcpy(&memory[0], &o.memory[0], N);
    length = o.length;
  };

  constexpr sstring(const sstring &o)
  {
    micron::constexpr_zero(&memory[0], N);
    micron::memcpy(&memory[0], &o.memory[0], N);
    length = o.length;
  };

  constexpr sstring(sstring &&o)
  {
    micron::memcpy(&memory[0], &o.memory[0], N);
    micron::constexpr_zero(&o.memory[0], N);
    length = o.length;
    o.length = 0;
  };

  template<size_type M, typename F, bool X = Sf> constexpr sstring(sstring<M, F, X> &&o)
  {
    if constexpr ( N < M ) {
      micron::memcpy(&memory[0], &o.memory[0], N);
      micron::constexpr_zero(&o.memory[0], N);
    } else if constexpr ( N >= M ) {
      micron::memcpy(&memory[0], &o.memory[0], M);
      micron::memcpy(&memory[0], &o.memory[0], M);
      micron::constexpr_zero(&o.memory[0], N);
    }
    if ( o.length > N )
      length = N;
    else
      length = o.length;
    o.length = 0;
  };

  sstring &
  operator=(sstring &&o)
  {
    if ( o.empty() ) {
      length = 0;
      return *this;
    }
    clear();
    micron::memcpy(&memory[0], &o.memory[0], N);
    length = o.length;
    micron::czero<N>(&o.memory[0]);
    o.length = 0;
    return *this;
  }

  sstring &
  operator=(const sstring &o)
  {
    if ( o.empty() ) {
      length = 0;
      return *this;
    }
    clear();
    micron::memcpy(&memory[0], &o.memory[0], N);
    length = o.length;
    return *this;
  }

  template<typename F>
  sstring &
  operator=(const F &o)
  {
    if ( o.empty() ) {
      length = 0;
      return *this;
    }
    clear();
    if ( o.size() < N ) {
      micron::memcpy(&memory[0], &o.cdata()[0], o.size());
      length = o.size();
    } else {
      micron::memcpy(&memory[0], &o.cdata()[0], N);
      length = o.size();
    }
    return *this;
  }

  template<size_type M, typename F, bool X = Sf>
  sstring &
  operator=(const sstring<M, F, X> &o)
  {
    if ( o.empty() ) {
      length = 0;
      return *this;
    }
    static_assert(N >= M, "micron::sstring operator= too large.");
    clear();
    micron::memcpy(&memory[0], &o.memory[0], M);
    length = o.length;
    return *this;
  }

  template<size_type M, typename F, bool X = Sf>
  sstring &
  operator=(sstring<M, F, X> &&o)
  {
    if ( o.empty() ) {
      length = 0;
      return *this;
    }
    static_assert(N >= M, "micron::sstring operator= too large.");
    clear();
    micron::memcpy(&memory[0], &o.memory[0], M);
    micron::constexpr_zero(&o.memory[0], M);
    length = o.length;
    o.length = 0;
    return *this;
  }

  sstring &
  operator=(char *ptr)
  {
    if ( ptr == nullptr ) return *this;
    clear();
    size_type n = strlen(ptr);
    micron::constexpr_zero(&memory[0], N);
    micron::memcpy(&memory[0], &ptr[0], n);
    length = n;
    return *this;
  }

  sstring &
  operator=(const char *ptr)
  {
    if ( ptr == nullptr ) return *this;
    clear();
    size_type n = strlen(ptr);
    micron::constexpr_zero(&memory[0], N);
    micron::memcpy(&memory[0], &ptr[0], n);
    length = n;
    return *this;
  }

  template<typename F>
  sstring &
  operator=(const F (&str)[N])
  {
    clear();
    micron::memcpy(&memory, &str, N);
    length = N;
    return *this;
  }

  constexpr bool
  operator!() const
  {
    return empty();
  }

  // overload this to always point to mem
  byte *
  operator&()
  {
    return reinterpret_cast<byte *>(&memory[0]);
  }

  const byte *
  operator&() const
  {
    return reinterpret_cast<const byte *>(&memory[0]);
  }

  constexpr auto *
  addr()
  {
    return this;
  }

  constexpr const auto *
  addr() const
  {
    return this;
  }

  constexpr bool
  empty() const
  {
    return (bool)length == 0;
  };

  constexpr void
  set_size(size_type n)
  {
    length = n;
  }

  constexpr size_type
  len(void) const
  {
    return length;
  }

  constexpr size_type
  size(void) const
  {
    return length;
  }

  constexpr size_type
  capacity() const
  {
    return N;
  }

  constexpr size_type
  max_size(void) const
  {
    return N;
  }

  const auto &
  cdata() const
  {
    return memory;
  }

  auto
  data() -> pointer
  {
    return &memory[0];
  }

  auto
  data() const -> const_pointer
  {
    return &memory[0];
  }

  inline constexpr const char *
  c_str()
  {
    return const_cast<const char *>(reinterpret_cast<char *>(&memory[0]));
  }

  inline constexpr const char *
  c_str() const
  {
    return const_cast<const char *>(reinterpret_cast<const char *>(&memory[0]));
  }

  inline span<char, N * (sizeof(T) / sizeof(char))>
  into_chars()
  {
    return span<char, N * (sizeof(T) / sizeof(char))>(reinterpret_cast<char *>(&memory[0]), reinterpret_cast<char *>(&memory[length]));
  }

  inline span<byte, N * (sizeof(T) / sizeof(byte))>
  into_bytes()
  {
    return span<byte, N * (sizeof(T) / sizeof(byte))>(reinterpret_cast<byte *>(&memory[0]), reinterpret_cast<byte *>(&memory[length]));
  }

  inline auto
  clone()
  {
    return sstring<N, T>(*this);      // copy
  }

  template<typename F>
  inline F
  clone_to()
  {
    return F(*this);      // copy
  }

  inline void
  _buf_set_length(const size_type s)
  {
    length = s;
  }

  void
  adjust_size()
  {
    auto ln = micron::strlen(memory);
    length = ln;
  }

  inline size_type
  rfind(char ch, size_type pos = npos) const
  {
    if ( length == 0 ) return npos;
    size_type lim = (pos == npos || pos >= length) ? length : pos + 1;
    return __simd_rfind_byte(memory, lim, static_cast<T>(ch));
  }

  inline size_type
  rfind(const T *needle, size_type pos = npos) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring rfind() null needle", static_cast<const void *>(needle));
    return __rfind_substr(needle, micron::strlen(needle), pos);
  }

  template<size_type M>
  inline size_type
  rfind(const T (&needle)[M], size_type pos = npos) const
  {
    constexpr size_type needle_len = M - 1;
    return __rfind_substr(&needle[0], needle_len, pos);
  }

  template<size_type M, typename F, bool X = Sf>
  inline size_type
  rfind(const sstring<M, F, X> &str, size_type pos = npos) const
  {
    return __rfind_substr(reinterpret_cast<const T *>(str.memory), str.length, pos);
  }

  inline size_type
  find_first_of(const T *chars, size_type pos = 0) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring find_first_of() null", static_cast<const void *>(chars));
    if ( pos >= length ) return npos;
    size_type k = micron::strlen(chars);
    if ( k == 0 ) return npos;
    if ( k == 1 )
      return __simd_find_byte(memory + pos, length - pos, chars[0]) == npos ? npos
                                                                            : pos + __simd_find_byte(memory + pos, length - pos, chars[0]);
    if ( k <= 8 ) {
      if ( k == 2 ) return __simd_find_first_of_small<2>(memory, length, chars, pos);
      if ( k == 3 ) return __simd_find_first_of_small<3>(memory, length, chars, pos);
      if ( k == 4 ) return __simd_find_first_of_small<4>(memory, length, chars, pos);
      if ( k == 5 ) return __simd_find_first_of_small<5>(memory, length, chars, pos);
      if ( k == 6 ) return __simd_find_first_of_small<6>(memory, length, chars, pos);
      if ( k == 7 ) return __simd_find_first_of_small<7>(memory, length, chars, pos);
      return __simd_find_first_of_small<8>(memory, length, chars, pos);
    }
    byte bm[32];
    __build_charset_bitmap(bm, chars, k);
    for ( size_type i = pos; i < length; ++i )
      if ( __charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  template<size_type M>
  inline size_type
  find_first_of(const T (&chars)[M], size_type pos = 0) const
  {
    constexpr size_type k = M - 1;
    if constexpr ( k == 0 ) return npos;
    if ( pos >= length ) return npos;
    if constexpr ( k == 1 ) {
      size_type r = __simd_find_byte(memory + pos, length - pos, chars[0]);
      return r == npos ? npos : pos + r;
    } else if constexpr ( k <= 8 ) {
      return __simd_find_first_of_small<k>(memory, length, &chars[0], pos);
    } else {
      byte bm[32];
      __build_charset_bitmap(bm, &chars[0], k);
      for ( size_type i = pos; i < length; ++i )
        if ( __charset_test(bm, static_cast<u8>(memory[i])) ) return i;
      return npos;
    }
  }

  template<size_type M, typename F, bool X = Sf>
  inline size_type
  find_first_of(const sstring<M, F, X> &chars, size_type pos = 0) const
  {
    if ( pos >= length || chars.length == 0 ) return npos;
    const T *cp = reinterpret_cast<const T *>(chars.memory);
    if ( chars.length == 1 ) {
      size_type r = __simd_find_byte(memory + pos, length - pos, cp[0]);
      return r == npos ? npos : pos + r;
    }
    if ( chars.length <= 8 ) {
      switch ( chars.length ) {
      case 2:
        return __simd_find_first_of_small<2>(memory, length, cp, pos);
      case 3:
        return __simd_find_first_of_small<3>(memory, length, cp, pos);
      case 4:
        return __simd_find_first_of_small<4>(memory, length, cp, pos);
      case 5:
        return __simd_find_first_of_small<5>(memory, length, cp, pos);
      case 6:
        return __simd_find_first_of_small<6>(memory, length, cp, pos);
      case 7:
        return __simd_find_first_of_small<7>(memory, length, cp, pos);
      case 8:
        return __simd_find_first_of_small<8>(memory, length, cp, pos);
      }
    }
    byte bm[32];
    __build_charset_bitmap(bm, cp, chars.length);
    for ( size_type i = pos; i < length; ++i )
      if ( __charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  inline size_type
  find_last_of(const T *chars, size_type pos = npos) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring find_last_of() null", static_cast<const void *>(chars));
    if ( length == 0 ) return npos;
    size_type k = micron::strlen(chars);
    if ( k == 0 ) return npos;
    size_type end = (pos == npos || pos >= length) ? length : pos + 1;
    if ( k == 1 ) return __simd_rfind_byte(memory, end, chars[0]);
    byte bm[32];
    __build_charset_bitmap(bm, chars, k);
    for ( size_type i = end; i-- > 0; )
      if ( __charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  template<size_type M>
  inline size_type
  find_last_of(const T (&chars)[M], size_type pos = npos) const
  {
    constexpr size_type k = M - 1;
    if constexpr ( k == 0 ) return npos;
    if ( length == 0 ) return npos;
    size_type end = (pos == npos || pos >= length) ? length : pos + 1;
    if constexpr ( k == 1 ) return __simd_rfind_byte(memory, end, chars[0]);
    byte bm[32];
    __build_charset_bitmap(bm, &chars[0], k);
    for ( size_type i = end; i-- > 0; )
      if ( __charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  template<size_type M, typename F, bool X = Sf>
  inline size_type
  find_last_of(const sstring<M, F, X> &chars, size_type pos = npos) const
  {
    if ( length == 0 || chars.length == 0 ) return npos;
    size_type end = (pos == npos || pos >= length) ? length : pos + 1;
    if ( chars.length == 1 ) return __simd_rfind_byte(memory, end, static_cast<T>(chars.memory[0]));
    byte bm[32];
    __build_charset_bitmap(bm, reinterpret_cast<const T *>(chars.memory), chars.length);
    for ( size_type i = end; i-- > 0; )
      if ( __charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  inline size_type
  find_first_not_of(const T *chars, size_type pos = 0) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring find_first_not_of() null",
                                                                  static_cast<const void *>(chars));
    if ( pos >= length ) return npos;
    size_type k = micron::strlen(chars);
    if ( k == 0 ) return pos;
    byte bm[32];
    __build_charset_bitmap(bm, chars, k);
    for ( size_type i = pos; i < length; ++i )
      if ( !__charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  template<size_type M>
  inline size_type
  find_first_not_of(const T (&chars)[M], size_type pos = 0) const
  {
    constexpr size_type k = M - 1;
    if ( pos >= length ) return npos;
    if constexpr ( k == 0 ) return pos;
    byte bm[32];
    __build_charset_bitmap(bm, &chars[0], k);
    for ( size_type i = pos; i < length; ++i )
      if ( !__charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  template<size_type M, typename F, bool X = Sf>
  inline size_type
  find_first_not_of(const sstring<M, F, X> &chars, size_type pos = 0) const
  {
    if ( pos >= length ) return npos;
    if ( chars.length == 0 ) return pos;
    byte bm[32];
    __build_charset_bitmap(bm, reinterpret_cast<const T *>(chars.memory), chars.length);
    for ( size_type i = pos; i < length; ++i )
      if ( !__charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  inline size_type
  find_last_not_of(const T *chars, size_type pos = npos) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring find_last_not_of() null",
                                                                  static_cast<const void *>(chars));
    if ( length == 0 ) return npos;
    size_type k = micron::strlen(chars);
    size_type end = (pos == npos || pos >= length) ? length : pos + 1;
    if ( k == 0 ) return end - 1;
    byte bm[32];
    __build_charset_bitmap(bm, chars, k);
    for ( size_type i = end; i-- > 0; )
      if ( !__charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  template<size_type M>
  inline size_type
  find_last_not_of(const T (&chars)[M], size_type pos = npos) const
  {
    constexpr size_type k = M - 1;
    if ( length == 0 ) return npos;
    size_type end = (pos == npos || pos >= length) ? length : pos + 1;
    if constexpr ( k == 0 ) return end - 1;
    byte bm[32];
    __build_charset_bitmap(bm, &chars[0], k);
    for ( size_type i = end; i-- > 0; )
      if ( !__charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  template<size_type M, typename F, bool X = Sf>
  inline size_type
  find_last_not_of(const sstring<M, F, X> &chars, size_type pos = npos) const
  {
    if ( length == 0 ) return npos;
    size_type end = (pos == npos || pos >= length) ? length : pos + 1;
    if ( chars.length == 0 ) return end - 1;
    byte bm[32];
    __build_charset_bitmap(bm, reinterpret_cast<const T *>(chars.memory), chars.length);
    for ( size_type i = end; i-- > 0; )
      if ( !__charset_test(bm, static_cast<u8>(memory[i])) ) return i;
    return npos;
  }

  inline bool
  starts_with(T ch) const
  {
    return length > 0 && memory[0] == ch;
  }

  inline bool
  starts_with(const T *prefix) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring starts_with() null", static_cast<const void *>(prefix));
    size_type plen = micron::strlen(prefix);
    if ( plen > length ) return false;
    if ( plen == 0 ) return true;
    return micron::memcmp<byte>(reinterpret_cast<const byte *>(memory), reinterpret_cast<const byte *>(prefix), plen) == 0;
  }

  template<size_type M>
  inline bool
  starts_with(const T (&prefix)[M]) const
  {
    constexpr size_type plen = M - 1;
    if constexpr ( plen == 0 ) return true;
    if ( plen > length ) return false;
    return micron::memcmp<byte>(reinterpret_cast<const byte *>(memory), reinterpret_cast<const byte *>(&prefix[0]), plen) == 0;
  }

  template<size_type M, typename F, bool X = Sf>
  inline bool
  starts_with(const sstring<M, F, X> &prefix) const
  {
    if ( prefix.empty() ) return true;
    if ( prefix.length > length ) return false;
    return micron::memcmp<byte>(reinterpret_cast<const byte *>(memory), reinterpret_cast<const byte *>(prefix.memory), prefix.length) == 0;
  }

  inline bool
  ends_with(T ch) const
  {
    return length > 0 && memory[length - 1] == ch;
  }

  inline bool
  ends_with(const T *suffix) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring ends_with() null", static_cast<const void *>(suffix));
    size_type slen = micron::strlen(suffix);
    if ( slen > length ) return false;
    if ( slen == 0 ) return true;
    return micron::memcmp<byte>(reinterpret_cast<const byte *>(memory + (length - slen)), reinterpret_cast<const byte *>(suffix), slen)
           == 0;
  }

  template<size_type M>
  inline bool
  ends_with(const T (&suffix)[M]) const
  {
    constexpr size_type slen = M - 1;
    if constexpr ( slen == 0 ) return true;
    if ( slen > length ) return false;
    return micron::memcmp<byte>(reinterpret_cast<const byte *>(memory + (length - slen)), reinterpret_cast<const byte *>(&suffix[0]), slen)
           == 0;
  }

  template<size_type M, typename F, bool X = Sf>
  inline bool
  ends_with(const sstring<M, F, X> &suffix) const
  {
    if ( suffix.empty() ) return true;
    if ( suffix.length > length ) return false;
    return micron::memcmp<byte>(reinterpret_cast<const byte *>(memory + (length - suffix.length)),
                                reinterpret_cast<const byte *>(suffix.memory), suffix.length)
           == 0;
  }

  inline bool
  contains(T ch) const
  {
    return find(ch) != npos;
  }

  inline bool
  contains(const T *needle) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring contains() null", static_cast<const void *>(needle));
    return find_substr(needle, micron::strlen(needle)) != npos;
  }

  template<size_type M>
  inline bool
  contains(const T (&needle)[M]) const
  {
    constexpr size_type needle_len = M - 1;
    if constexpr ( needle_len == 0 ) return true;
    return find_substr(&needle[0], needle_len) != npos;
  }

  template<size_type M, typename F, bool X = Sf>
  inline bool
  contains(const sstring<M, F, X> &needle) const
  {
    if ( needle.empty() ) return true;
    return find_substr(reinterpret_cast<const T *>(needle.memory), needle.length) != npos;
  }

  inline sstring &
  replace(size_type pos, size_type cnt, const T *with)
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring replace() null replacement",
                                                                  static_cast<const void *>(with));
    __safety_check<&sstring::__range_pos_cnt, except::library_error>("micron::sstring replace() out of range", pos, cnt);
    return __replace_impl(pos, cnt, with, micron::strlen(with));
  }

  template<size_type M>
  inline sstring &
  replace(size_type pos, size_type cnt, const T (&with)[M])
  {
    __safety_check<&sstring::__range_pos_cnt, except::library_error>("micron::sstring replace() out of range", pos, cnt);
    return __replace_impl(pos, cnt, &with[0], M - 1);
  }

  template<size_type M, typename F, bool X = Sf>
  inline sstring &
  replace(size_type pos, size_type cnt, const sstring<M, F, X> &with)
  {
    __safety_check<&sstring::__range_pos_cnt, except::library_error>("micron::sstring replace() out of range", pos, cnt);
    return __replace_impl(pos, cnt, reinterpret_cast<const T *>(with.memory), with.length);
  }

  inline sstring &
  replace(iterator first, iterator last, const T *with)
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring replace() null replacement",
                                                                  static_cast<const void *>(with));
    __safety_check<&sstring::__iterator_check, except::library_error>("micron::sstring replace() iterator out of range", first);
    return __replace_impl(static_cast<size_type>(first - &memory[0]), static_cast<size_type>(last - first), with, micron::strlen(with));
  }

  inline sstring &
  replace_all(const T *needle, const T *with)
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring replace_all() null needle",
                                                                  static_cast<const void *>(needle));
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring replace_all() null replacement",
                                                                  static_cast<const void *>(with));
    size_type needle_len = micron::strlen(needle);
    size_type with_len = micron::strlen(with);
    if ( needle_len == 0 ) return *this;
    size_type pos = 0;
    while ( (pos = find_substr(needle, needle_len, pos)) != npos ) {
      __replace_impl(pos, needle_len, with, with_len);
      pos += with_len;      // advance past replacement to avoid re-matching
    }
    return *this;
  }

  template<size_type M, size_type K>
  inline sstring &
  replace_all(const T (&needle)[M], const T (&with)[K])
  {
    constexpr size_type needle_len = M - 1;
    constexpr size_type with_len = K - 1;
    if constexpr ( needle_len == 0 ) return *this;
    size_type pos = 0;
    while ( (pos = find_substr(&needle[0], needle_len, pos)) != npos ) {
      __replace_impl(pos, needle_len, &with[0], with_len);
      pos += with_len;
    }
    return *this;
  }

  template<size_type M, size_type K, typename F, typename G, bool X = Sf, bool Y = Sf>
  inline sstring &
  replace_all(const sstring<M, F, X> &needle, const sstring<K, G, Y> &with)
  {
    if ( needle.empty() ) return *this;
    size_type pos = 0;
    while ( (pos = find_substr(reinterpret_cast<const T *>(needle.memory), needle.length, pos)) != npos ) {
      __replace_impl(pos, needle.length, reinterpret_cast<const T *>(with.memory), with.length);
      pos += with.length;
    }
    return *this;
  }

  inline int
  compare(const T *other) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring compare() null", static_cast<const void *>(other));
    return __lexcmp(memory, length, other, micron::strlen(other));
  }

  template<size_type M>
  inline int
  compare(const T (&other)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&other[0]), M - 1);
  }

  template<size_type M, typename F, bool X = Sf>
  inline int
  compare(const sstring<M, F, X> &other) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(other.memory), other.length);
  }

  size_type
  find(char ch, size_type pos = 0) const
  {
    if ( pos >= length ) return npos;
    size_type r = __simd_find_byte(memory + pos, length - pos, static_cast<T>(ch));
    return r == npos ? npos : pos + r;
  }

  template<typename F>
  size_type
  find(F ch, size_type pos = 0) const
  {
    if ( pos >= length ) return npos;
    size_type r = __simd_find_byte(memory + pos, length - pos, static_cast<T>(ch));
    return r == npos ? npos : pos + r;
  }

  size_type
  find(const T *needle, size_type pos = 0) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring find() null needle", static_cast<const void *>(needle));

    size_type needle_len = micron::strlen(needle);
    if ( needle_len == 0 ) return pos;      // vacuous match, mirrors std::string

    return find_substr(reinterpret_cast<const T *>(needle), needle_len, pos);
  }

  template<size_type M>
  size_type
  find(const T (&needle)[M], size_type pos = 0) const
  {
    constexpr size_type needle_len = M - 1;
    if constexpr ( needle_len == 0 ) return pos;

    return find_substr(&needle[0], needle_len, pos);
  }

  template<size_type M, typename F, bool X = Sf>
  size_type
  find(const sstring<M, F, X> &str, size_type pos = 0) const
  {
    if ( str.empty() ) return pos;      // vacuous match

    return find_substr(reinterpret_cast<const T *>(str.memory), str.length, pos);
  }

  inline sstring &
  to_lower()
  {
    size_type i = 0;
#if defined(__micron_x86_avx2)
    const __m256i vA = _mm256_set1_epi8('A');
    const __m256i v26b = _mm256_set1_epi8(26 - 0x80);
    const __m256i v80 = _mm256_set1_epi8(static_cast<char>(0x80));
    const __m256i vSpace = _mm256_set1_epi8(0x20);
    for ( ; i + 32 <= length; i += 32 ) {
      __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&memory[i]));
      __m256i shifted = _mm256_sub_epi8(v, vA);
      __m256i biased = _mm256_sub_epi8(shifted, v80);
      __m256i mask = _mm256_cmpgt_epi8(v26b, biased);
      __m256i delta = _mm256_and_si256(mask, vSpace);
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(&memory[i]), _mm256_or_si256(v, delta));
    }
#endif
#if defined(__micron_x86_sse2)
    const __m128i vA128 = _mm_set1_epi8('A');
    const __m128i v26b128 = _mm_set1_epi8(26 - 0x80);
    const __m128i v80_128 = _mm_set1_epi8(static_cast<char>(0x80));
    const __m128i vSpace128 = _mm_set1_epi8(0x20);
    for ( ; i + 16 <= length; i += 16 ) {
      __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&memory[i]));
      __m128i shifted = _mm_sub_epi8(v, vA128);
      __m128i biased = _mm_sub_epi8(shifted, v80_128);
      __m128i mask = _mm_cmpgt_epi8(v26b128, biased);
      __m128i delta = _mm_and_si128(mask, vSpace128);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(&memory[i]), _mm_or_si128(v, delta));
    }
#elif defined(__micron_arm_neon)
    const micron::simd::__bits::uint8x16_t vA_n = vdupq_n_u8('A');
    const micron::simd::__bits::uint8x16_t v26_n = vdupq_n_u8(26);
    const micron::simd::__bits::uint8x16_t vSpace_n = vdupq_n_u8(0x20);
    for ( ; i + 16 <= length; i += 16 ) {
      auto v = vld1q_u8(reinterpret_cast<const u8 *>(&memory[i]));
      auto shifted = vsubq_u8(v, vA_n);
      auto mask = vcgtq_u8(v26_n, shifted);      // unsigned: 26 > shifted ↔ shifted in [0,25]
      auto delta = vandq_u8(mask, vSpace_n);
      vst1q_u8(reinterpret_cast<u8 *>(&memory[i]), vorrq_u8(v, delta));
    }
#endif
    for ( ; i < length; ++i )
      if ( memory[i] >= static_cast<T>('A') && memory[i] <= static_cast<T>('Z') ) memory[i] += static_cast<T>('a' - 'A');
    return *this;
  }

  inline sstring &
  to_upper()
  {
    size_type i = 0;
#if defined(__micron_x86_avx2)
    const __m256i va = _mm256_set1_epi8('a');
    const __m256i v26b = _mm256_set1_epi8(26 - 0x80);
    const __m256i v80 = _mm256_set1_epi8(static_cast<char>(0x80));
    const __m256i vSpace = _mm256_set1_epi8(0x20);
    for ( ; i + 32 <= length; i += 32 ) {
      __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&memory[i]));
      __m256i shifted = _mm256_sub_epi8(v, va);
      __m256i biased = _mm256_sub_epi8(shifted, v80);
      __m256i mask = _mm256_cmpgt_epi8(v26b, biased);
      __m256i delta = _mm256_and_si256(mask, vSpace);
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(&memory[i]), _mm256_andnot_si256(delta, v));
    }
#endif
#if defined(__micron_x86_sse2)
    const __m128i va128 = _mm_set1_epi8('a');
    const __m128i v26b128 = _mm_set1_epi8(26 - 0x80);
    const __m128i v80_128 = _mm_set1_epi8(static_cast<char>(0x80));
    const __m128i vSpace128 = _mm_set1_epi8(0x20);
    for ( ; i + 16 <= length; i += 16 ) {
      __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&memory[i]));
      __m128i shifted = _mm_sub_epi8(v, va128);
      __m128i biased = _mm_sub_epi8(shifted, v80_128);
      __m128i mask = _mm_cmpgt_epi8(v26b128, biased);
      __m128i delta = _mm_and_si128(mask, vSpace128);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(&memory[i]), _mm_andnot_si128(delta, v));
    }
#elif defined(__micron_arm_neon)
    // has no native ANDNOT wrapper for u8 on arm32
    const micron::simd::__bits::uint8x16_t va_n = vdupq_n_u8('a');
    const micron::simd::__bits::uint8x16_t v26_n = vdupq_n_u8(26);
    const micron::simd::__bits::uint8x16_t vSpace_n = vdupq_n_u8(0x20);
    for ( ; i + 16 <= length; i += 16 ) {
      auto v = vld1q_u8(reinterpret_cast<const u8 *>(&memory[i]));
      auto shifted = vsubq_u8(v, va_n);
      auto mask = vcgtq_u8(v26_n, shifted);
      auto delta = vandq_u8(mask, vSpace_n);
      vst1q_u8(reinterpret_cast<u8 *>(&memory[i]), veorq_u8(v, delta));
    }
#endif
    for ( ; i < length; ++i )
      if ( memory[i] >= static_cast<T>('a') && memory[i] <= static_cast<T>('z') ) memory[i] -= static_cast<T>('a' - 'A');
    return *this;
  }

  inline sstring &
  trim_left()
  {
    static const T ws[4] = { static_cast<T>(' '), static_cast<T>('\t'), static_cast<T>('\n'), static_cast<T>('\r') };
    size_type i = 0;
#if defined(__micron_x86_avx2)
    {
      __m256i cv[4];
      for ( int k = 0; k < 4; ++k ) cv[k] = _mm256_set1_epi8(static_cast<char>(ws[k]));
      for ( ; i + 32 <= length; i += 32 ) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&memory[i]));
        __m256i any = _mm256_or_si256(_mm256_or_si256(_mm256_cmpeq_epi8(v, cv[0]), _mm256_cmpeq_epi8(v, cv[1])),
                                      _mm256_or_si256(_mm256_cmpeq_epi8(v, cv[2]), _mm256_cmpeq_epi8(v, cv[3])));
        u32 m = static_cast<u32>(_mm256_movemask_epi8(any));
        if ( m != 0xFFFFFFFFu ) {
          i += __builtin_ctz(~m);
          goto sstring_trim_left_done;
        }
      }
    }
#elif defined(__micron_arm_neon)
    {
      micron::simd::__bits::uint8x16_t cv_n[4];
      for ( int k = 0; k < 4; ++k ) cv_n[k] = vdupq_n_u8(static_cast<u8>(ws[k]));
      for ( ; i + 16 <= length; i += 16 ) {
        auto v = vld1q_u8(reinterpret_cast<const u8 *>(&memory[i]));
        auto any = vorrq_u8(vorrq_u8(vceqq_u8(v, cv_n[0]), vceqq_u8(v, cv_n[1])), vorrq_u8(vceqq_u8(v, cv_n[2]), vceqq_u8(v, cv_n[3])));
        u32 m = micron::simd::__neon_movemask_u8(any);
        if ( m != 0xFFFFu ) {
          i += __builtin_ctz(static_cast<u32>(~m) & 0xFFFFu);
          goto sstring_trim_left_done;
        }
      }
    }
#endif
    while ( i < length
            && (memory[i] == static_cast<T>(' ') || memory[i] == static_cast<T>('\t') || memory[i] == static_cast<T>('\n')
                || memory[i] == static_cast<T>('\r')) )
      ++i;
#if defined(__micron_x86_avx2) || defined(__micron_arm_neon)
  sstring_trim_left_done:
#endif
    if ( i > length ) i = length;
    if ( i > 0 ) {
      micron::bytemove(&memory[0], &memory[i], length - i);
      micron::typeset<T>(&memory[length - i], 0x0, i);
      length -= i;
    }
    return *this;
  }

  inline sstring &
  trim_right()
  {
    while ( length > 0
            && (memory[length - 1] == static_cast<T>(' ') || memory[length - 1] == static_cast<T>('\t')
                || memory[length - 1] == static_cast<T>('\n') || memory[length - 1] == static_cast<T>('\r')) )
      memory[--length] = 0x0;
    return *this;
  }

  inline sstring &
  trim()
  {
    return trim_left().trim_right();
  }

  inline sstring &
  reverse()
  {
    if ( length <= 1 ) return *this;
    size_type lo = 0, hi = length;
#if defined(__micron_x86_ssse3) || defined(__micron_x86_avx2)
    const __m128i rev = _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    while ( hi >= lo + 32 ) {
      __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&memory[lo]));
      __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&memory[hi - 16]));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(&memory[lo]), _mm_shuffle_epi8(b, rev));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(&memory[hi - 16]), _mm_shuffle_epi8(a, rev));
      lo += 16;
      hi -= 16;
    }
#elif defined(__micron_arm_neon)
    while ( hi >= lo + 32 ) {
      auto a = vld1q_u8(reinterpret_cast<const u8 *>(&memory[lo]));
      auto b = vld1q_u8(reinterpret_cast<const u8 *>(&memory[hi - 16]));
      auto a_rev = vextq_u8(vrev64q_u8(a), vrev64q_u8(a), 8);
      auto b_rev = vextq_u8(vrev64q_u8(b), vrev64q_u8(b), 8);
      vst1q_u8(reinterpret_cast<u8 *>(&memory[lo]), b_rev);
      vst1q_u8(reinterpret_cast<u8 *>(&memory[hi - 16]), a_rev);
      lo += 16;
      hi -= 16;
    }
#endif
    while ( lo + 1 < hi ) {
      T t = memory[lo];
      memory[lo++] = memory[--hi];
      memory[hi] = t;
    }
    return *this;
  }

  inline size_type
  count(T ch) const
  {
    if ( length == 0 ) return 0;
#if defined(__micron_x86_avx2)
    return micron::simd::count_set_256(memory, length, static_cast<char>(ch));
#else
    return micron::simd::count_set_128(memory, length, static_cast<char>(ch));
#endif
  }

  inline size_type
  count(const T *needle) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring count() null needle", static_cast<const void *>(needle));
    size_type needle_len = micron::strlen(needle);
    if ( needle_len == 0 ) return 0;
    size_type n = 0, pos = 0;
    while ( (pos = find_substr(needle, needle_len, pos)) != npos ) {
      ++n;
      pos += needle_len;
    }
    return n;
  }

  template<size_type M>
  inline size_type
  count(const T (&needle)[M]) const
  {
    constexpr size_type needle_len = M - 1;
    if constexpr ( needle_len == 0 ) return 0;
    size_type n = 0, pos = 0;
    while ( (pos = find_substr(&needle[0], needle_len, pos)) != npos ) {
      ++n;
      pos += needle_len;
    }
    return n;
  }

  template<size_type M, typename F, bool X = Sf>
  inline size_type
  count(const sstring<M, F, X> &needle) const
  {
    if ( needle.empty() ) return 0;
    size_type n = 0, pos = 0;
    while ( (pos = find_substr(reinterpret_cast<const T *>(needle.memory), needle.length, pos)) != npos ) {
      ++n;
      pos += needle.length;
    }
    return n;
  }

  inline sstring &
  fill(T ch, size_type cnt = N - 1)
  {
    size_type n = cnt < N ? cnt : N - 1;
    micron::typeset<T>(&memory[0], ch, n);
    memory[n] = 0x0;
    length = n;
    return *this;
  }

  inline sstring &
  repeat(size_type n)
  {
    if ( n == 0 ) {
      clear();
      return *this;
    }
    if ( n == 1 || length == 0 ) return *this;
    size_type orig = length;
    for ( size_type i = 1; i < n; ++i ) {
      if constexpr ( Sf ) {
        if ( length + orig >= N ) exc<except::library_error>("micron::sstring repeat() result exceeds capacity");
      }
      micron::memcpy(&memory[length], &memory[0], orig);
      length += orig;
    }
    return *this;
  }

  inline constexpr const_iterator
  begin() const
  {
    return const_cast<T *>(&memory[0]);
  }

  inline constexpr const_iterator
  end() const
  {
    return const_cast<T *>(&memory[length]);
  }

  inline constexpr iterator
  begin()
  {
    return const_cast<T *>(&memory[0]);
  }

  inline constexpr iterator
  end()
  {
    return const_cast<T *>(&memory[length]);
  }

  inline constexpr iterator
  last()
  {
    return const_cast<T *>(&memory[length - 1]);
  }

  inline constexpr iterator
  last() const
  {
    return const_cast<T *>(&memory[length - 1]);
  }

  inline constexpr const_iterator
  cbegin() const
  {
    return &(memory)[0];
  }

  inline constexpr const_iterator
  cend() const
  {
    return &(memory)[length];
  }

  inline constexpr void
  clear()
  {
    micron::czero<N>(memory);
    length = 0;
  }

  inline constexpr void
  fast_clear()
  {
    length = 0;
  }

  inline sstring &
  pop_back(void)
  {
    if ( (length) > 0 ) (memory)[--length] = 0x0;
    return *this;
  }

  inline sstring &
  push_back(char ch)
  {
    if ( (length + 1) < N ) memory[length++] = ch;
    return *this;
  }

  template<typename F>
  inline sstring &
  push_back(F ch)
  {
    if ( (length + 1) < N ) memory[length++] = ch;
    return *this;
  }

  template<typename F>
  inline sstring &
  pop_back(void)
  {
    if ( length > 0 ) memory[length--] = 0x0;
    return *this;
  }

  template<typename F = T, typename I = size_type>
    requires(micron::is_arithmetic_v<I>)
  inline sstring &
  erase(I __ind, size_type cnt = 1)
  {
    size_type ind = static_cast<size_type>(__ind);

    if ( cnt == 0 ) [[unlikely]]
      return *this;

    __safety_check<&sstring::__erase_ind_check, except::library_error>("micron::sstring erase() out of range", ind, cnt);
    __safety_check<&sstring::__capacity_exceed, except::library_error>("micron::sstring erase() count exceeds capacity", cnt);

    micron::bytemove(&memory[ind], &memory[ind + (1 + (cnt - 1))], length - (ind + 1 + (cnt - 1)));
    micron::typeset<T>(&memory[length - (cnt)], 0x0, cnt);
    length -= cnt;
    return *this;
  }

  template<typename F = T>
  inline sstring &
  erase(iterator itr, size_type cnt = 1)
  {
    if ( cnt == 0 ) [[unlikely]]
      return *this;

    __safety_check<&sstring::__erase_iter_check, except::library_error>("micron::sstring erase() out of range", static_cast<const T *>(itr),
                                                                        cnt);
    __safety_check<&sstring::__capacity_exceed, except::library_error>("micron::sstring erase() count exceeds capacity", cnt);

    micron::bytemove(itr, itr + (1 + (cnt - 1)), length - ((itr - &memory[0]) + 1 + (cnt - 1)));
    micron::typeset<T>(&memory[length - cnt], 0x0, cnt);
    length -= cnt;
    return *this;
  }

  template<typename F = T>
  inline sstring &
  erase(const_iterator itr, size_type cnt = 1)
  {
    __safety_check<&sstring::__erase_iter_check, except::library_error>("micron::sstring erase() out of range", itr, cnt);
    __safety_check<&sstring::__capacity_exceed, except::library_error>("micron::sstring erase() count exceeds capacity", cnt);

    micron::bytemove(itr, itr + (1 + (cnt - 1)), length - ((itr - &memory[0]) + 1 + (cnt - 1)));
    micron::typeset<T>(&memory[length - cnt], 0x0, cnt);
    length -= cnt;
    return *this;
  }

  inline size_type
  find_substr(const T *needle, size_type needle_len, size_type pos = 0) const
  {
    if ( needle_len == 0 || needle_len > length ) return npos;
    if ( pos > length - needle_len ) return npos;
    auto *r = micron::memmem<byte>(reinterpret_cast<const byte *>(memory + pos), length - pos, reinterpret_cast<const byte *>(needle),
                                   needle_len);
    return r == nullptr ? npos : static_cast<size_type>(reinterpret_cast<const T *>(r) - memory);
  }

  inline sstring &
  remove(const char *needle)
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring remove() null needle",
                                                                  static_cast<const void *>(needle));

    size_type needle_len = micron::strlen(needle);
    if ( needle_len == 0 ) return *this;

    size_type pos = find_substr(reinterpret_cast<const T *>(needle), needle_len);
    if ( pos == npos ) return *this;

    micron::bytemove(&memory[pos], &memory[pos + needle_len], length - (pos + needle_len));
    micron::typeset<T>(&memory[length - needle_len], 0x0, needle_len);
    length -= needle_len;
    return *this;
  }

  template<size_type M>
  inline sstring &
  remove(const T (&needle)[M])
  {
    constexpr size_type needle_len = M - 1;
    if constexpr ( needle_len == 0 ) return *this;

    size_type pos = find_substr(&needle[0], needle_len);
    if ( pos == npos ) return *this;

    micron::bytemove(&memory[pos], &memory[pos + needle_len], length - (pos + needle_len));
    micron::typeset<T>(&memory[length - needle_len], 0x0, needle_len);
    length -= needle_len;
    return *this;
  }

  template<size_type M, typename F, bool X = Sf>
  inline sstring &
  remove(const sstring<M, F, X> &needle)
  {
    if ( needle.empty() ) return *this;

    size_type pos = find_substr(needle.memory, needle.length);
    if ( pos == npos ) return *this;

    micron::bytemove(&memory[pos], &memory[pos + needle.length], length - (pos + needle.length));
    micron::typeset<T>(&memory[length - needle.length], 0x0, needle.length);
    length -= needle.length;
    return *this;
  }

  inline sstring &
  remove_all(const char *needle)
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring remove_all() null needle",
                                                                  static_cast<const void *>(needle));

    size_type needle_len = micron::strlen(needle);
    if ( needle_len == 0 ) return *this;

    size_type pos = 0;
    while ( (pos = find_substr(reinterpret_cast<const T *>(needle), needle_len, pos)) != npos ) {
      micron::bytemove(&memory[pos], &memory[pos + needle_len], length - (pos + needle_len));
      micron::typeset<T>(&memory[length - needle_len], 0x0, needle_len);
      length -= needle_len;
    }
    return *this;
  }

  template<size_type M>
  inline sstring &
  remove_all(const T (&needle)[M])
  {
    constexpr size_type needle_len = M - 1;
    if constexpr ( needle_len == 0 ) return *this;

    size_type pos = 0;
    while ( (pos = find_substr(&needle[0], needle_len, pos)) != npos ) {
      micron::bytemove(&memory[pos], &memory[pos + needle_len], length - (pos + needle_len));
      micron::typeset<T>(&memory[length - needle_len], 0x0, needle_len);
      length -= needle_len;
    }
    return *this;
  }

  template<size_type M, typename F, bool X = Sf>
  inline sstring &
  remove_all(const sstring<M, F, X> &needle)
  {
    if ( needle.empty() ) return *this;

    size_type pos = 0;
    while ( (pos = find_substr(needle.memory, needle.length, pos)) != npos ) {
      micron::bytemove(&memory[pos], &memory[pos + needle.length], length - (pos + needle.length));
      micron::typeset<T>(&memory[length - needle.length], 0x0, needle.length);
      length -= needle.length;
    }
    return *this;
  }

  template<typename I, typename F = T>
    requires micron::same_as<micron::remove_cvref_t<F>, T> && micron::convertible_to<I, size_type> && micron::integral<I>
             && (!micron::is_pointer_v<I>)
  inline sstring &
  insert(I ind, F ch, size_type cnt = 1)
  {
    if ( cnt == 0 ) [[unlikely]]
      return *this;

    __safety_check<&sstring::__insert_ind_check, except::library_error>("micron::sstring insert() out of range",
                                                                        static_cast<size_type>(ind), cnt);

    micron::bytemove(&memory[ind + cnt], &memory[ind], length - ind);
    micron::typeset<T>(&memory[ind], ch, cnt);
    length += cnt;
    return *this;
  }

  template<typename I, typename F = T, size_type M>
    requires micron::convertible_to<I, size_type> && micron::integral<I> && (!micron::is_pointer_v<I>)
  inline sstring &
  insert(I ind, const F (&str)[M], size_type cnt = 1)
  {
    if ( cnt == 0 ) [[unlikely]]
      return *this;

    size_type str_len = M - 1;

    __safety_check<&sstring::__insert_ind_check, except::library_error>("micron::sstring insert() out of range",
                                                                        static_cast<size_type>(ind), cnt * str_len);

    micron::bytemove(&memory[ind + cnt * str_len], &memory[ind], length - ind);
    for ( size_type i = 0; i < cnt; ++i ) micron::memcpy(&memory[ind + i * str_len], str, str_len);

    length += (cnt * str_len);
    return *this;
  }

  template<typename F = T>
    requires true
  inline sstring &
  insert(iterator itr, F ch, size_type cnt = 1)
  {
    if ( cnt == 0 ) [[unlikely]]
      return *this;

    __safety_check<&sstring::__insert_iter_check, except::library_error>("micron::sstring insert() out of range",
                                                                         static_cast<const T *>(itr), cnt);

    micron::bytemove(itr + cnt, itr, length - static_cast<size_type>(itr - &memory[0]));
    micron::typeset<T>(itr, ch, cnt);
    length += cnt;
    return *this;
  }

  template<typename F = T, size_type M>
  inline sstring &
  insert(iterator itr, const F (&str)[M], size_type cnt = 1)
  {
    size_type str_len = M - 1;

    __safety_check<&sstring::__insert_iter_check, except::library_error>("micron::sstring insert() out of range",
                                                                         static_cast<const T *>(itr), cnt * str_len);

    micron::bytemove(itr + cnt * str_len, itr, (memory + length) - itr);
    for ( size_type i = 0; i < cnt; ++i ) micron::memcpy(itr + i * str_len, str, str_len);
    length += (cnt * str_len);
    return *this;
  }

  template<typename F, size_type M, bool X = Sf>
  inline sstring &
  insert(iterator itr, const sstring<M, F, X> &o)
  {
    __safety_check<&sstring::__insert_iter_check, except::library_error>("micron::sstring insert() out of range",
                                                                         static_cast<const T *>(itr), o.length);

    micron::bytemove(itr + (o.length), itr, length - static_cast<size_type>(itr - &memory[0]));
    micron::memcpy(itr, &o.memory[0], o.length);
    length += o.length;
    return *this;
  }

  template<typename F, size_type M, bool X = Sf>
  inline sstring &
  insert(iterator itr, sstring<M, F, X> &&o)
  {
    __safety_check<&sstring::__insert_iter_check, except::library_error>("micron::sstring insert() out of range",
                                                                         static_cast<const T *>(itr), o.length);

    micron::bytemove(itr + (o.length), itr, length - static_cast<size_type>(itr - &memory[0]));
    micron::memcpy(itr, &o.memory[0], o.length);
    length += o.length;
    micron::constexpr_zero(o.memory, o.length);
    o.length = 0;
    return *this;
  }

  inline T &
  at(const size_type n)
  {
    __safety_check<&sstring::__index_check, except::library_error>("micron::sstring at() out of range", n);
    return memory[n];
  };

  inline T &
  at(const size_type n) const
  {
    __safety_check<&sstring::__index_check, except::library_error>("micron::sstring at() out of range", n);
    return memory[n];
  };

  template<typename Itr>
    requires(micron::same_as<Itr, const_iterator>)
  inline size_type
  at(Itr n)
  {
    __safety_check<&sstring::__at_iter_check, except::library_error>("micron::sstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &memory[0];
  };

  template<typename Itr>
    requires(micron::same_as<Itr, const_iterator>)
  inline size_type
  at(Itr n) const
  {
    __safety_check<&sstring::__at_iter_check, except::library_error>("micron::sstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &memory[0];
  };

  template<typename Itr>
    requires(micron::same_as<Itr, iterator>)
  inline size_type
  at(Itr n)
  {
    __safety_check<&sstring::__at_iter_check, except::library_error>("micron::sstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &memory[0];
  };

  template<typename Itr>
    requires(micron::same_as<Itr, iterator>)
  inline size_type
  at(Itr n) const
  {
    __safety_check<&sstring::__at_iter_check, except::library_error>("micron::sstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &memory[0];
  };

  inline T &
  operator[](const size_type n)
  {
    return memory[n];
  };

  inline const T &
  operator[](const size_type n) const
  {
    return memory[n];
  };

  inline sstring &
  operator+=(const sstring &data)
  {
    if ( data.empty() ) return *this;

    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring operator+=() out of memory", data.length);

    micron::memcpy(&memory[length], &data.memory[0], data.length);
    length += data.length;
    return *this;
  };

  template<typename F = T>
  inline sstring &
  operator+=(const slice<F> &data)
  {
    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring operator+=() out of memory", data.length);

    micron::memcpy(&(memory)[length], data.begin(), data.size() - 1);
    length += data.size() - 1;
    return *this;
  };

  inline sstring &
  append(const sstring &o)
  {
    if ( o.empty() ) return *this;

    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring append() out of memory", o.length);

    micron::memcpy(&memory[length], &o.memory[0], o.length);
    length += o.length;
    return *this;
  }

  template<typename F>
  inline sstring &
  append(const slice<F> &f, usize n)
  {
    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring append() out of memory", n);

    micron::memcpy(&(memory)[length], &f[0], n);
    length += n;
    return *this;
  }

  template<typename F = T, size_type M>
  inline sstring &
  append_null(const F (&str)[M])
  {
    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring append_null() out of memory", M);

    micron::memcpy(&memory[length], &str[0], M - 1);
    length += M - 1;
    return *this;
  }

  inline void
  null_term(void)
  {
    memory[length] = 0x0;
  }

  inline sstring &
  operator+=(char ch)
  {
    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring operator+=() out of memory", static_cast<size_type>(1));

    memory[length++] = ch;
    return *this;
  };

  inline sstring &
  operator+=(const buffer &data)
  {
    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring operator+=() out of memory", data.size());

    micron::memcpy(&memory[length], &data, data.size());
    length += data.size();
    return *this;
  };

  template<size_type M>
  inline sstring &
  operator+=(const char (&data)[M])
  {
    if ( M == 1 and data[0] == 0x0 ) return *this;

    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring operator+=() out of memory", M);

    micron::memcpy(&memory[length], &data[0], M);
    length += M - 1;
    return *this;
  };

  template<typename F = T>
  inline sstring &
  operator+=(const F *&data)
  {
    auto sz = strlen(data) + 1;

    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring operator+=() out of memory", static_cast<size_type>(sz));

    micron::memcpy(&memory[length], &data[0], sz);
    length += sz - 1;
    return *this;
  };

  template<size_type M = N, typename F = T, bool X = Sf>
  inline sstring<M, F>
  substr(usize pos = 0, usize cnt = npos) const
  {
    if ( cnt == npos ) cnt = (pos < length) ? (length - pos) : 0;

    __safety_check<&sstring::__range_pos_cnt, except::library_error>("micron::sstring substr() invalid range", static_cast<size_type>(pos),
                                                                     static_cast<size_type>(cnt));

    if ( cnt >= M ) exc<except::library_error>("micron::sstring substr() result too large for target.");

    sstring<M, F, X> buf;
    micron::memcpy(&buf.data()[0], &memory[pos], cnt);
    buf.set_size(cnt);
    return buf;
  };

  template<size_type M = N, typename F = T, bool X = Sf>
  inline sstring<M, F>
  substr(const_iterator _start, const_iterator _end = nullptr) const
  {
    if ( _end == nullptr ) _end = cend();

    __safety_check<&sstring::__iter_substr_check, except::library_error>("micron::sstring substr() invalid range", _start, _end);

    size_type len = static_cast<size_type>(_end - _start);

    if ( len >= M ) exc<except::library_error>("micron::sstring substr() result too large for target.");

    sstring<M, F, X> buf;
    micron::memcpy(&buf.data()[0], _start, len);
    buf.set_size(len);
    return buf;
  };

  template<typename I>
    requires micron::convertible_to<I, size_type> && micron::integral<I> && (!micron::is_pointer_v<I>)
  inline sstring &
  truncate(I n)
  {
    if ( static_cast<size_type>(n) >= length ) return *this;
    if ( static_cast<size_type>(n) > N ) n = N;
    micron::typeset<T>(&memory[static_cast<size_type>(n)], 0x0, length - static_cast<size_type>(n));
    length = n;
    return *this;
  }

  inline sstring &
  truncate(iterator itr)
  {
    __safety_check<&sstring::__iterator_check, except::library_error>("micron::sstring truncate() iterator out of range", itr);

    if ( itr >= end() ) return *this;

    size_type n = static_cast<size_type>(itr - begin());
    micron::typeset<T>(&memory[n], 0x0, length - n);
    length = n;
    return *this;
  }

  inline sstring &
  truncate(const_iterator itr)
  {
    __safety_check<&sstring::__iterator_check, except::library_error>("micron::sstring truncate() iterator out of range", itr);

    if ( itr >= cend() ) return *this;

    size_type n = static_cast<size_type>(itr - cbegin());
    micron::typeset<T>(&memory[n], 0x0, length - n);
    length = n;
    return *this;
  }

  explicit inline
  operator bool() const noexcept
  {
    return !empty();
  }

  explicit constexpr
  operator const char *() const
  {
    return c_str();
  }

  template<is_string S>
  inline bool
  operator==(const S &str) const
  {
    return __lexcmp(memory, length, str.c_str(), str.size()) == 0;
  }

  template<is_string S>
  inline bool
  operator!=(const S &str) const
  {
    return __lexcmp(memory, length, str.c_str(), str.size()) != 0;
  }

  inline bool
  operator==(const T *data) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring operator==() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(memory, length, data, micron::strlen(data)) == 0;
  }

  inline bool
  operator!=(const T *data) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring operator!=() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(memory, length, data, micron::strlen(data)) != 0;
  }

  inline bool
  operator<(const T *data) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring operator<() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(memory, length, data, micron::strlen(data)) < 0;
  }

  inline bool
  operator>(const T *data) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring operator>() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(memory, length, data, micron::strlen(data)) > 0;
  }

  inline bool
  operator<=(const T *data) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring operator<=() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(memory, length, data, micron::strlen(data)) <= 0;
  }

  inline bool
  operator>=(const T *data) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring operator>=() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(memory, length, data, micron::strlen(data)) >= 0;
  }

  template<typename F = T, size_type M>
  inline bool
  operator==(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) == 0;
  }

  template<typename F = T, size_type M>
  inline bool
  operator!=(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) != 0;
  }

  template<typename F = T, size_type M>
  inline bool
  operator<(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) < 0;
  }

  template<typename F = T, size_type M>
  inline bool
  operator>(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) > 0;
  }

  template<typename F = T, size_type M>
  inline bool
  operator<=(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) <= 0;
  }

  template<typename F = T, size_type M>
  inline bool
  operator>=(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) >= 0;
  }

  template<size_type M, typename F = T, bool X = Sf>
  inline bool
  operator==(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) == 0;
  }

  template<size_type M, typename F = T, bool X = Sf>
  inline bool
  operator!=(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) != 0;
  }

  template<size_type M, typename F = T, bool X = Sf>
  inline bool
  operator<(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) < 0;
  }

  template<size_type M, typename F = T, bool X = Sf>
  inline bool
  operator>(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) > 0;
  }

  template<size_type M, typename F = T, bool X = Sf>
  inline bool
  operator<=(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) <= 0;
  }

  template<size_type M, typename F = T, bool X = Sf>
  inline bool
  operator>=(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) >= 0;
  }
};

};      // namespace micron
