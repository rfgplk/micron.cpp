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
// #include "../slice_forward.hpp"
#include "../span.hpp"

#include "../memory_block.hpp"

#include "unitypes.hpp"

namespace micron
{
// string on the stack, inplace (sstring means stackstring)
template <usize N, is_scalar_literal T = schar, bool Sf = true> struct sstring {
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

  template <auto Fn, typename E, typename... Args>
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
    size_type common = alen < blen ? alen : blen;
    for ( size_type i = 0; i < common; ++i ) {
      auto ca = static_cast<unsigned char>(a[i]);
      auto cb = static_cast<unsigned char>(b[i]);
      if ( ca < cb ) return -1;
      if ( ca > cb ) return 1;
    }
    if ( alen < blen ) return -1;
    if ( alen > blen ) return 1;
    return 0;
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

  template <size_type M, typename F> constexpr sstring(const F (&str)[M])
  {
    static_assert(N >= M, "micron::sstring sstring(cconst) too large.");
    micron::memcpy(&memory[0], &str[0], M);
    length = M - 1;     // cut null
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

  template <size_type M, typename F, bool X = Sf> constexpr sstring(const sstring<M, F, X> &o)
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

  template <is_string S> constexpr sstring(const S &o)
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

  template <size_type M, typename F, bool X = Sf> constexpr sstring(sstring<M, F, X> &&o)
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

  template <typename F>
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

  template <size_type M, typename F, bool X = Sf>
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

  template <size_type M, typename F, bool X = Sf>
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

  template <typename F>
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
    return sstring<N, T>(*this);     // copy
  }

  template <typename F>
  inline F
  clone_to()
  {
    return F(*this);     // copy
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

  // most of these live within format, but bringing them here for ease of use
  // TODO: do the same for hstring
  inline size_type
  rfind(char ch, size_type pos = npos) const
  {
    if ( length == 0 ) return npos;
    size_type i = (pos == npos || pos >= length) ? length - 1 : pos;
    for ( ;; ) {
      if ( memory[i] == static_cast<T>(ch) ) return i;
      if ( i == 0 ) break;
      --i;
    }
    return npos;
  }

  inline size_type
  rfind(const T *needle, size_type pos = npos) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring rfind() null needle", static_cast<const void *>(needle));
    return __rfind_substr(needle, micron::strlen(needle), pos);
  }

  template <size_type M>
  inline size_type
  rfind(const T (&needle)[M], size_type pos = npos) const
  {
    constexpr size_type needle_len = M - 1;
    return __rfind_substr(&needle[0], needle_len, pos);
  }

  template <size_type M, typename F, bool X = Sf>
  inline size_type
  rfind(const sstring<M, F, X> &str, size_type pos = npos) const
  {
    return __rfind_substr(reinterpret_cast<const T *>(str.memory), str.length, pos);
  }

  inline size_type
  find_first_of(const T *chars, size_type pos = 0) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring find_first_of() null", static_cast<const void *>(chars));
    for ( size_type i = pos; i < length; ++i )
      for ( size_type j = 0; chars[j] != 0x0; ++j )
        if ( memory[i] == chars[j] ) return i;
    return npos;
  }

  template <size_type M>
  inline size_type
  find_first_of(const T (&chars)[M], size_type pos = 0) const
  {
    for ( size_type i = pos; i < length; ++i )
      for ( size_type j = 0; j < M - 1; ++j )
        if ( memory[i] == chars[j] ) return i;
    return npos;
  }

  template <size_type M, typename F, bool X = Sf>
  inline size_type
  find_first_of(const sstring<M, F, X> &chars, size_type pos = 0) const
  {
    for ( size_type i = pos; i < length; ++i )
      for ( size_type j = 0; j < chars.length; ++j )
        if ( memory[i] == static_cast<T>(chars.memory[j]) ) return i;
    return npos;
  }

