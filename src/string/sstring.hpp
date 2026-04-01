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
      if ( ca < cb )
        return -1;
      if ( ca > cb )
        return 1;
    }
    if ( alen < blen )
      return -1;
    if ( alen > blen )
      return 1;
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
    if ( sz > N )
      exc<except::library_error>("sstring::sstring() const char* too large.");
    micron::memcpy(&memory[0], &str[0], sz + 1);
    length = sz;
  };

  constexpr sstring(char *ptr)
  {
    size_type n = micron::strlen(ptr);
    if ( n >= N )
      exc<except::library_error>("sstring::sstring(): char* too large.");
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
    if ( __start >= __end )
      exc<except::library_error>("micron::sstring sstring() wrong iterators");
    if ( static_cast<usize>(__end - __start) + 1 > N )
      exc<except::library_error>("sstring(iter,iter): range too large.");
    micron::constexpr_zero(&memory[0], N);
    micron::memcpy(&memory[0], __start, __end - __start);
    length = __end - __start;
  };

  constexpr sstring(const_iterator __start, const_iterator __end)
  {
    if ( __start >= __end )
      exc<except::library_error>("micron::sstring sstring() wrong iterators");
    if ( static_cast<usize>(__end - __start) + 1 > N )
      exc<except::library_error>("sstring(iter,iter): range too large.");
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
    if ( ptr == nullptr )
      return *this;
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
    if ( ptr == nullptr )
      return *this;
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

  size_type
  find(char ch, size_type pos = 0) const
  {
    for ( ;; pos++ ) {
      if ( memory[pos] == '\0' )
        return npos;
      if ( memory[pos] == ch )
        return pos;
    }
    return npos;
  }

  template <typename F>
  size_type
  find(F ch, size_type pos = 0) const
  {
    for ( ;; pos++ ) {
      if ( memory[pos] == NULL )
        return npos;
      if ( memory[pos] == ch )
        return pos;
    }
    return npos;
  }

  template <size_type M = N, typename F = T, bool X = Sf>
  size_type
  find(const sstring<M, F, X> &str, size_type pos = 0) const
  {
    for ( ;; pos++ ) {
    }
    return npos;
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
    if ( (length) > 0 )
      (memory)[--length] = 0x0;
    return *this;
  }

  inline sstring &
  push_back(char ch)
  {
    if ( (length + 1) < N )
      memory[length++] = ch;
    return *this;
  }

  template <typename F>
  inline sstring &
  push_back(F ch)
  {
    if ( (length + 1) < N )
      memory[length++] = ch;
    return *this;
  }

  template <typename F>
  inline sstring &
  pop_back(void)
  {
    if ( length > 0 )
      memory[length--] = 0x0;
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
    if ( needle_len == 0 or needle_len > length )
      return npos;
    size_type limit = length - needle_len;
    for ( size_type i = pos; i <= limit; ++i ) {
      size_type j = 0;
      while ( j < needle_len and memory[i + j] == needle[j] )
        ++j;
      if ( j == needle_len )
        return i;
    }
    return npos;
  }

  inline sstring &
  remove(const char *needle)
  {
    __safety_check<&sstring::__null_check, except::library_error>("micron::sstring remove() null needle",
                                                                  static_cast<const void *>(needle));

    size_type needle_len = micron::strlen(needle);
    if ( needle_len == 0 )
      return *this;

    size_type pos = find_substr(reinterpret_cast<const T *>(needle), needle_len);
    if ( pos == npos )
      return *this;

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
    if constexpr ( needle_len == 0 )
      return *this;

    size_type pos = find_substr(&needle[0], needle_len);
    if ( pos == npos )
      return *this;

    micron::bytemove(&memory[pos], &memory[pos + needle_len], length - (pos + needle_len));
    micron::typeset<T>(&memory[length - needle_len], 0x0, needle_len);
    length -= needle_len;
    return *this;
  }

  template <size_type M, typename F, bool X = Sf>
  inline sstring &
  remove(const sstring<M, F, X> &needle)
  {
    if ( needle.empty() )
      return *this;

    size_type pos = find_substr(needle.memory, needle.length);
    if ( pos == npos )
      return *this;

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
    if ( needle_len == 0 )
      return *this;

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
    if constexpr ( needle_len == 0 )
      return *this;

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
    if ( needle.empty() )
      return *this;

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
    for ( size_type i = 0; i < cnt; ++i )
      micron::memcpy(&memory[ind + i * str_len], str, str_len);

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
    for ( size_type i = 0; i < cnt; ++i )
      micron::memcpy(itr + i * str_len, str, str_len);
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
    if ( data.empty() )
      return *this;

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
    if ( o.empty() )
      return *this;

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
    if ( M == 1 and data[0] == 0x0 )
      return *this;

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
    if ( cnt == npos )
      cnt = (pos < length) ? (length - pos) : 0;

    __safety_check<&sstring::__range_pos_cnt, except::library_error>("micron::sstring substr() invalid range", static_cast<size_type>(pos),
                                                                     static_cast<size_type>(cnt));

    if ( cnt >= M )
      exc<except::library_error>("micron::sstring substr() result too large for target.");

    sstring<M, F, X> buf;
    micron::memcpy(&buf.data()[0], &memory[pos], cnt);
    buf.set_size(cnt);
    return buf;
  };

  template <size_type M = N, typename F = T, bool X = Sf>
  inline sstring<M, F>
  substr(const_iterator _start, const_iterator _end = nullptr) const
  {
    if ( _end == nullptr )
      _end = cend();

    __safety_check<&sstring::__iter_substr_check, except::library_error>("micron::sstring substr() invalid range", _start, _end);

    size_type len = static_cast<size_type>(_end - _start);

    if ( len >= M )
      exc<except::library_error>("micron::sstring substr() result too large for target.");

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
    if ( static_cast<size_type>(n) >= length )
      return *this;
    if ( static_cast<size_type>(n) > N )
      n = N;
    micron::typeset<T>(&memory[static_cast<size_type>(n)], 0x0, length - static_cast<size_type>(n));
    length = n;
    return *this;
  }

  inline sstring &
  truncate(iterator itr)
  {
    __safety_check<&sstring::__iterator_check, except::library_error>("micron::sstring truncate() iterator out of range", itr);

    if ( itr >= end() )
      return *this;

    size_type n = static_cast<size_type>(itr - begin());
    micron::typeset<T>(&memory[n], 0x0, length - n);
    length = n;
    return *this;
  }

  inline sstring &
  truncate(const_iterator itr)
  {
    __safety_check<&sstring::__iterator_check, except::library_error>("micron::sstring truncate() iterator out of range", itr);

    if ( itr >= cend() )
      return *this;

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
