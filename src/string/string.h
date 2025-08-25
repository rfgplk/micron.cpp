//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../type_traits.hpp"

#include "../algorithm/mem.hpp"
#include "../allocation/chunks.hpp"
#include "../allocator.hpp"
#include "../memory/memory.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"

#include "unitypes.hpp"

namespace micron
{
// null terminated

static const ssize_t npos = -1;

// string on the stack, inplace (sstring means stackstring)
template <size_t N, typename T = schar> class sstring
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
  size_t length;
  constexpr sstring()
  {
    micron::zero(&memory[0], N);
    length = 0;
  };
  constexpr sstring(const char *&str)
  {
    micron::zero(&memory[0], N);
    size_t sz = strlen(str);
    if ( sz > N )
      throw except::library_error("sstring::sstring() const char* too large.");
    micron::memcpy(&memory[0], &str[0], sz + 1);
    length = sz;
  };
  constexpr sstring(char *ptr)
  {
    ssize_t n = micron::strlen(ptr);
    micron::zero(&memory[0], N);
    micron::memcpy(&memory[0], &ptr[0], n);
    length = n;
  }
  template <size_t M, typename F> constexpr sstring(const F (&str)[M])
  {
    static_assert(N >= M, "micron::sstring sstring(cconst) too large.");
    micron::memcpy(&memory[0], &str[0], M);
    length = M - 1;     // cut null
  };
  template <size_t M, typename F> constexpr sstring(const sstring<M, F> &o)
  {
    static_assert(N >= M, "micron::sstring sstring(cconst) too large.");
    micron::memcpy(&memory[0], &o.memory[0], M);
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
  template <size_t M, typename F> constexpr sstring(sstring<M, F> &&o)
  {
    static_assert(N >= M, "micron::sstring sstring(mconst) too large.");
    micron::memcpy(&memory[0], &o.memory[0], M);
    micron::zero(&o.memory[0], N);
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
  template <size_t M, typename F>
  sstring &
  operator=(const sstring<M, F> &o)
  {
    static_assert(N >= M, "micron::sstring operator= too large.");
    micron::memcpy(&memory[0], &o.memory[0], M);
    length = o.length;
    return *this;
  }
  template <size_t M, typename F>
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
    ssize_t n = strlen(ptr);
    micron::zero(&memory[0], N);
    micron::memcpy(&memory[0], &ptr[0], n);
    length = n;
    return *this;
  }
  sstring &
  operator=(const char *ptr)
  {
    ssize_t n = strlen(ptr);
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
  set_size(size_t n)
  {
    length = n;
  }
  size_t
  size() const
  {
    return length;
  }
  size_t
  capacity() const
  {
    return N;
  }
  size_t
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
  data() -> T *
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
  _buf_set_length(const size_t s)
  {
    length = s;
  }
  void
  adjust_size()
  {
    auto ln = micron::strlen(memory);
    length = ln;
  }
  size_t
  find(char ch, size_t pos = 0) const
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
  size_t
  find(F ch, size_t pos = 0) const
  {
    for ( ;; pos++ ) {
      if ( memory[pos] == NULL )
        return npos;
      if ( memory[pos] == ch )
        return pos;
    }
    return npos;
  }
  template <size_t M = N, typename F = T>
  size_t
  find(const sstring<M, F> &str, size_t pos = 0) const
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
  template <typename F = T, typename I = size_t>
    requires micron::is_arithmetic_v<I>
  inline sstring &
  erase(I ind, size_t cnt = 1)
  {
    if ( cnt > N )
      throw except::library_error("micron:sstring erase() out of range");
    if ( !cnt )
      return *this;
    micron::bytemove(&memory[ind], &memory[ind + (1 + (cnt - 1))], length - (ind + 1 + (cnt - 1)));
    micron::typeset<T>(&memory[length - (cnt)], 0x0, cnt);
    length -= cnt;
    return *this;
  }

  template <typename F = T>
  inline sstring &
  erase(iterator itr, size_t cnt = 1)
  {
    if ( cnt > N )
      throw except::library_error("micron:sstring erase() out of range");
    // micron::memmove(itr - cnt + 1, itr, length - (&memory[0] - itr - 1));
    micron::bytemove(itr, itr + (1 + (cnt - 1)), length - ((itr - &memory[0]) + 1 + (cnt - 1)));
    micron::typeset<T>(&memory[length - cnt], 0x0, cnt);
    length -= cnt;
    return *this;
  }
  template <typename F = T>
  inline sstring &
  erase(const_iterator itr, size_t cnt = 1)
  {
    if ( cnt > N )
      throw except::library_error("micron:sstring erase() out of range");
    micron::bytemove(itr, itr + (1 + (cnt - 1)), length - ((itr - &memory[0]) + 1 + (cnt - 1)));
    micron::typeset<T>(&memory[length - cnt], 0x0, cnt);
    length -= cnt;
    return *this;
  }

  template <typename F = T>
  inline sstring &
  insert(size_t ind, F ch, size_t cnt = 1)
  {
    if ( length >= N or length + cnt >= N )
      throw except::library_error("micron:sstring insert() out of range");
    char *buf = reinterpret_cast<char *>(malloc(cnt));
    micron::typeset<T>(&buf[0], ch, cnt);
    micron::bytemove(&memory[ind + cnt], &memory[ind], length - ind);
    micron::memcpy(&memory[ind], &buf[0], cnt);
    free(buf);
    length += cnt;
    return *this;
  }
  template <typename F = T, size_t M>
  inline sstring &
  insert(size_t ind, const F (&str)[M], size_t cnt = 1)
  {
    if ( length >= N or length + cnt >= N )
      throw except::library_error("micron:sstring insert() out of range");
    ssize_t str_len = strlen(str);
    char *buf = reinterpret_cast<char *>(malloc(cnt * str_len));
    for ( size_t i = 0; i < cnt; i++ )
      micron::memcpy(&buf[str_len * i], &str[0], str_len);
    micron::bytemove(&memory[ind + (cnt * str_len)], &memory[ind], length - ind);
    micron::memcpy(&memory[ind], &buf[0], cnt * str_len);
    free(buf);
    length += (cnt * str_len);
    return *this;
  }
  template <typename F = T>
  inline sstring &
  insert(iterator itr, F ch, size_t cnt = 1)
  {
    if ( length >= N or length + cnt >= N )
      throw except::library_error("micron:sstring insert() out of range");
    char *buf = reinterpret_cast<char *>(malloc(cnt));
    micron::typeset<T>(&buf[0], ch, cnt);
    micron::bytemove(itr + cnt, itr, length - (&memory[0] - itr - 1));
    micron::memcpy(itr, &buf[0], cnt);
    free(buf);
    length += cnt;
    return *this;
  }
  template <typename F = T, size_t M>
  inline sstring &
  insert(iterator itr, const F (&str)[M], size_t cnt = 1)
  {
    if ( length >= N or length + cnt >= N )
      throw except::library_error("micron:sstring insert() out of range");
    ssize_t str_len = strlen(str);
    char *buf = reinterpret_cast<char *>(malloc(cnt * str_len));
    for ( size_t i = 0; i < cnt; i++ )
      micron::memcpy(&buf[str_len * i], &str[0], str_len);
    micron::bytemove(itr + (cnt * str_len), itr, length - (&memory[0] - itr - 1));
    micron::memcpy(itr, &buf[0], cnt * str_len);
    free(buf);
    length += (cnt * str_len);
    return *this;
  }

  template <typename F, size_t M>
  inline sstring &
  insert(iterator itr, const sstring<M, F> &o)
  {
    if ( (length + o.length) >= N )
      throw except::library_error("micron:sstring insert() out of range");
    micron::bytemove(itr + (o.length), itr, length - (&memory[0] - itr - 1));
    micron::memcpy(itr, &o.memory[0], o.length);
    length += o.length;
    return *this;
  }
  template <typename F, size_t M>
  inline sstring &
  insert(iterator itr, sstring<M, F> &&o)
  {
    if ( (length + o.length) >= N )
      throw except::library_error("micron:sstring insert() out of range");
    micron::bytemove(itr + (o.length), itr, length - (&memory[0] - itr - 1));
    micron::memcpy(itr, &o.memory[0], o.length);

    length += o.length;
    micron::zero(o.memory, o.length);
    o.length = 0;
    return *this;
  }

  inline T &
  at(const size_t n)
  {
    if ( n >= length )
      throw except::library_error("micron:sstring at() out of range");
    return memory[n];
  };

  inline T &
  at(const size_t n) const
  {
    if ( n >= length )
      throw except::library_error("micron:sstring at() out of range");
    return memory[n];
  };

  inline size_t
  at(const_iterator n)
  {
    if ( n - &memory[0] > 256 or n - &memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &memory[0];
  };

  inline size_t
  at(const_iterator n) const
  {
    if ( n - &memory[0] > 256 or n - &memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &memory[0];
  };
  inline size_t
  at(iterator n)
  {
    if ( n - &memory[0] > 256 or n - &memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &memory[0];
  };

  inline size_t
  at(iterator n) const
  {
    if ( n - &memory[0] > 256 or n - &memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &memory[0];
  };
  inline T &
  operator[](const size_t n)
  {
    return memory[n];
  };
  inline const T &
  operator[](const size_t n) const
  {
    return memory[n];
  };
  inline sstring &
  operator+=(const sstring &data)
  {
    if ( length + data.length >= N )
      throw except::library_error("micron::sstring += out of memory.");
    micron::memcpy(&memory[length], &data.memory[0], data.length);
    length += data.length;
    return *this;
  };

  inline sstring &
  operator+=(const buffer &data)
  {
    if ( length + data.size() >= N )
      throw except::library_error("micron::sstring += out of memory.");
    micron::memcpy(&memory[length], &data, data.size());
    length += data.size();
    return *this;
  };

  template <size_t M>
  inline sstring &
  operator+=(const char (&data)[M])
  {
    if ( length + M >= N )
      throw except::library_error("micron::sstring += out of memory.");
    micron::memcpy(&memory[length], &data[0], M);
    length += M;
    return *this;
  };
  template <typename F = T>
  inline sstring &
  operator+=(const F *&data)
  {
    auto sz = strlen(data);
    if ( length + sz >= N )
      throw except::library_error("micron::sstring += out of memory.");
    micron::memcpy(&memory[length], &data[0], sz);
    length += sz;
    return *this;
  };
  template <size_t M = N, typename F = T>
  inline sstring<M, F>
  substr(size_t pos = 0, size_t cnt = 0) const
  {
    if ( pos > N or cnt > N or (pos + cnt) > N )
      throw except::library_error("error micron::sstring substr invalid range.");
    sstring<M, F> buf;
    micron::memcpy(&buf.data()[0], &memory[pos], cnt);
    buf[cnt] = '\0';
    return buf;
  };
  inline bool
  operator==(const char *data) const
  {
    auto sz = strlen(data);
    if ( sz > length )
      return false;
    for ( size_t i = 0; i < sz; i++ )
      if ( data[i] != memory[i] )
        return false;
    return true;
  };
  inline bool
  operator==(const sstring &data) const
  {
    if ( data.length == length ) {
      for ( size_t i = 0; i < length; i++ )
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
      for ( size_t i = 0; i < length; i++ )
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
    for ( size_t i = 0; i < sz; i++ ) {
      if ( memory[i] != str[i] )
        return true;
    }
    return false;
  }
  template <typename F, size_t M>
  bool
  operator!=(const F (&str)[M]) const
  {
    if ( M != N )
      return true;
    for ( size_t i = 0; i < length; i++ )
      if ( memory[i] != str[i] )
        return true;
    return false;
  }
};

template <typename T>
concept non_class = !requires { typename T::type; };

constexpr const char _null_str[1] = "";
constexpr const wide _null_wstr[1] = L"";
constexpr const unicode32 _null_u32str[1] = U"";

// string on the heap, mutable, standard replacement of std::string
// accepts only char simple types
template <non_class T = schar, class Alloc = micron::allocator_serial<>>
class hstring : private Alloc, public contiguous_memory<T>
{
  using __mem = contiguous_memory<T>;

public:
  using category_type = string_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef size_t size_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  // contiguous_memory<T> memory;
  constexpr hstring()
      : contiguous_memory<T>(Alloc::create((Alloc::auto_size() >= sizeof(T) ? Alloc::auto_size() : sizeof(T)))) {};
  constexpr hstring(const size_t n) : contiguous_memory<T>(Alloc::create(n)) {};
  hstring(size_t cnt, T ch) : contiguous_memory<T>(Alloc::create(cnt < Alloc::auto_size() ? Alloc::auto_size() : cnt))
  {
    micron::typeset<T>(&__mem::memory[0], ch, cnt);
    __mem::length = cnt;
  }
  constexpr hstring(const char *&str) : contiguous_memory<T>(Alloc::create(micron::strlen(str)))
  {
    size_t end = micron::strlen(str);
    micron::memcpy(&(__mem::memory)[0], &str[0], end);     // - 1);
    __mem::length = end;                                   // - 1;
  };
  template <size_t M, typename F> constexpr hstring(const F (&str)[M]) : contiguous_memory<T>(Alloc::create(M))
  {
    micron::bytecpy(&(__mem::memory)[0], &str[0], M * sizeof(F));     // - 1);
    __mem::length = M - 1;                                            // - 1;
  };
  constexpr hstring(const hstring &o) : Alloc(), contiguous_memory<T>(this->create(o.capacity))
  {
    micron::strcpy(__mem::memory, o.memory);
    __mem::length = o.length;
  };
  constexpr hstring(hstring &&o)
  {
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  }
  template <typename F> constexpr hstring(const hstring<F> &o) : contiguous_memory<T>(this->create(o.capacity))
  {
    micron::strcpy(__mem::memory, o.memory);
    __mem::length = o.length;
  };
  template <size_t N, typename F>
  constexpr hstring(const sstring<N, F> &o) : contiguous_memory<T>(this->create(o.length))
  {
    memcpy(&(__mem::memory)[0], &o.memory[0],
           o.length);             // - 1);
    __mem::length = o.length;     // - 1; // no null
  };

  hstring &
  operator=(const hstring &o)
  {
    if ( __mem::capacity < o.capacity )
      reserve(o.capacity + 1);
    micron::zero(__mem::memory, __mem::capacity);
    micron::strcpy(__mem::memory, o.memory);
    __mem::length = o.length;
    return *this;
  }
  hstring &
  operator=(hstring &&o)
  {
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
    ;
    return *this;
  }
  template <template <typename> class S, typename F>
  hstring &
  operator=(const S<F> &o)
  {
    if ( __mem::capacity < o.capacity )
      reserve(o.capacity + 1);
    micron::strcpy(__mem::memory, o.memory);
    __mem::length = o.length;

    return *this;
  }
  template <typename F>
  hstring &
  operator=(hstring<F> &&o)
  {
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
    ;
    return *this;
  }
  template <template <size_t, typename> class S, typename F, size_t N>
  hstring &
  operator=(const S<N, F> &o)
  {
    if ( __mem::capacity < o.length )
      reserve(o.length);
    clear();
    memcpy(__mem::memory, &o.memory[0], o.length);
    __mem::length = o.length;
    return *this;
  }
  template <size_t M, typename F = T>
  hstring &
  operator=(const F (&str)[M])
  {
    if ( __mem::capacity < M )
      reserve(M);
    clear();
    memcpy(&(__mem::memory)[0], &str[0], M);
    __mem::length = M;
    return *this;
  }
  ~hstring()
  {
    if ( __mem::memory == nullptr )
      return;
    this->destroy(to_chunk(__mem::memory, __mem::capacity * (sizeof(T) / sizeof(byte))));
  }
  chunk<byte>
  operator*()
  {
    return { reinterpret_cast<byte *>(__mem::memory), __mem::capacity };
  }
  inline bool
  operator!() const
  {
    return empty();
  }
  // overload this to always point to mem
  byte *
  operator&()
  {
    return reinterpret_cast<byte *>(__mem::memory);
  }
  const byte *
  operator&() const
  {
    return reinterpret_cast<const byte *>(__mem::memory);
  }
  template <typename C = T>
  void
  swap(hstring<C> &o)
  {
    auto tmp = contiguous_memory<C>(__mem::memory, __mem::length, __mem::capacity);
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = tmp.memory;
    o.length = tmp.length;
    o.capacity = tmp.capacity;
  }
  bool
  empty() const
  {
    return (bool)__mem::length == 0;
  };
  size_t
  size() const
  {
    return __mem::length;
  }
  void
  adjust_size()
  {
    auto ln = micron::strlen(__mem::memory);
    __mem::length = ln;
  }
  size_t
  max_size() const
  {
    return __mem::capacity;
  }
  // unsafe
  T *
  data()
  {
    return __mem::memory;
  };
  const auto &
  cdata() const
  {
    return __mem::memory;
  };

  inline sstring<256, T>
  stack(void) const
  {
    if ( __mem::size >= 255 )
      throw except::library_error("micron::hstring stack() out of memory.");
    return sstr<512, T>(c_str());
  };
  inline const char *
  c_str() const
  {
    if ( __mem::memory == nullptr )
      return _null_str;
    return reinterpret_cast<const char *>(&(__mem::memory)[0]);
  };
  inline const wide *
  w_str() const
  {
    if ( __mem::memory == nullptr )
      return _null_wstr;
    return reinterpret_cast<const wide *>(&(__mem::memory)[0]);
  };
  inline const unicode32 *
  uni_str() const
  {
    if ( __mem::memory == nullptr )
      return _null_u32str;
    return reinterpret_cast<const unicode32 *>(&(__mem::memory)[0]);
  };
  inline slice<byte>
  into_bytes()
  {
    return slice<byte>(reinterpret_cast<byte *>(&__mem::memory[0]),
                       reinterpret_cast<byte *>(&__mem::memory[__mem::length]));
  }
  inline auto
  clone()
  {
    return hstring<T>(*this);     // copy
  }
  template <typename F>
  inline F
  clone()
  {
    return F(*this);     // copy
  }
  inline T &
  front()
  {
    return __mem::memory[0];
  }
  inline const T &
  front() const
  {
    return __mem::memory[0];
  }
  inline T &
  back()
  {
    return __mem::memory[__mem::length - 1];
  }
  inline const T &
  back() const
  {
    return __mem::memory[__mem::length - 1];
  }
  // if used for buffering, adjust the length of the internals
  inline void
  _buf_set_length(const size_t s)
  {
    __mem::length = s;
  }
  template <typename F>
  size_t
  find(F ch, size_t pos = 0) const
  {
    for ( ; pos < __mem::length; pos++ ) {
      if ( (__mem::memory)[pos] == ch )
        return pos;
    }
    return npos;
  }
  template <typename F>
  size_t
  find(const hstring<F> &str, size_t pos = 0) const
  {
    return npos;
  }
  inline iterator
  begin()
  {
    return const_cast<T *>(&(__mem::memory)[0]);
  }
  inline iterator
  end()
  {
    return const_cast<T *>(&(__mem::memory)[__mem::length]);
  }
  inline iterator
  begin() const
  {
    return const_cast<T *>(&(__mem::memory)[0]);
  }
  inline iterator
  end() const
  {
    return const_cast<T *>(&(__mem::memory)[__mem::length]);
  }
  inline iterator
  last()
  {
    return const_cast<T *>(&(__mem::memory)[__mem::length - 1]);
  }
  inline iterator
  last() const
  {
    return const_cast<T *>(&(__mem::memory)[__mem::length - 1]);
  }
  inline const_iterator
  cbegin() const
  {
    return &(__mem::memory)[0];
  }
  inline const_iterator
  cend() const
  {
    return &(__mem::memory)[__mem::length];     // correct, should point at nptr
  }
  inline void
  clear()
  {
    zero(__mem::memory, __mem::capacity);
    __mem::length = 0;
  }
  inline hstring &
  append(const buffer &f, size_t n)
  {
    if ( (__mem::length + n) >= __mem::capacity )
      reserve(__mem::capacity + n);
    micron::memcpy(&(__mem::memory)[__mem::length],     // null is here so, overwrite it
                   &f[0], n);
    __mem::length += n;
    return *this;
  }
  template <typename F>
  inline hstring &
  append(const slice<F> &f, size_t n)
  {
    if ( (__mem::length + n) >= __mem::capacity )
      reserve(__mem::capacity + n);
    micron::memcpy(&(__mem::memory)[__mem::length],     // null is here so, overwrite it
                   &f[0], n);
    __mem::length += n;
    return *this;
  }
  template <typename F>
  inline hstring &
  append(const F *f, size_t n)
  {
    if ( (__mem::length + n) >= __mem::capacity )
      reserve(__mem::capacity + n);
    micron::memcpy(&(__mem::memory)[__mem::length],     // null is here so, overwrite it
                   f, n);
    __mem::length += n - 1;
    return *this;
  }
  template <typename F = T, size_t M>
  inline hstring &
  append(const F (&str)[M])
  {
    if ( (__mem::length + M) >= __mem::capacity )
      reserve(__mem::capacity + M);
    micron::memcpy(&(__mem::memory)[__mem::length],     // null is here so, overwrite it
                   &str[0], M);
    __mem::length += M - 1;
    return *this;
  }
  template <typename F = T>
  inline hstring &
  append(const hstring<F> &o)
  {
    if ( (__mem::length + o.length) >= __mem::capacity )
      reserve(__mem::capacity + o.length);
    micron::memcpy(&(__mem::memory)[__mem::length],     // null trunc
                   &(o.memory)[0], o.length);
    __mem::length += o.length;
    return *this;
  }
  template <size_t M, typename F = T>
  inline hstring &
  append(const sstring<M, F> &o)
  {
    size_t end = micron::strlen(o.c_str());
    if ( (__mem::length + end) >= __mem::capacity )
      reserve(__mem::capacity + end);
    micron::memcpy(&(__mem::memory)[__mem::length], &(o.memory)[0],
                   end);     // truncate null
    __mem::length += end;
    return *this;
  }

  template <typename F = T>
  inline hstring &
  push_back(F ch)
  {
    if ( (__mem::length) >= __mem::capacity )
      reserve(__mem::capacity + 1);
    (__mem::memory)[__mem::length++] = ch;
    return *this;
  }
  template <typename F = T, size_t M>
  inline hstring &
  push_back(const F (&str)[M])
  {
    if ( (__mem::length + M) >= __mem::capacity )
      reserve(__mem::capacity + M);
    micron::memcpy(&(__mem::memory)[__mem::length], &str[0], M);
    __mem::length += M - 1;
    return *this;
  }
  template <typename F = T>
  inline hstring &
  push_back(const hstring<F> &o)
  {
    if ( (__mem::length + o.length) >= __mem::capacity )
      reserve(__mem::capacity + o.length);
    micron::memcpy(&(__mem::memory)[__mem::length], &(o.memory)[0],
                   o.length);     // truncate null
    __mem::length += o.length;
    return *this;
  }
  template <size_t M, typename F = T>
  inline hstring &
  push_back(const sstring<M, F> &o)
  {
    size_t end = micron::strlen(o.c_str());
    if ( (__mem::length + end) >= __mem::capacity )
      reserve(__mem::capacity + end);
    micron::memcpy(&(__mem::memory)[__mem::length], &(o.memory)[0],
                   end);     // truncate null
    __mem::length += end - 1;
    return *this;
  }
  template <typename F = T>
  inline hstring &
  insert(size_t ind, F ch, size_t cnt = 1)
  {
    if ( __mem::length >= __mem::capacity or (__mem::length + cnt + 1) >= __mem::capacity )
      reserve(__mem::capacity + 1);
    char *buf = reinterpret_cast<char *>(malloc(cnt));
    micron::typeset<T>(&buf[0], ch, cnt);
    micron::bytemove(&(__mem::memory)[ind + cnt], &(__mem::memory)[ind], __mem::length - ind);
    micron::memcpy(&(__mem::memory)[ind], &buf[0], cnt);
    free(buf);
    __mem::length += cnt;
    return *this;
  }
  template <typename F = T, size_t M>
  inline hstring &
  insert(size_t ind, const F (&str)[M], size_t cnt = 1)
  {
    if ( __mem::length >= __mem::capacity or (__mem::length + (cnt * M)) >= __mem::capacity )
      reserve(__mem::capacity + 1);
    ssize_t str_len = strlen(str);
    char *buf = reinterpret_cast<char *>(malloc(cnt * str_len));
    for ( size_t i = 0; i < cnt; i++ )
      micron::memcpy(&buf[str_len * i], &str[0], str_len);
    micron::bytemove(&(__mem::memory)[ind + (cnt * str_len)], &(__mem::memory)[ind], __mem::length - ind);
    micron::memcpy(&(__mem::memory)[ind], &buf[0],

                   cnt * str_len);
    free(buf);
    __mem::length += (cnt * str_len);
    return *this;
  }

  template <typename F, size_t M>
  inline hstring &
  insert(size_t ind, const sstring<M, F> &o)
  {
    size_t end = micron::strlen(o.c_str());
    if ( __mem::length + end >= __mem::capacity ) {
      reserve(__mem::capacity + o.length);
    }
    micron::bytemove(&(__mem::memory)[ind + (end)], &(__mem::memory)[ind], __mem::length - ind);
    micron::memcpy(&(__mem::memory)[ind], &o.memory[0], end);

    __mem::length += end;
    return *this;
  }
  template <typename F = T>
  inline hstring &
  insert(iterator itr, F ch, size_t cnt = 1)
  {
    if ( __mem::length >= __mem::capacity or (__mem::length + cnt) >= __mem::capacity ) {
      size_t dif = itr - __mem::memory;
      reserve(__mem::capacity + cnt);
      itr = __mem::memory + dif;
    }
    char *buf = reinterpret_cast<char *>(malloc(cnt));
    micron::typeset<T>(&buf[0], ch, cnt);
    micron::bytemove(itr + cnt, itr,
                     __mem::length - (itr - &(__mem::memory)[0]));     // not null term
    micron::memcpy(itr, &buf[0], cnt);
    free(buf);
    __mem::length += cnt;
    return *this;
  }
  template <typename F = T, size_t M>
  inline hstring &
  insert(iterator itr, const F (&str)[M], size_t cnt = 1)
  {
    if ( __mem::length >= __mem::capacity or (__mem::length + (cnt * M)) >= __mem::capacity ) {
      size_t dif = itr - __mem::memory;
      reserve(__mem::capacity + M);
      itr = __mem::memory + dif;
    }

    char *buf = reinterpret_cast<char *>(malloc(cnt * (M - 1)));
    for ( size_t i = 0; i < cnt; i++ )
      micron::memcpy(&buf[(M)*i], &str[0], (M));
    micron::bytemove(itr + (cnt * (M)), itr, __mem::length - (itr - &(__mem::memory)[0]));
    micron::memcpy(itr, &buf[0], cnt * (M));
    free(buf);
    __mem::length += (cnt * (M));
    return *this;
  }
  template <typename F = T>
  inline hstring &
  insert(iterator itr, const hstring<F> &o)
  {
    if ( __mem::length + o.length >= __mem::capacity ) {
      size_t dif = itr - __mem::memory;
      reserve(__mem::capacity + o.length);
      itr = __mem::memory + dif;
    }
    micron::bytemove(itr + (o.length - 1), itr, __mem::length - (itr - &(__mem::memory)[0]));
    micron::memcpy(itr, &(o.memory)[0], o.length - 1);
    __mem::length += o.length - 1;
    return *this;
  }

  template <typename F, size_t M>
  inline hstring &
  insert(iterator itr, const sstring<M, F> &o)
  {
    size_t end = micron::strlen(o.c_str());
    if ( __mem::length + end >= __mem::capacity ) {
      size_t dif = itr - __mem::memory;
      reserve(__mem::capacity + o.length);
      itr = __mem::memory + dif;
    }
    micron::bytemove(itr + (end - 1), itr, __mem::length - (itr - &(__mem::memory)[0]));
    micron::memcpy(itr, &o.memory[0], end - 1);
    __mem::length += end - 1;
    return *this;
  }

  template <typename F, size_t M>
  inline hstring &
  insert(iterator itr, sstring<M, F> &&o)
  {
    if ( __mem::length + o.length >= __mem::capacity ) {
      size_t dif = itr - __mem::memory;
      reserve(__mem::capacity + o.length);
      itr = __mem::memory + dif;
    }
    micron::bytemove(itr + (o.length - 1), itr, __mem::length - (itr - &(__mem::memory)[0]));
    micron::memcpy(itr, &o.memory[0], o.length - 1);

    __mem::length += o.length - 1;
    micron::zero(o.memory, o.length);
    o.length = 0;
    return *this;
  }

  inline T &
  at(const size_t n)
  {
    if ( n >= __mem::length )
      throw except::library_error("micron:string at() out of range");
    return (__mem::memory)[n];
  };

  inline const T &
  at(const size_t n) const
  {
    if ( n >= __mem::length )
      throw except::library_error("micron:string at() out of range");
    return (__mem::memory)[n];
  };

  inline size_t
  at(const_iterator n)
  {
    if ( n - &__mem::memory[0] > 256 or n - &__mem::memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &__mem::memory[0];
  };

  inline size_t
  at(const_iterator n) const
  {
    if ( n - &__mem::memory[0] > 256 or n - &__mem::memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &__mem::memory[0];
  };
  inline size_t
  at(iterator n)
  {
    if ( n - &__mem::memory[0] > 256 or n - &__mem::memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &__mem::memory[0];
  };

  inline size_t
  at(iterator n) const
  {
    if ( n - &__mem::memory[0] > 256 or n - &__mem::memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &__mem::memory[0];
  };
  inline T &
  operator[](const size_t n)
  {
    return (__mem::memory)[n];
  };
  inline const T &
  operator[](const size_t n) const
  {
    return (__mem::memory)[n];
  };

  inline hstring &
  operator+=(const buffer &data)
  {
    if ( (__mem::length + data.size()) >= __mem::capacity )
      reserve(__mem::capacity + data.size() + 1);

    micron::memcpy(&(__mem::memory)[__mem::length], &data[0], data.size());
    __mem::length += data.size();
    return *this;
  };
  template <size_t M>
  inline hstring &
  operator+=(const char (&data)[M])
  {
    if ( (__mem::length + M) >= __mem::capacity )
      reserve(__mem::capacity + M + 1);

    size_t ln = __mem::length == 0 ? 0 : __mem::length;
    micron::memcpy(&(__mem::memory)[ln], &(data)[0], M);
    __mem::length += M - 1;
    return *this;
  };
  template <typename F = T>
  inline hstring &
  operator+=(const F *&data)
  {
    size_t end = micron::strlen(data);
    if ( (__mem::length + end) >= __mem::capacity )
      reserve(__mem::capacity + end + 1);

    micron::memcpy(&(__mem::memory)[__mem::length], &(data)[0], end);
    __mem::length += end;
    return *this;
  };
  template <typename F, size_t M>
  inline hstring &
  operator+=(const sstring<M, F> &data)
  {
    if ( (data.length + __mem::length) >= __mem::capacity ) [[unlikely]]
      reserve(__mem::capacity + data.length + 1);

    size_t end = micron::strlen(data.c_str());
    micron::memcpy(&(__mem::memory)[__mem::length], &(data.memory)[0], end);
    __mem::length += end;
    return *this;
  };
  template <typename F = T>
  inline hstring &
  operator+=(const hstring<F> &data)
  {
    if ( (data.length + __mem::length) >= __mem::capacity )
      reserve(__mem::capacity + data.length + 1);

    micron::memcpy(&(__mem::memory)[__mem::length], &(data.memory)[0], data.length);
    __mem::length += data.length;
    return *this;
  };

  template <typename F = T>
  inline hstring &
  operator+=(const slice<F> &data)
  {
    if ( (data.size() + __mem::length) >= __mem::capacity )
      reserve(__mem::capacity + data.size() + 1);

    micron::memcpy(&(__mem::memory)[__mem::length], &data[0], data.size() - 1);
    __mem::length += data.size() - 1;
    return *this;
  };
  template <typename F = T>
  inline hstring<F>
  substr(size_t pos = 0, size_t cnt = 0) const
  {
    if ( pos > __mem::length or (cnt + pos) > __mem::capacity )
      throw except::library_error("error micron::string substr invalid range.");
    hstring<F> buf(__mem::capacity);
    micron::memcpy(&buf[0], &__mem::memory[pos], cnt);
    buf[cnt] = '\0';
    return buf;
  };
  // grow container
  inline void
  reserve(size_t n)
  {
    if ( (n < __mem::capacity) ) {
      return;
    }
    // NOTE: this is here because there may be edge cases where hstring has been hard zero'ed out. salvage it in that
    // case
    if ( __mem::memory == nullptr ) [[unlikely]] {
      __mem::accept_new_memory(this->create((this->auto_size() >= sizeof(T) ? this->auto_size() : sizeof(T))));
      return;
    }
    __mem::accept_new_memory(
        this->grow(reinterpret_cast<byte *>(__mem::memory), __mem::capacity * sizeof(T), sizeof(T) * n));
  }
  inline void
  try_reserve(size_t n)
  {
    if ( (n < __mem::capacity) ) {
      throw except::memory_error("error micron::string was unable to allocate memory");
    }
    __mem::accept_new_memory(
        this->grow(reinterpret_cast<byte *>(__mem::memory), __mem::capacity * sizeof(T), sizeof(T) * n));
  }
  inline bool
  operator==(const char *data) const
  {
    size_t M = strlen(data);
    if ( M == __mem::length ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        if ( data[i] != __mem::memory[i] )
          return false;
    } else
      return false;
    return true;
  };
  template <typename F = T, size_t M>
  inline bool
  operator==(const F (&data)[M]) const
  {
    if ( M == __mem::length ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        if ( data[i] != __mem::memory[i] )
          return false;
    } else
      return false;
    return true;
  };
  template <typename F = T>
  inline bool
  operator==(const hstring<F> &data) const
  {
    if ( data.length == __mem::length ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        if ( data.memory[i] != __mem::memory[i] )
          return false;
    } else
      return false;
    return true;
  };

  inline bool
  operator==(const char *data)
  {
    size_t M = strlen(data);
    if ( M == __mem::length ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        if ( data[i] != __mem::memory[i] )
          return false;
    } else
      return false;
    return true;
  };
  template <typename F = T, size_t M>
  inline bool
  operator==(const F (&data)[M])
  {
    if ( M == __mem::length ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        if ( data[i] != __mem::memory[i] )
          return false;
    } else
      return false;
    return true;
  };
  template <typename F = T>
  inline bool
  operator==(const hstring<F> &data)
  {
    if ( data.length == __mem::length ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        if ( data.memory[i] != __mem::memory[i] )
          return false;
    } else
      return false;
    return true;
  };
};

};     // namespace micron