  inline size_type
  find_last_of(const T *chars, size_type pos = npos) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring find_last_of() null", static_cast<const void *>(chars));
    if ( length == 0 ) return npos;
    size_type i = (pos == npos || pos >= length) ? length - 1 : pos;
    for ( ;; ) {
      for ( size_type j = 0; chars[j] != 0x0; ++j )
        if ( memory[i] == chars[j] ) return i;
      if ( i == 0 ) break;
      --i;
    }
    return npos;
  }

  template <size_type M>
  inline size_type
  find_last_of(const T (&chars)[M], size_type pos = npos) const
  {
    if ( length == 0 ) return npos;
    size_type i = (pos == npos || pos >= length) ? length - 1 : pos;
    for ( ;; ) {
      for ( size_type j = 0; j < M - 1; ++j )
        if ( memory[i] == chars[j] ) return i;
      if ( i == 0 ) break;
      --i;
    }
    return npos;
  }

  template <size_type M, typename F, bool X = Sf>
  inline size_type
  find_last_of(const sstring<M, F, X> &chars, size_type pos = npos) const
  {
    if ( length == 0 ) return npos;
    size_type i = (pos == npos || pos >= length) ? length - 1 : pos;
    for ( ;; ) {
      for ( size_type j = 0; j < chars.length; ++j )
        if ( memory[i] == static_cast<T>(chars.memory[j]) ) return i;
      if ( i == 0 ) break;
      --i;
    }
    return npos;
  }

  inline size_type
  find_first_not_of(const T *chars, size_type pos = 0) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring find_first_not_of() null",
                                                                  static_cast<const void *>(chars));
    for ( size_type i = pos; i < length; ++i ) {
      bool hit = false;
      for ( size_type j = 0; chars[j] != 0x0; ++j )
        if ( memory[i] == chars[j] ) {
          hit = true;
          break;
        }
      if ( !hit ) return i;
    }
    return npos;
  }

  template <size_type M>
  inline size_type
  find_first_not_of(const T (&chars)[M], size_type pos = 0) const
  {
    for ( size_type i = pos; i < length; ++i ) {
      bool hit = false;
      for ( size_type j = 0; j < M - 1; ++j )
        if ( memory[i] == chars[j] ) {
          hit = true;
          break;
        }
      if ( !hit ) return i;
    }
    return npos;
  }

  template <size_type M, typename F, bool X = Sf>
  inline size_type
  find_first_not_of(const sstring<M, F, X> &chars, size_type pos = 0) const
  {
    for ( size_type i = pos; i < length; ++i ) {
      bool hit = false;
      for ( size_type j = 0; j < chars.length; ++j )
        if ( memory[i] == static_cast<T>(chars.memory[j]) ) {
          hit = true;
          break;
        }
      if ( !hit ) return i;
    }
    return npos;
  }

  inline size_type
  find_last_not_of(const T *chars, size_type pos = npos) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring find_last_not_of() null",
                                                                  static_cast<const void *>(chars));
    if ( length == 0 ) return npos;
    size_type i = (pos == npos || pos >= length) ? length - 1 : pos;
    for ( ;; ) {
      bool hit = false;
      for ( size_type j = 0; chars[j] != 0x0; ++j )
        if ( memory[i] == chars[j] ) {
          hit = true;
          break;
        }
      if ( !hit ) return i;
      if ( i == 0 ) break;
      --i;
    }
    return npos;
  }

  template <size_type M>
  inline size_type
  find_last_not_of(const T (&chars)[M], size_type pos = npos) const
  {
    if ( length == 0 ) return npos;
    size_type i = (pos == npos || pos >= length) ? length - 1 : pos;
    for ( ;; ) {
      bool hit = false;
      for ( size_type j = 0; j < M - 1; ++j )
        if ( memory[i] == chars[j] ) {
          hit = true;
          break;
        }
      if ( !hit ) return i;
      if ( i == 0 ) break;
      --i;
    }
    return npos;
  }

  template <size_type M, typename F, bool X = Sf>
  inline size_type
  find_last_not_of(const sstring<M, F, X> &chars, size_type pos = npos) const
  {
    if ( length == 0 ) return npos;
    size_type i = (pos == npos || pos >= length) ? length - 1 : pos;
    for ( ;; ) {
      bool hit = false;
      for ( size_type j = 0; j < chars.length; ++j )
        if ( memory[i] == static_cast<T>(chars.memory[j]) ) {
          hit = true;
          break;
        }
      if ( !hit ) return i;
      if ( i == 0 ) break;
      --i;
    }
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
    size_type prefix_len = micron::strlen(prefix);
    if ( prefix_len > length ) return false;
    for ( size_type i = 0; i < prefix_len; ++i )
      if ( memory[i] != prefix[i] ) return false;
    return true;
  }

  template <size_type M>
  inline bool
  starts_with(const T (&prefix)[M]) const
  {
    constexpr size_type prefix_len = M - 1;
    if constexpr ( prefix_len == 0 ) return true;
    if ( prefix_len > length ) return false;
    for ( size_type i = 0; i < prefix_len; ++i )
      if ( memory[i] != prefix[i] ) return false;
    return true;
  }

  template <size_type M, typename F, bool X = Sf>
  inline bool
  starts_with(const sstring<M, F, X> &prefix) const
  {
    if ( prefix.empty() ) return true;
    if ( prefix.length > length ) return false;
    for ( size_type i = 0; i < prefix.length; ++i )
      if ( memory[i] != static_cast<T>(prefix.memory[i]) ) return false;
    return true;
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
    size_type suffix_len = micron::strlen(suffix);
    if ( suffix_len > length ) return false;
    size_type offset = length - suffix_len;
    for ( size_type i = 0; i < suffix_len; ++i )
      if ( memory[offset + i] != suffix[i] ) return false;
    return true;
  }

  template <size_type M>
  inline bool
  ends_with(const T (&suffix)[M]) const
  {
    constexpr size_type suffix_len = M - 1;
    if constexpr ( suffix_len == 0 ) return true;
    if ( suffix_len > length ) return false;
    size_type offset = length - suffix_len;
    for ( size_type i = 0; i < suffix_len; ++i )
      if ( memory[offset + i] != suffix[i] ) return false;
    return true;
  }

  template <size_type M, typename F, bool X = Sf>
  inline bool
  ends_with(const sstring<M, F, X> &suffix) const
  {
    if ( suffix.empty() ) return true;
    if ( suffix.length > length ) return false;
    size_type offset = length - suffix.length;
    for ( size_type i = 0; i < suffix.length; ++i )
      if ( memory[offset + i] != static_cast<T>(suffix.memory[i]) ) return false;
    return true;
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

  template <size_type M>
  inline bool
  contains(const T (&needle)[M]) const
  {
    constexpr size_type needle_len = M - 1;
    if constexpr ( needle_len == 0 ) return true;
    return find_substr(&needle[0], needle_len) != npos;
  }

  template <size_type M, typename F, bool X = Sf>
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

  template <size_type M>
  inline sstring &
  replace(size_type pos, size_type cnt, const T (&with)[M])
  {
    __safety_check<&sstring::__range_pos_cnt, except::library_error>("micron::sstring replace() out of range", pos, cnt);
    return __replace_impl(pos, cnt, &with[0], M - 1);
  }

  template <size_type M, typename F, bool X = Sf>
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
      pos += with_len;     // advance past replacement to avoid re-matching
    }
    return *this;
  }

  template <size_type M, size_type K>
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

  template <size_type M, size_type K, typename F, typename G, bool X = Sf, bool Y = Sf>
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

  template <size_type M>
  inline int
  compare(const T (&other)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&other[0]), M - 1);
  }

  template <size_type M, typename F, bool X = Sf>
  inline int
  compare(const sstring<M, F, X> &other) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(other.memory), other.length);
  }

  size_type
  find(char ch, size_type pos = 0) const
  {
    for ( ;; pos++ ) {
      if ( memory[pos] == '\0' ) return npos;
      if ( memory[pos] == ch ) return pos;
    }
    return npos;
  }

  template <typename F>
  size_type
  find(F ch, size_type pos = 0) const
  {
    for ( ;; pos++ ) {
      if ( memory[pos] == NULL ) return npos;
      if ( memory[pos] == ch ) return pos;
    }
    return npos;
  }

  size_type
  find(const T *needle, size_type pos = 0) const
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring find() null needle", static_cast<const void *>(needle));

    size_type needle_len = micron::strlen(needle);
    if ( needle_len == 0 ) return pos;     // vacuous match, mirrors std::string

    return find_substr(reinterpret_cast<const T *>(needle), needle_len, pos);
  }

  template <size_type M>
  size_type
  find(const T (&needle)[M], size_type pos = 0) const
  {
    constexpr size_type needle_len = M - 1;
    if constexpr ( needle_len == 0 ) return pos;

    return find_substr(&needle[0], needle_len, pos);
  }

  template <size_type M, typename F, bool X = Sf>
  size_type
  find(const sstring<M, F, X> &str, size_type pos = 0) const
  {
    if ( str.empty() ) return pos;     // vacuous match

    return find_substr(reinterpret_cast<const T *>(str.memory), str.length, pos);
  }

  inline sstring &
  to_lower()
  {
    for ( size_type i = 0; i < length; ++i )
      if ( memory[i] >= static_cast<T>('A') && memory[i] <= static_cast<T>('Z') ) memory[i] += static_cast<T>('a' - 'A');
    return *this;
  }

  inline sstring &
  to_upper()
  {
    for ( size_type i = 0; i < length; ++i )
      if ( memory[i] >= static_cast<T>('a') && memory[i] <= static_cast<T>('z') ) memory[i] -= static_cast<T>('a' - 'A');
    return *this;
  }

  inline sstring &
  trim_left()
  {
    size_type i = 0;
    while ( i < length
            && (memory[i] == static_cast<T>(' ') || memory[i] == static_cast<T>('\t') || memory[i] == static_cast<T>('\n')
                || memory[i] == static_cast<T>('\r')) )
      ++i;
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
    size_type lo = 0, hi = length - 1;
    while ( lo < hi ) {
      T tmp = memory[lo];
      memory[lo] = memory[hi];
      memory[hi] = tmp;
      ++lo;
      --hi;
    }
    return *this;
  }

  inline size_type
  count(T ch) const
  {
    size_type n = 0;
    for ( size_type i = 0; i < length; ++i )
      if ( memory[i] == ch ) ++n;
    return n;
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

  template <size_type M>
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

  template <size_type M, typename F, bool X = Sf>
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

  template <typename F>
  inline sstring &
  push_back(F ch)
  {
    if ( (length + 1) < N ) memory[length++] = ch;
    return *this;
  }

  template <typename F>
  inline sstring &
  pop_back(void)
  {
    if ( length > 0 ) memory[length--] = 0x0;
    return *this;
  }

  template <typename F = T, typename I = size_type>
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

  template <typename F = T>
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

  template <typename F = T>
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
    if ( needle_len == 0 or needle_len > length ) return npos;
    size_type limit = length - needle_len;
    for ( size_type i = pos; i <= limit; ++i ) {
      size_type j = 0;
      while ( j < needle_len and memory[i + j] == needle[j] ) ++j;
      if ( j == needle_len ) return i;
    }
    return npos;
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

  template <size_type M>
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

  template <size_type M, typename F, bool X = Sf>
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

  template <size_type M>
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

  template <size_type M, typename F, bool X = Sf>
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

  template <typename I, typename F = T>
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

  template <typename I, typename F = T, size_type M>
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

  template <typename F = T>
    requires true
  inline sstring &
  insert(iterator itr, F ch, size_type cnt = 1)
  {
    if ( cnt == 0 ) [[unlikely]]
      return *this;

    __safety_check<&sstring::__insert_iter_check, except::library_error>("micron::sstring insert() out of range",
                                                                         static_cast<const T *>(itr), cnt);

    micron::bytemove(itr + cnt, itr, length - (&memory[0] - itr - 1));
    micron::typeset<T>(itr, ch, cnt);
    length += cnt;
    return *this;
  }

  template <typename F = T, size_type M>
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

  template <typename F, size_type M, bool X = Sf>
  inline sstring &
  insert(iterator itr, const sstring<M, F, X> &o)
  {
    __safety_check<&sstring::__insert_iter_check, except::library_error>("micron::sstring insert() out of range",
                                                                         static_cast<const T *>(itr), o.length);

    micron::bytemove(itr + (o.length), itr, length - (&memory[0] - itr - 1));
    micron::memcpy(itr, &o.memory[0], o.length);
    length += o.length;
    return *this;
  }

  template <typename F, size_type M, bool X = Sf>
  inline sstring &
  insert(iterator itr, sstring<M, F, X> &&o)
  {
    __safety_check<&sstring::__insert_iter_check, except::library_error>("micron::sstring insert() out of range",
                                                                         static_cast<const T *>(itr), o.length);

    micron::bytemove(itr + (o.length), itr, length - (&memory[0] - itr - 1));
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

  template <typename Itr>
    requires(micron::same_as<Itr, const_iterator>)
  inline size_type
  at(Itr n)
  {
    __safety_check<&sstring::__at_iter_check, except::library_error>("micron::sstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &memory[0];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, const_iterator>)
  inline size_type
  at(Itr n) const
  {
    __safety_check<&sstring::__at_iter_check, except::library_error>("micron::sstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &memory[0];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, iterator>)
  inline size_type
  at(Itr n)
  {
    __safety_check<&sstring::__at_iter_check, except::library_error>("micron::sstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &memory[0];
  };

  template <typename Itr>
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

  template <typename F = T>
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

  template <typename F>
  inline sstring &
  append(const slice<F> &f, usize n)
  {
    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring append() out of memory", n);

    micron::memcpy(&(memory)[length], &f[0], n);
    length += n;
    return *this;
  }

  template <typename F = T, size_type M>
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

  template <size_type M>
  inline sstring &
  operator+=(const char (&data)[M])
  {
    if ( M == 1 and data[0] == 0x0 ) return *this;

    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring operator+=() out of memory", M);

    micron::memcpy(&memory[length], &data[0], M);
    length += M - 1;
    return *this;
  };

  template <typename F = T>
  inline sstring &
  operator+=(const F *&data)
  {
    auto sz = strlen(data) + 1;

    __safety_check<&sstring::__size_check, except::library_error>("micron::sstring operator+=() out of memory", static_cast<size_type>(sz));

    micron::memcpy(&memory[length], &data[0], sz);
    length += sz - 1;
    return *this;
  };

  template <size_type M = N, typename F = T, bool X = Sf>
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

  template <size_type M = N, typename F = T, bool X = Sf>
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

  template <typename I>
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

  template <is_string S>
  inline bool
  operator==(const S &str) const
  {
    return __lexcmp(memory, length, str.c_str(), str.size()) == 0;
  }

  template <is_string S>
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

  template <typename F = T, size_type M>
  inline bool
  operator==(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) == 0;
  }

  template <typename F = T, size_type M>
  inline bool
  operator!=(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) != 0;
  }

  template <typename F = T, size_type M>
  inline bool
  operator<(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) < 0;
  }

  template <typename F = T, size_type M>
  inline bool
  operator>(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) > 0;
  }

  template <typename F = T, size_type M>
  inline bool
  operator<=(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) <= 0;
  }

  template <typename F = T, size_type M>
  inline bool
  operator>=(const F (&data)[M]) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(&data[0]), M - 1) >= 0;
  }

  template <size_type M, typename F = T, bool X = Sf>
  inline bool
  operator==(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) == 0;
  }

  template <size_type M, typename F = T, bool X = Sf>
  inline bool
  operator!=(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) != 0;
  }

  template <size_type M, typename F = T, bool X = Sf>
  inline bool
  operator<(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) < 0;
  }

  template <size_type M, typename F = T, bool X = Sf>
  inline bool
  operator>(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) > 0;
  }

  template <size_type M, typename F = T, bool X = Sf>
  inline bool
  operator<=(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) <= 0;
  }

  template <size_type M, typename F = T, bool X = Sf>
  inline bool
  operator>=(const sstring<M, F, X> &data) const
  {
    return __lexcmp(memory, length, reinterpret_cast<const T *>(data.memory), data.length) >= 0;
  }
};

};     // namespace micron
