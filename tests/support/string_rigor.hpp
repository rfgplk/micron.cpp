//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Shared oracle + generators for the exhaustive, adversarial string rigor
// suites (rigor_sstring / rigor_string / rigor_istring / rigor_rope /
// rigor_unistring). Modeled on tests/support/algo_rigor.hpp.
//
// The oracle `ref_string<T>` is a flat, obviously-correct (linear, no SIMD, no
// allocator games) reference against which micron's string classes are diffed
// per operation. STL is forbidden (CLAUDE.md) so everything is hand-rolled on
// top of micron::types only.
//
// Conventions:
//   * NPOS == ~usize(0) == micron::npos.
//   * "seq" arguments are (const T* p, usize n) raw spans.
//   * generators draw from mtest::prng (oracles.hpp); seed for reproducibility.

#include "../../src/types.hpp"

#include "oracles.hpp"

namespace mtest
{

inline constexpr usize REF_NPOS = ~static_cast<usize>(0);

enum class alpha : int {
  full,
  ascii,
  small,
  ws_mix,
  case_mix,
  digits,
};

inline bool
ref_is_ws(unsigned long c) noexcept
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

template<typename T>
inline T
gen_char(prng &rng, alpha a) noexcept
{
  const u64 r = rng.next();
  switch ( a ) {
  case alpha::ascii:
    return static_cast<T>(0x20 + (r % (0x7eu - 0x20u + 1u)));
  case alpha::small:
    return static_cast<T>('a' + (r % 3u));
  case alpha::ws_mix: {
    static const char set[] = { ' ', '\t', '\n', 'a', 'b', ' ', '\r', 'c' };
    return static_cast<T>(set[r % (sizeof(set))]);
  }
  case alpha::case_mix: {
    const u64 k = r % 52u;
    return static_cast<T>(k < 26u ? ('A' + k) : ('a' + (k - 26u)));
  }
  case alpha::digits:
    return static_cast<T>('0' + (r % 10u));
  case alpha::full:
  default:

    if constexpr ( sizeof(T) == 1 ) {
      return static_cast<T>(1u + (r % 255u));
    } else {

      return static_cast<T>(1u + (r % 0x10FFFEu));
    }
  }
}

template<typename T>
inline void
gen_string(prng &rng, T *out, usize &len, usize maxlen, alpha a = alpha::ascii) noexcept
{
  len = (maxlen == 0) ? 0 : static_cast<usize>(rng.next() % (maxlen + 1));
  for ( usize i = 0; i < len; ++i ) out[i] = gen_char<T>(rng, a);
}

template<typename T>
inline void
gen_string_n(prng &rng, T *out, usize len, alpha a = alpha::ascii) noexcept
{
  for ( usize i = 0; i < len; ++i ) out[i] = gen_char<T>(rng, a);
}

template<typename T>
inline void
gen_string_with_nuls(prng &rng, T *out, usize len, alpha a = alpha::ascii) noexcept
{
  for ( usize i = 0; i < len; ++i ) {
    if ( (rng.next() & 7u) == 0u )
      out[i] = static_cast<T>(0);
    else
      out[i] = gen_char<T>(rng, a);
  }
}

inline constexpr usize kStrLens[] = { 0, 1, 2, 3, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 257, 511, 512, 1023 };
inline constexpr usize kStrLensCount = sizeof(kStrLens) / sizeof(kStrLens[0]);

template<typename T, usize Cap = 8192> struct ref_string {
  T buf[Cap];
  usize len = 0;

  ref_string() noexcept : len(0) { }

  void
  clear(void) noexcept
  {
    len = 0;
  }

  usize
  size(void) const noexcept
  {
    return len;
  }

  bool
  empty(void) const noexcept
  {
    return len == 0;
  }

  static constexpr usize
  capacity(void) noexcept
  {
    return Cap;
  }

  T &
  operator[](usize i) noexcept
  {
    return buf[i];
  }

  const T &
  operator[](usize i) const noexcept
  {
    return buf[i];
  }

  usize
  to_buf(T *out) const noexcept
  {
    for ( usize i = 0; i < len; ++i ) out[i] = buf[i];
    return len;
  }

  void
  assign(const T *p, usize n) noexcept
  {
    len = (n <= Cap) ? n : Cap;
    for ( usize i = 0; i < len; ++i ) buf[i] = p[i];
  }

  void
  push_back(T ch) noexcept
  {
    if ( len < Cap ) buf[len++] = ch;
  }

  void
  pop_back(void) noexcept
  {
    if ( len ) --len;
  }

  void
  append(const T *p, usize n) noexcept
  {
    for ( usize i = 0; i < n && len < Cap; ++i ) buf[len++] = p[i];
  }

  void
  append_char(T ch, usize cnt) noexcept
  {
    for ( usize i = 0; i < cnt && len < Cap; ++i ) buf[len++] = ch;
  }

  void
  insert_char(usize ind, T ch, usize cnt) noexcept
  {
    if ( ind > len ) ind = len;
    if ( len + cnt > Cap ) cnt = Cap - len;
    for ( usize i = len; i > ind; --i ) buf[i + cnt - 1] = buf[i - 1];
    for ( usize i = 0; i < cnt; ++i ) buf[ind + i] = ch;
    len += cnt;
  }

  void
  insert_seq(usize ind, const T *p, usize n) noexcept
  {
    if ( ind > len ) ind = len;
    if ( len + n > Cap ) n = Cap - len;
    for ( usize i = len; i > ind; --i ) buf[i + n - 1] = buf[i - 1];
    for ( usize i = 0; i < n; ++i ) buf[ind + i] = p[i];
    len += n;
  }

  void
  erase(usize ind, usize cnt) noexcept
  {
    if ( ind >= len ) return;
    if ( cnt > len - ind ) cnt = len - ind;
    for ( usize i = ind; i + cnt < len; ++i ) buf[i] = buf[i + cnt];
    len -= cnt;
  }

  void
  replace(usize pos, usize cnt, const T *p, usize n) noexcept
  {
    erase(pos, cnt);
    insert_seq(pos, p, n);
  }

  void
  fill(T ch, usize cnt) noexcept
  {
    if ( cnt > Cap ) cnt = Cap;
    for ( usize i = 0; i < cnt; ++i ) buf[i] = ch;
    len = cnt;
  }

  void
  repeat(usize n) noexcept
  {
    const usize base = len;
    if ( base == 0 || n <= 1 ) {
      if ( n == 0 ) len = 0;
      return;
    }
    for ( usize k = 1; k < n; ++k )
      for ( usize i = 0; i < base && len < Cap; ++i ) buf[len++] = buf[i];
  }

  void
  reverse(void) noexcept
  {
    for ( usize i = 0, j = (len ? len - 1 : 0); i < j; ++i, --j ) {
      T t = buf[i];
      buf[i] = buf[j];
      buf[j] = t;
    }
  }

  void
  to_lower(void) noexcept
  {
    for ( usize i = 0; i < len; ++i ) {
      auto c = static_cast<unsigned long>(buf[i]);
      if ( c >= 'A' && c <= 'Z' ) buf[i] = static_cast<T>(c + 32);
    }
  }

  void
  to_upper(void) noexcept
  {
    for ( usize i = 0; i < len; ++i ) {
      auto c = static_cast<unsigned long>(buf[i]);
      if ( c >= 'a' && c <= 'z' ) buf[i] = static_cast<T>(c - 32);
    }
  }

  void
  trim_left(void) noexcept
  {
    usize i = 0;
    while ( i < len && ref_is_ws(static_cast<unsigned long>(buf[i])) ) ++i;
    if ( i == 0 ) return;
    for ( usize k = i; k < len; ++k ) buf[k - i] = buf[k];
    len -= i;
  }

  void
  trim_right(void) noexcept
  {
    while ( len && ref_is_ws(static_cast<unsigned long>(buf[len - 1])) ) --len;
  }

  void
  trim(void) noexcept
  {
    trim_right();
    trim_left();
  }

  void
  remove(const T *p, usize n) noexcept
  {
    usize at = find_seq(p, n, 0);
    if ( at != REF_NPOS ) erase(at, n);
  }

  void
  remove_all(const T *p, usize n) noexcept
  {
    if ( n == 0 ) return;
    usize at = 0;
    while ( (at = find_seq(p, n, at)) != REF_NPOS ) erase(at, n);
  }

  void
  xor_key(const byte *kp, usize kn) noexcept
  {
    if ( kn == 0 ) return;
    byte *b = reinterpret_cast<byte *>(&buf[0]);
    const usize nb = len * sizeof(T);
    for ( usize i = 0; i < nb; ++i ) b[i] = static_cast<byte>(b[i] ^ kp[i % kn]);
  }

  void
  and_key(const byte *kp, usize kn) noexcept
  {
    if ( kn == 0 ) return;
    byte *b = reinterpret_cast<byte *>(&buf[0]);
    const usize nb = len * sizeof(T);
    for ( usize i = 0; i < nb; ++i ) b[i] = static_cast<byte>(b[i] & kp[i % kn]);
  }

  void
  or_key(const byte *kp, usize kn) noexcept
  {
    if ( kn == 0 ) return;
    byte *b = reinterpret_cast<byte *>(&buf[0]);
    const usize nb = len * sizeof(T);
    for ( usize i = 0; i < nb; ++i ) b[i] = static_cast<byte>(b[i] | kp[i % kn]);
  }

  void
  truncate(usize n) noexcept
  {
    if ( n < len ) len = n;
  }

  usize
  find(T ch, usize pos = 0) const noexcept
  {
    for ( usize i = pos; i < len; ++i )
      if ( buf[i] == ch ) return i;
    return REF_NPOS;
  }

  usize
  find_seq(const T *p, usize n, usize pos = 0) const noexcept
  {
    if ( n == 0 ) return (pos <= len) ? pos : REF_NPOS;
    if ( n > len ) return REF_NPOS;
    for ( usize i = pos; i + n <= len; ++i ) {
      usize j = 0;
      for ( ; j < n; ++j )
        if ( buf[i + j] != p[j] ) break;
      if ( j == n ) return i;
    }
    return REF_NPOS;
  }

  usize
  rfind(T ch, usize pos = REF_NPOS) const noexcept
  {
    if ( len == 0 ) return REF_NPOS;
    usize start = (pos >= len) ? len - 1 : pos;
    for ( usize i = start + 1; i-- > 0; )
      if ( buf[i] == ch ) return i;
    return REF_NPOS;
  }

  usize
  rfind_seq(const T *p, usize n, usize pos = REF_NPOS) const noexcept
  {
    if ( n == 0 ) return (len < pos) ? len : pos;
    if ( n > len ) return REF_NPOS;
    usize last = len - n;
    usize start = (pos > last) ? last : pos;
    for ( usize i = start + 1; i-- > 0; ) {
      usize j = 0;
      for ( ; j < n; ++j )
        if ( buf[i + j] != p[j] ) break;
      if ( j == n ) return i;
      if ( i == 0 ) break;
    }
    return REF_NPOS;
  }

  bool
  in_set(T ch, const T *set, usize n) const noexcept
  {
    for ( usize i = 0; i < n; ++i )
      if ( set[i] == ch ) return true;
    return false;
  }

  usize
  find_first_of(const T *set, usize n, usize pos = 0) const noexcept
  {
    for ( usize i = pos; i < len; ++i )
      if ( in_set(buf[i], set, n) ) return i;
    return REF_NPOS;
  }

  usize
  find_last_of(const T *set, usize n, usize pos = REF_NPOS) const noexcept
  {
    if ( len == 0 ) return REF_NPOS;
    usize start = (pos >= len) ? len - 1 : pos;
    for ( usize i = start + 1; i-- > 0; )
      if ( in_set(buf[i], set, n) ) return i;
    return REF_NPOS;
  }

  usize
  find_first_not_of(const T *set, usize n, usize pos = 0) const noexcept
  {
    for ( usize i = pos; i < len; ++i )
      if ( !in_set(buf[i], set, n) ) return i;
    return REF_NPOS;
  }

  usize
  find_last_not_of(const T *set, usize n, usize pos = REF_NPOS) const noexcept
  {
    if ( len == 0 ) return REF_NPOS;
    usize start = (pos >= len) ? len - 1 : pos;
    for ( usize i = start + 1; i-- > 0; )
      if ( !in_set(buf[i], set, n) ) return i;
    return REF_NPOS;
  }

  bool
  starts_with(const T *p, usize n) const noexcept
  {
    if ( n > len ) return false;
    for ( usize i = 0; i < n; ++i )
      if ( buf[i] != p[i] ) return false;
    return true;
  }

  bool
  ends_with(const T *p, usize n) const noexcept
  {
    if ( n > len ) return false;
    const usize off = len - n;
    for ( usize i = 0; i < n; ++i )
      if ( buf[off + i] != p[i] ) return false;
    return true;
  }

  bool
  contains(const T *p, usize n) const noexcept
  {
    return find_seq(p, n, 0) != REF_NPOS;
  }

  usize
  count_char(T ch) const noexcept
  {
    usize c = 0;
    for ( usize i = 0; i < len; ++i )
      if ( buf[i] == ch ) ++c;
    return c;
  }

  usize
  count_seq(const T *p, usize n) const noexcept
  {
    if ( n == 0 ) return 0;
    usize c = 0, at = 0;
    while ( (at = find_seq(p, n, at)) != REF_NPOS ) {
      ++c;
      at += n;
    }
    return c;
  }

  int
  compare(const T *p, usize n) const noexcept
  {
    const usize m = (len < n) ? len : n;
    for ( usize i = 0; i < m; ++i ) {
      auto a = static_cast<unsigned long long>(buf[i]);
      auto b = static_cast<unsigned long long>(p[i]);
      if ( a < b ) return -1;
      if ( a > b ) return 1;
    }
    if ( len < n ) return -1;
    if ( len > n ) return 1;
    return 0;
  }
};

template<typename MStr, typename T, usize Cap>
inline bool
seq_eq(const MStr &m, const ref_string<T, Cap> &r) noexcept
{
  if ( static_cast<usize>(m.size()) != r.len ) return false;
  for ( usize i = 0; i < r.len; ++i )
    if ( static_cast<T>(m[i]) != r.buf[i] ) return false;
  return true;
}

template<typename MStr, typename T, usize Cap>
inline bool
cstr_eq(const MStr &m, const ref_string<T, Cap> &r) noexcept
{
  if constexpr ( sizeof(T) != 1 ) {
    (void)m;
    (void)r;
    return true;
  } else {
    if ( static_cast<usize>(m.size()) != r.len ) return false;
    const char *c = m.c_str();
    for ( usize i = 0; i < r.len; ++i )
      if ( static_cast<unsigned char>(c[i]) != static_cast<unsigned char>(r.buf[i]) ) return false;
    return c[r.len] == '\0';
  }
}

};      // namespace mtest
