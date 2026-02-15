//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../type_traits.hpp"

#include "../algorithm/memory.hpp"
#include "../allocation/resources.hpp"
#include "../allocator.hpp"
#include "../concepts.hpp"
#include "../memory/memory.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"

#include "unitypes.hpp"

namespace micron
{
// string on the stack, inplace (sstring means stackstring)
template <size_t N, is_scalar_literal T = schar> class sstring
{
public:
  using category_type = string_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef T value_type;
  typedef T *iterator;
  typedef const T *const_iterator;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef size_t size_type;
  typedef T &reference;
  typedef const T &const_reference;

  T memory[N];
  size_type length;
  constexpr sstring()
  {
    micron::zero(&memory[0], N);
    length = 0;
  };
  constexpr sstring(const char *str)
  {
    micron::zero(&memory[0], N);
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
    micron::zero(&memory[0], N);
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
      exc<except::library_error>("micron::hstring sstring() wrong iterators");
    micron::memcpy(&memory[0], __start, __end - __start);
    length = __end - __start;
  };
  constexpr sstring(const_iterator __start, const_iterator __end)
  {
    if ( __start >= __end )
      exc<except::library_error>("micron::hstring sstring() wrong iterators");
    micron::memcpy(&memory[0], __start, __end - __start);
    length = __end - __start;
  };
  template <size_type M, typename F> constexpr sstring(const sstring<M, F> &o)
  {
    // removed, only copy what we can
    // static_assert(N >= M, "micron::sstring sstring(cconst) too large.");
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
    micron::memcpy(&memory[0], &o.memory[0], N);
    length = o.length;
  };
  constexpr sstring(const sstring &o)
  {
    micron::memcpy(&memory[0], &o.memory[0], N);
    length = o.length;
  };
  constexpr sstring(sstring &&o)
  {
    micron::memcpy(&memory[0], &o.memory[0], N);
    micron::zero(&o.memory[0], N);
    length = o.length;
    o.length = 0;
  };
  template <size_type M, typename F> constexpr sstring(sstring<M, F> &&o)
  {
    if constexpr ( N < M ) {
      micron::memcpy(&memory[0], &o.memory[0], N);
      micron::zero(&o.memory[0], N);
    } else if constexpr ( N >= M ) {
      micron::memcpy(&memory[0], &o.memory[0], M);
      micron::memcpy(&memory[0], &o.memory[0], M);
      micron::zero(&o.memory[0], N);
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
    micron::memcpy(&memory[0], &o.memory[0], N);
    length = o.length;
    micron::czero<N>(&o.memory[0]);
    o.length = 0;
    return *this;
  }
  sstring &
  operator=(const sstring &o)
  {
    micron::memcpy(&memory[0], &o.memory[0], N);
    length = o.length;
    return *this;
  }
  template <typename F>
  sstring &
  operator=(const F &o)
  {
    if ( o.size() < N ) {
      micron::memcpy(&memory[0], &o.cdata()[0], o.size());
      length = o.size();
    } else {
      micron::memcpy(&memory[0], &o.cdata()[0], N);
      length = o.size();
    }
    return *this;
  }
  template <size_type M, typename F>
  sstring &
  operator=(const sstring<M, F> &o)
  {
    static_assert(N >= M, "micron::sstring operator= too large.");
    micron::memcpy(&memory[0], &o.memory[0], M);
    length = o.length;
    return *this;
  }
  template <size_type M, typename F>
  sstring &
  operator=(sstring<M, F> &&o)
  {
    static_assert(N >= M, "micron::sstring operator= too large.");
    micron::memcpy(&memory[0], &o.memory[0], M);
    micron::zero(&o.memory[0], M);
    length = o.length;
    o.length = 0;
    return *this;
  }
  sstring &
  operator=(char *ptr)
  {
    size_type n = strlen(ptr);
    micron::zero(&memory[0], N);
    micron::memcpy(&memory[0], &ptr[0], n);
    length = n;
    return *this;
  }
  sstring &
  operator=(const char *ptr)
  {
    size_type n = strlen(ptr);
    micron::zero(&memory[0], N);
    micron::memcpy(&memory[0], &ptr[0], n);
    length = n;
    return *this;
  }
  template <typename F>
  sstring &
  operator=(const F (&str)[N])
  {
    micron::memcpy(&memory, &str, N);
    length = N;
    return *this;
  }
  ~sstring() {}

  bool
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
  bool
  empty() const
  {
    return (bool)length == 0;
  };
  void
  set_size(size_type n)
  {
    length = n;
  }
  size_type
  size() const
  {
    return length;
  }
  size_type
  capacity() const
  {
    return N;
  }
  size_type
  max_size() const
  {
    return N;
  }
  // unsafe
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
  inline const char *
  c_str()
  {
    return const_cast<const char *>(reinterpret_cast<char *>(&memory[0]));
  }
  inline const char *
  c_str() const
  {
    return const_cast<const char *>(reinterpret_cast<const char *>(&memory[0]));
  }
  inline slice<byte>
  into_bytes()
  {
    return slice<byte>(reinterpret_cast<byte *>(&memory[0]), reinterpret_cast<byte *>(&memory[length]));
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
  // if used for buffering, adjust the length of the internals
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
  template <size_type M = N, typename F = T>
  size_type
  find(const sstring<M, F> &str, size_type pos = 0) const
  {
    for ( ;; pos++ ) {
    }
    return npos;
  }
  // end points to one-past-last (ie null) while last points to the last OCCUPIED element
  inline iterator
  begin() const
  {
    return const_cast<T *>(&memory[0]);
  }
  inline iterator
  end() const
  {
    return const_cast<T *>(&memory[length]);
  }
  inline iterator
  begin()
  {
    return const_cast<T *>(&memory[0]);
  }
  inline iterator
  end()
  {
    return const_cast<T *>(&memory[length]);
  }
  inline iterator
  last()
  {
    return const_cast<T *>(&memory[length - 1]);
  }
  inline iterator
  last() const
  {
    return const_cast<T *>(&memory[length - 1]);
  }
  inline const_iterator
  cbegin() const
  {
    return &(memory)[0];
  }
  inline const_iterator
  cend() const
  {
    return &(memory)[length];
  }
  inline void
  clear()
  {
    micron::zero(&memory);
    length = 0;
  }
  // doesn't zero memory, just resets the cur. pointer to 0
  inline void
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
    requires micron::is_arithmetic_v<I>
  inline sstring &
  erase(I ind, size_type cnt = 1)
  {
    if ( ind >= length || cnt > length - ind )
      exc<except::library_error>("micron:sstring erase() out of range");
    if ( cnt > N )
      exc<except::library_error>("micron:sstring erase() out of range");
    if ( !cnt )
      return *this;
    micron::bytemove(&memory[ind], &memory[ind + (1 + (cnt - 1))], length - (ind + 1 + (cnt - 1)));
    micron::typeset<T>(&memory[length - (cnt)], 0x0, cnt);
    length -= cnt;
    return *this;
  }

  template <typename F = T>
  inline sstring &
  erase(iterator itr, size_type cnt = 1)
  {
    if ( itr < begin() || itr > end() || static_cast<size_type>(cnt) > static_cast<size_type>(end() - itr) )
      exc<except::library_error>("micron:sstring erase() out of range");
    if ( cnt > N )
      exc<except::library_error>("micron:sstring erase() out of range");
    // micron::memmove(itr - cnt + 1, itr, length - (&memory[0] - itr - 1));
    micron::bytemove(itr, itr + (1 + (cnt - 1)), length - ((itr - &memory[0]) + 1 + (cnt - 1)));
    micron::typeset<T>(&memory[length - cnt], 0x0, cnt);
    length -= cnt;
    return *this;
  }
  template <typename F = T>
  inline sstring &
  erase(const_iterator itr, size_type cnt = 1)
  {
    if ( itr < begin() or itr > end() or cnt > (end() - itr) )
      exc<except::library_error>("micron:sstring erase() out of range");
    if ( cnt > N )
      exc<except::library_error>("micron:sstring erase() out of range");
    micron::bytemove(itr, itr + (1 + (cnt - 1)), length - ((itr - &memory[0]) + 1 + (cnt - 1)));
    micron::typeset<T>(&memory[length - cnt], 0x0, cnt);
    length -= cnt;
    return *this;
  }

  template <typename F = T>
  inline sstring &
  insert(size_type ind, F ch, size_type cnt = 1)
  {
    if ( ind >= length || cnt > N - length )
      exc<except::library_error>("micron:sstring insert() out of range");
    micron::bytemove(&memory[ind + cnt], &memory[ind], length - ind);
    micron::typeset<T>(&memory[ind], ch, cnt);
    length += cnt;
    return *this;
  }
  template <typename F = T, size_type M>
  inline sstring &
  insert(size_type ind, const F (&str)[M], size_type cnt = 1)
  {
    size_type str_len = M - 1;     // strlen(str);
    if ( ind > length || length + cnt * str_len >= N )
      exc<except::library_error>("micron:sstring insert() out of range");

    micron::bytemove(&memory[ind + cnt * str_len], &memory[ind], length - ind);
    for ( size_type i = 0; i < cnt; ++i )
      micron::memcpy(&memory[ind + i * str_len], str, str_len);

    length += (cnt * str_len);
    return *this;
  }
  template <typename F = T>
  inline sstring &
  insert(iterator itr, F ch, size_type cnt = 1)
  {
    if ( length >= N or length + cnt >= N )
      exc<except::library_error>("micron:sstring insert() out of range");
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
    if ( itr < memory || itr > memory + length || length + cnt * str_len > N )
      exc<except::library_error>("micron:sstring insert() out of range");

    micron::bytemove(itr + cnt * str_len, itr, (memory + length) - itr);

    for ( size_type i = 0; i < cnt; ++i )
      micron::memcpy(itr + i * str_len, str, str_len);
    length += (cnt * str_len);
    return *this;
  }

  template <typename F, size_type M>
  inline sstring &
  insert(iterator itr, const sstring<M, F> &o)
  {
    if ( itr < memory || itr > memory + length || length + o.length >= N )
      exc<except::library_error>("micron:sstring insert() out of range");
    micron::bytemove(itr + (o.length), itr, length - (&memory[0] - itr - 1));
    micron::memcpy(itr, &o.memory[0], o.length);
    length += o.length;
    return *this;
  }
  template <typename F, size_type M>
  inline sstring &
  insert(iterator itr, sstring<M, F> &&o)
  {
    if ( (length + o.length) >= N )
      exc<except::library_error>("micron:sstring insert() out of range");
    micron::bytemove(itr + (o.length), itr, length - (&memory[0] - itr - 1));
    micron::memcpy(itr, &o.memory[0], o.length);

    length += o.length;
    micron::zero(o.memory, o.length);
    o.length = 0;
    return *this;
  }

  inline T &
  at(const size_type n)
  {
    if ( n >= length )
      exc<except::library_error>("micron:sstring at() out of range");
    return memory[n];
  };

  inline T &
  at(const size_type n) const
  {
    if ( n >= length )
      exc<except::library_error>("micron:sstring at() out of range");
    return memory[n];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, const_iterator>)
  inline size_type
  at(Itr n)
  {
    if ( n - &memory[0] > length or n - &memory[0] < 0 )
      exc<except::library_error>("micron:sstring at() out of range");
    return n - &memory[0];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, const_iterator>)
  inline size_type
  at(Itr n) const
  {
    if ( n - &memory[0] > length or n - &memory[0] < 0 )
      exc<except::library_error>("micron:sstring at() out of range");
    return n - &memory[0];
  };
  template <typename Itr>
    requires(micron::same_as<Itr, iterator>)
  inline size_type
  at(Itr n)
  {
    if ( n - &memory[0] > length or n - &memory[0] < 0 )
      exc<except::library_error>("micron:sstring at() out of range");
    return n - &memory[0];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, iterator>)
  inline size_type
  at(Itr n) const
  {
    if ( n - &memory[0] > length or n - &memory[0] < 0 )
      exc<except::library_error>("micron:sstring at() out of range");
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
    if ( length + data.length >= N )
      exc<except::library_error>("micron::sstring += out of memory.");
    micron::memcpy(&memory[length], &data.memory[0], data.length);
    length += data.length;
    return *this;
  };
  inline sstring &
  append(const sstring &o)
  {
    if ( (length + o.length) >= N )
      exc<except::library_error>("micron::sstring append() out of memory.");
    micron::memcpy(&memory[length], &o.memory[0], o.length);
    length += o.length;
    return *this;
  }
  template <typename F = T, size_type M>
  inline sstring &
  append_null(const F (&str)[M])
  {
    if ( (length + M) >= N )
      exc<except::library_error>("micron::sstring append() out of memory.");
    micron::memcpy(&memory[length], &str[0], M - 1);
    length += M - 1;
    return *this;
  }
  inline void
  null_term(void)
  {
    memory[length] = 0x0;
  }
  // was missing this
  inline sstring &
  operator+=(char ch)
  {
    if ( length + 1 >= N )
      exc<except::library_error>("micron::sstring += out of memory.");
    memory[length++] = ch;
    return *this;
  };
  inline sstring &
  operator+=(const buffer &data)
  {
    if ( length + data.size() >= N )
      exc<except::library_error>("micron::sstring += out of memory.");
    micron::memcpy(&memory[length], &data, data.size());
    length += data.size();
    return *this;
  };

  template <size_type M>
  inline sstring &
  operator+=(const char (&data)[M])
  {
    if ( length + M >= N )
      exc<except::library_error>("micron::sstring += out of memory.");
    micron::memcpy(&memory[length], &data[0], M);
    length += M - 1;
    return *this;
  };
  template <typename F = T>
  inline sstring &
  operator+=(const F *&data)
  {
    auto sz = strlen(data) + 1;
    if ( length + sz >= N )
      exc<except::library_error>("micron::sstring += out of memory.");
    micron::memcpy(&memory[length], &data[0], sz);
    length += sz - 1;
    return *this;
  };
  template <size_type M = N, typename F = T>
  inline sstring<M, F>
  substr(size_type pos = 0, size_type cnt = 0) const
  {
    if ( pos > N or cnt > N or (pos + cnt) > N )
      exc<except::library_error>("error micron::sstring substr invalid range.");
    sstring<M, F> buf;
    micron::memcpy(&buf.data()[0], &memory[pos], cnt);
    buf[cnt] = '\0';
    buf.set_size(cnt);
    return buf;
  };
  template <size_type M = N, typename F = T>
  inline sstring<M, F>
  substr(const_iterator _start, const_iterator _end) const
  {
    if ( _start < begin() or _end > end() )
      exc<except::library_error>("error micron::sstring substr invalid range.");
    sstring<M, F> buf;
    micron::memcpy(&buf.data()[0], _start, _end - _start);
    buf[_end - _start] = '\0';
    buf.set_size(_end - _start);
    return buf;
  };
  inline bool
  operator==(const char *data) const
  {
    auto sz = strlen(data);
    if ( sz > length )
      return false;
    for ( size_type i = 0; i < sz; i++ )
      if ( data[i] != memory[i] )
        return false;
    return true;
  };
  inline bool
  operator==(const sstring &data) const
  {
    if ( data.length == length ) {
      for ( size_type i = 0; i < length; i++ )
        if ( data.memory[i] != memory[i] )
          return false;
    } else
      return false;
    return true;
  };
  bool
  operator!=(const sstring &o) const
  {
    if ( o.length == length ) {
      for ( size_type i = 0; i < length; i++ )
        if ( o.memory[i] != memory[i] )
          return true;
    } else
      return true;
    return false;
  }
  bool
  operator!=(const char *str) const
  {
    auto sz = strlen(str);
    if ( sz != length )
      return true;     // must be unequal
    for ( size_type i = 0; i < sz; i++ ) {
      if ( memory[i] != str[i] )
        return true;
    }
    return false;
  }
  template <typename F, size_type M>
  bool
  operator!=(const F (&str)[M]) const
  {
    if ( M != N )
      return true;
    for ( size_type i = 0; i < length; i++ )
      if ( memory[i] != str[i] )
        return true;
    return false;
  }
};

};
