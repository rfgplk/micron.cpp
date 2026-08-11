//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../compare.hpp"
#include "../except.hpp"
#include "../memory/cmemory/memcmp.hpp"
#include "../memory/cstring.hpp"
#include "../tags.hpp"
#include "../types.hpp"

// fixed_string<N>
//
// a small (literal-class) compile-time string usable as a non-type template parameter
namespace micron
{

constexpr inline bool
__fs_is_upper(char c) noexcept
{
  return c >= 'A' && c <= 'Z';
}

constexpr inline bool
__fs_is_lower(char c) noexcept
{
  return c >= 'a' && c <= 'z';
}

constexpr inline bool
__fs_is_space(char c) noexcept
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

constexpr inline char
__fs_to_lower(char c) noexcept
{
  return __fs_is_upper(c) ? static_cast<char>(c - 'A' + 'a') : c;
}

constexpr inline char
__fs_to_upper(char c) noexcept
{
  return __fs_is_lower(c) ? static_cast<char>(c - 'a' + 'A') : c;
}

template<typename T>
constexpr inline int
__fs_lexcmp(const T *a, usize alen, const T *b, usize blen) noexcept
{

  if !consteval {
    if ( a == b ) return alen < blen ? -1 : (alen > blen ? 1 : 0);
  }
  const usize common = alen < blen ? alen : blen;
  if ( common ) {
    const i64 d = micron::memcmp<T, T>(a, b, common);
    if ( d != 0 ) return d < 0 ? -1 : 1;
  }
  if ( alen < blen ) return -1;
  if ( alen > blen ) return 1;
  return 0;
}

constexpr inline std::strong_ordering
__fs_ord(int c) noexcept
{
  return c < 0 ? std::strong_ordering::less : (c > 0 ? std::strong_ordering::greater : std::strong_ordering::equal);
}

template<usize N> struct fixed_string {
  using category_type = string_tag;
  using contiguous_tag = void;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef char value_type;
  typedef char *iterator;
  typedef const char *const_iterator;
  typedef char *pointer;
  typedef const char *const_pointer;
  typedef usize size_type;
  typedef char &reference;
  typedef const char &const_reference;

  static constexpr size_type npos = ~static_cast<size_type>(0);
  static constexpr size_type buffer_size = N;

  char buf[N]{};

  constexpr fixed_string() noexcept = default;

  constexpr fixed_string(const char (&s)[N]) noexcept
  {
    for ( usize i = 0; i < N; ++i ) buf[i] = s[i];
  }

  constexpr fixed_string(const char *s, usize n) noexcept
  {
    const usize lim = n < (N ? N - 1 : 0) ? n : (N ? N - 1 : 0);
    for ( usize i = 0; i < lim; ++i ) buf[i] = s[i];
  }

  constexpr size_type
  size() const noexcept
  {
    return N ? N - 1 : 0;
  }

  constexpr size_type
  len() const noexcept
  {
    return micron::strnlen(buf, N);
  }

  constexpr bool
  empty() const noexcept
  {
    return len() == 0;
  }

  constexpr size_type
  capacity() const noexcept
  {
    return N ? N - 1 : 0;
  }

  constexpr size_type
  max_size() const noexcept
  {
    return N ? N - 1 : 0;
  }

  constexpr char
  operator[](size_type i) const noexcept
  {
    return buf[i];
  }

  constexpr reference
  operator[](size_type i) noexcept
  {
    return buf[i];
  }

  constexpr const_reference
  at(size_type i) const
  {
    if ( i >= len() ) exc<except::library_error>("micron::fixed_string at() out of range");
    return buf[i];
  }

  constexpr reference
  at(size_type i)
  {
    if ( i >= len() ) exc<except::library_error>("micron::fixed_string at() out of range");
    return buf[i];
  }

  constexpr const_pointer
  data() const noexcept
  {
    return buf;
  }

  constexpr pointer
  data() noexcept
  {
    return buf;
  }

  constexpr const_pointer
  cdata() const noexcept
  {
    return buf;
  }

  constexpr const char *
  c_str() const noexcept
  {
    return buf;
  }

  constexpr const_reference
  front() const noexcept
  {
    return buf[0];
  }

  constexpr const_reference
  back() const noexcept
  {
    return buf[len() ? len() - 1 : 0];
  }

  constexpr const_reference
  last() const noexcept
  {
    return back();
  }

  constexpr const_iterator
  begin() const noexcept
  {
    return buf;
  }

  constexpr const_iterator
  end() const noexcept
  {
    return buf + len();
  }

  constexpr iterator
  begin() noexcept
  {
    return buf;
  }

  constexpr iterator
  end() noexcept
  {
    return buf + len();
  }

  constexpr const_iterator
  cbegin() const noexcept
  {
    return buf;
  }

  constexpr const_iterator
  cend() const noexcept
  {
    return buf + len();
  }

  constexpr int
  compare(const char *p, size_type n) const noexcept
  {
    return __fs_lexcmp(buf, len(), p, n);
  }

  constexpr int
  compare(const char *s) const noexcept
  {
    return __fs_lexcmp(buf, len(), s, micron::strlen(s));
  }

  template<usize M>
  constexpr int
  compare(const fixed_string<M> &o) const noexcept
  {
    return __fs_lexcmp(buf, len(), o.buf, o.len());
  }

  template<is_string S>
  constexpr int
  compare(const S &s) const
  {
    return __fs_lexcmp(buf, len(), s.c_str(), s.size());
  }

  template<usize M>
  constexpr std::strong_ordering
  operator<=>(const fixed_string<M> &o) const noexcept
  {
    return __fs_ord(compare(o));
  }

  template<usize M>
  constexpr bool
  operator==(const fixed_string<M> &o) const noexcept
  {
    return compare(o) == 0;
  }

  constexpr std::strong_ordering
  operator<=>(const char *s) const noexcept
  {
    return __fs_ord(compare(s));
  }

  constexpr bool
  operator==(const char *s) const noexcept
  {
    return compare(s) == 0;
  }

  template<usize M>
  constexpr std::strong_ordering
  operator<=>(const char (&s)[M]) const noexcept
  {
    return __fs_ord(__fs_lexcmp(buf, len(), s, micron::strnlen(s, M)));
  }

  template<usize M>
  constexpr bool
  operator==(const char (&s)[M]) const noexcept
  {
    return __fs_lexcmp(buf, len(), s, micron::strnlen(s, M)) == 0;
  }

  template<is_string S>
  constexpr std::strong_ordering
  operator<=>(const S &s) const
  {
    return __fs_ord(compare(s));
  }

  template<is_string S>
  constexpr bool
  operator==(const S &s) const
  {
    return compare(s) == 0;
  }

  constexpr size_type
  find(char c, size_type pos = 0) const noexcept
  {
    const size_type n = len();
    for ( size_type i = pos; i < n; ++i )
      if ( buf[i] == c ) return i;
    return npos;
  }

  constexpr size_type
  find_substr(const char *needle, size_type nlen, size_type pos = 0) const noexcept
  {
    const size_type n = len();
    if ( nlen == 0 ) return pos <= n ? pos : npos;
    if ( nlen > n ) return npos;
    for ( size_type i = pos; i + nlen <= n; ++i ) {
      size_type j = 0;
      while ( j < nlen && buf[i + j] == needle[j] ) ++j;
      if ( j == nlen ) return i;
    }
    return npos;
  }

  constexpr size_type
  find(const char *needle, size_type pos = 0) const noexcept
  {
    return find_substr(needle, micron::strlen(needle), pos);
  }

  template<usize M>
  constexpr size_type
  find(const fixed_string<M> &needle, size_type pos = 0) const noexcept
  {
    return find_substr(needle.buf, needle.len(), pos);
  }

  constexpr size_type
  rfind(char c, size_type pos = npos) const noexcept
  {
    const size_type n = len();
    if ( n == 0 ) return npos;
    size_type i = (pos >= n ? n - 1 : pos);
    for ( ;; ) {
      if ( buf[i] == c ) return i;
      if ( i == 0 ) break;
      --i;
    }
    return npos;
  }

  constexpr size_type
  rfind_substr(const char *needle, size_type nlen, size_type pos = npos) const noexcept
  {
    const size_type n = len();
    if ( nlen == 0 ) return n < pos ? n : pos;
    if ( nlen > n ) return npos;
    const size_type last = n - nlen;
    size_type i = (pos > last ? last : pos);
    for ( ;; ) {
      size_type j = 0;
      while ( j < nlen && buf[i + j] == needle[j] ) ++j;
      if ( j == nlen ) return i;
      if ( i == 0 ) break;
      --i;
    }
    return npos;
  }

  constexpr size_type
  rfind(const char *needle, size_type pos = npos) const noexcept
  {
    return rfind_substr(needle, micron::strlen(needle), pos);
  }

  constexpr size_type
  find_first_of_n(const char *set, size_type slen, size_type pos = 0) const noexcept
  {
    const size_type n = len();
    for ( size_type i = pos; i < n; ++i )
      for ( size_type k = 0; k < slen; ++k )
        if ( buf[i] == set[k] ) return i;
    return npos;
  }

  constexpr size_type
  find_first_of(const char *set, size_type pos = 0) const noexcept
  {
    return find_first_of_n(set, micron::strlen(set), pos);
  }

  constexpr size_type
  find_last_of_n(const char *set, size_type slen, size_type pos = npos) const noexcept
  {
    const size_type n = len();
    if ( n == 0 ) return npos;
    size_type i = (pos >= n ? n - 1 : pos);
    for ( ;; ) {
      for ( size_type k = 0; k < slen; ++k )
        if ( buf[i] == set[k] ) return i;
      if ( i == 0 ) break;
      --i;
    }
    return npos;
  }

  constexpr size_type
  find_last_of(const char *set, size_type pos = npos) const noexcept
  {
    return find_last_of_n(set, micron::strlen(set), pos);
  }

  constexpr size_type
  find_first_not_of_n(const char *set, size_type slen, size_type pos = 0) const noexcept
  {
    const size_type n = len();
    for ( size_type i = pos; i < n; ++i ) {
      bool hit = false;
      for ( size_type k = 0; k < slen; ++k )
        if ( buf[i] == set[k] ) {
          hit = true;
          break;
        }
      if ( !hit ) return i;
    }
    return npos;
  }

  constexpr size_type
  find_first_not_of(const char *set, size_type pos = 0) const noexcept
  {
    return find_first_not_of_n(set, micron::strlen(set), pos);
  }

  constexpr size_type
  find_last_not_of_n(const char *set, size_type slen, size_type pos = npos) const noexcept
  {
    const size_type n = len();
    if ( n == 0 ) return npos;
    size_type i = (pos >= n ? n - 1 : pos);
    for ( ;; ) {
      bool hit = false;
      for ( size_type k = 0; k < slen; ++k )
        if ( buf[i] == set[k] ) {
          hit = true;
          break;
        }
      if ( !hit ) return i;
      if ( i == 0 ) break;
      --i;
    }
    return npos;
  }

  constexpr size_type
  find_last_not_of(const char *set, size_type pos = npos) const noexcept
  {
    return find_last_not_of_n(set, micron::strlen(set), pos);
  }

  constexpr bool
  contains(char c) const noexcept
  {
    return find(c) != npos;
  }

  constexpr bool
  contains(const char *needle) const noexcept
  {
    return find(needle) != npos;
  }

  template<usize M>
  constexpr bool
  contains(const fixed_string<M> &needle) const noexcept
  {
    return find(needle) != npos;
  }

  constexpr bool
  starts_with(char c) const noexcept
  {
    return len() != 0 && buf[0] == c;
  }

  constexpr bool
  starts_with(const char *p, size_type n) const noexcept
  {
    if ( n > len() ) return false;
    for ( size_type i = 0; i < n; ++i )
      if ( buf[i] != p[i] ) return false;
    return true;
  }

  constexpr bool
  starts_with(const char *s) const noexcept
  {
    return starts_with(s, micron::strlen(s));
  }

  template<usize M>
  constexpr bool
  starts_with(const fixed_string<M> &o) const noexcept
  {
    return starts_with(o.buf, o.len());
  }

  constexpr bool
  ends_with(char c) const noexcept
  {
    return len() != 0 && buf[len() - 1] == c;
  }

  constexpr bool
  ends_with(const char *p, size_type n) const noexcept
  {
    const size_type m = len();
    if ( n > m ) return false;
    for ( size_type i = 0; i < n; ++i )
      if ( buf[m - n + i] != p[i] ) return false;
    return true;
  }

  constexpr bool
  ends_with(const char *s) const noexcept
  {
    return ends_with(s, micron::strlen(s));
  }

  template<usize M>
  constexpr bool
  ends_with(const fixed_string<M> &o) const noexcept
  {
    return ends_with(o.buf, o.len());
  }

  constexpr size_type
  count(char c) const noexcept
  {
    const size_type n = len();
    size_type k = 0;
    for ( size_type i = 0; i < n; ++i )
      if ( buf[i] == c ) ++k;
    return k;
  }

  constexpr size_type
  count(const char *needle, size_type nlen) const noexcept
  {
    if ( nlen == 0 ) return 0;
    size_type k = 0, at = 0;
    while ( (at = find_substr(needle, nlen, at)) != npos ) {
      ++k;
      at += nlen;
    }
    return k;
  }

  constexpr size_type
  count(const char *needle) const noexcept
  {
    return count(needle, micron::strlen(needle));
  }

  template<size_type Pos, size_type Cnt>
  constexpr fixed_string<Cnt + 1>
  substr() const noexcept
  {
    static_assert(Pos + Cnt < N || (Pos == 0 && Cnt == 0), "micron::fixed_string substr() out of buffer");
    fixed_string<Cnt + 1> out;
    const size_type n = len();
    for ( size_type i = 0; i < Cnt && Pos + i < n; ++i ) out.buf[i] = buf[Pos + i];
    return out;
  }

  constexpr fixed_string<N>
  to_lower() const noexcept
  {
    fixed_string<N> out;
    const size_type n = len();
    for ( size_type i = 0; i < n; ++i ) out.buf[i] = __fs_to_lower(buf[i]);
    return out;
  }

  constexpr fixed_string<N>
  to_upper() const noexcept
  {
    fixed_string<N> out;
    const size_type n = len();
    for ( size_type i = 0; i < n; ++i ) out.buf[i] = __fs_to_upper(buf[i]);
    return out;
  }

  constexpr fixed_string<N>
  trim_left() const noexcept
  {
    fixed_string<N> out;
    const size_type n = len();
    size_type a = 0;
    while ( a < n && __fs_is_space(buf[a]) ) ++a;
    for ( size_type i = a; i < n; ++i ) out.buf[i - a] = buf[i];
    return out;
  }

  constexpr fixed_string<N>
  trim_right() const noexcept
  {
    fixed_string<N> out;
    size_type b = len();
    while ( b > 0 && __fs_is_space(buf[b - 1]) ) --b;
    for ( size_type i = 0; i < b; ++i ) out.buf[i] = buf[i];
    return out;
  }

  constexpr fixed_string<N>
  trim() const noexcept
  {
    fixed_string<N> out;
    const size_type n = len();
    size_type a = 0, b = n;
    while ( a < b && __fs_is_space(buf[a]) ) ++a;
    while ( b > a && __fs_is_space(buf[b - 1]) ) --b;
    for ( size_type i = a; i < b; ++i ) out.buf[i - a] = buf[i];
    return out;
  }

  constexpr fixed_string<N>
  reverse() const noexcept
  {
    fixed_string<N> out;
    const size_type n = len();
    for ( size_type i = 0; i < n; ++i ) out.buf[i] = buf[n - 1 - i];
    return out;
  }
};

template<usize N> fixed_string(const char (&)[N]) -> fixed_string<N>;

template<usize A, usize B>
constexpr fixed_string<A + B - 1>
operator+(const fixed_string<A> &a, const fixed_string<B> &b) noexcept
{
  fixed_string<A + B - 1> out;
  const usize an = a.len(), bn = b.len();
  for ( usize i = 0; i < an; ++i ) out.buf[i] = a.buf[i];
  for ( usize i = 0; i < bn; ++i ) out.buf[an + i] = b.buf[i];
  return out;
}

};      // namespace micron
