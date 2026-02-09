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

#include "sstring.hpp"

// TODO: very messy, rewrite this

namespace micron
{
// null terminated

constexpr const char _null_str[1] = "";
constexpr const wide _null_wstr[1] = L"";
constexpr const unicode32 _null_u32str[1] = U"";

// string on the heap, mutable, standard replacement of std::string
// accepts only char simple types
template <integral T = schar, class Alloc = micron::allocator_serial<>>
class hstring : private Alloc, public __mutable_memory_resource<T>
{
  using __mem = __mutable_memory_resource<T, Alloc>;

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
  ~hstring()
  {
    if ( __mem::is_zero() )
      return;
    clear();
  }
  // contiguous_memory<T> memory;
  constexpr hstring() : __mem(Alloc::auto_size()) {};
  constexpr hstring(const size_t n) : __mem(n) {};
  hstring(size_t cnt, T ch) : __mem(cnt)
  {
    micron::typeset<T>(&__mem::memory[0], ch, cnt);
    __mem::length = cnt;
  }
  constexpr hstring(const char *str) : __mem(micron::strlen(str))
  {
    size_t end = micron::strlen(str);
    micron::memcpy(&(__mem::memory)[0], &str[0], end);     // - 1);
    __mem::length = end;                                   // - 1;
  };
  template <size_t M, typename F> constexpr hstring(const F (&str)[M]) : __mem((M))
  {
    micron::bytecpy(&(__mem::memory)[0], &str[0], M * sizeof(F));     // - 1);
    __mem::length = M - 1;                                            // - 1;
  };
  constexpr hstring(const hstring &o) : __mem(o.capacity)
  {
    micron::strcpy(__mem::memory, o.memory);
    __mem::length = o.length;
  };
  template <typename F>
  constexpr hstring(hstring<F> &&o)
  {
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  }
  constexpr hstring(hstring &&o)
  {
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  }
  template <typename F> constexpr hstring(const hstring<F> &o) : __mem(o.capacity)
  {
    micron::strcpy(__mem::memory, o.memory);
    __mem::length = o.length;
  };
  template <size_t N, typename F> constexpr hstring(const sstring<N, F> &o) : __mem(o.length)
  {
    micron::memcpy(&(__mem::memory)[0], &o.memory[0],
           o.length);             // - 1);
    __mem::length = o.length;     // - 1; // no null
  };
  // allow construction from - to iterator (be careful!)
  constexpr hstring(iterator __start, iterator __end)
      : __mem((ssize_t)Alloc::auto_size() < (__end - __start) ? (__end - __start) : Alloc::auto_size())
  {
    micron::memcpy(__mem::memory, __start, __end - __start);
    __mem::length = __end - __start;
  };
  constexpr hstring(const_iterator __start, const_iterator __end)
      : __mem(Alloc::auto_size() < (__end - __start) ? (__end - __start) : Alloc::auto_size())
  {
    micron::memcpy(__mem::memory, __start, __end - __start);
    __mem::length = __end - __start;
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
    if ( __mem::memory ) {
      // kill old memory first
      __mem::free();
    }
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
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
    if ( __mem::memory ) {
      // kill old memory first
      __mem::free();
    }
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
    micron::memcpy(__mem::memory, &o.memory[0], o.length);
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
    micron::memcpy(&(__mem::memory)[0], &str[0], M);
    __mem::length = M;
    return *this;
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
  /* TODO
   * template <typename C = T>
  hstring<T> &
  swap(hstring<C> &o)
  {
    __mem::swap(o);
    return *this;
  }*/
  bool
  empty() const
  {
    return (__mem::length == 0 or __mem::memory == nullptr);
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
      if ( __mem::memory[pos] == ch )
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
    return const_cast<iterator>(&(__mem::memory)[0]);
  }
  inline iterator
  end()
  {
    return const_cast<iterator>(&(__mem::memory)[__mem::length]);
  }
  inline iterator
  begin() const
  {
    return const_cast<iterator>(&(__mem::memory)[0]);
  }
  inline iterator
  end() const
  {
    return const_cast<iterator>(&(__mem::memory)[__mem::length]);
  }
  inline iterator
  last()
  {
    return const_cast<iterator>(&(__mem::memory)[__mem::length - 1]);
  }
  inline iterator
  last() const
  {
    return const_cast<iterator>(&(__mem::memory)[__mem::length - 1]);
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
  // doesn't zero memory, just resets the cur. pointer to 0
  inline void
  fast_clear()
  {
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
  inline hstring &
  pop_back(void)
  {
    if ( (__mem::length) > 0 )
      (__mem::memory)[__mem::length--] = 0x0;
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
    if ( __mem::length + cnt >= __mem::capacity )
      reserve(__mem::capacity + 1);
    micron::bytemove(&__mem::memory[ind + cnt], &__mem::memory[ind], __mem::length - ind);
    micron::memset(&__mem::memory[ind], ch, cnt);
    __mem::length += cnt;
    return *this;
  }
  template <typename F = T, size_t M>
  inline hstring &
  insert(size_t ind, const F (&str)[M], size_t cnt = 1)
  {
    if ( __mem::length >= __mem::capacity or (__mem::length + (cnt * M)) >= __mem::capacity )
      reserve(__mem::capacity + 1);
    size_t str_len = M - 1;     // strlen(str);

    micron::bytemove(&__mem::memory[ind + cnt * str_len], &__mem::memory[ind], __mem::length - ind);

    for ( size_t i = 0; i < cnt; ++i )
      micron::memcpy(&__mem::memory[ind + i * str_len], str, str_len);
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

    if ( itr < __mem::memory || itr > __mem::memory + __mem::length )
      throw except::library_error("micron:string string() out of range");

    micron::bytemove(itr + cnt, itr, __mem::length - (itr - __mem::memory));
    micron::memset(itr, ch, cnt);
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
    ssize_t str_len = M - 1;     // strlen(str);

    size_t tail_len = __mem::length - (itr - __mem::memory);
    micron::bytemove(itr + cnt * str_len, itr, tail_len);
    for ( size_t i = 0; i < cnt; ++i )
      micron::memcpy(itr + i * str_len, str, str_len);
    __mem::length += (cnt * str_len);
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

  template <typename Itr>
    requires(micron::same_as<Itr, const_iterator>)
  inline size_t
  at(Itr n)
  {
    if ( n - &__mem::memory[0] > 256 or n - &__mem::memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &__mem::memory[0];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, const_iterator>)
  inline size_t
  at(Itr n) const
  {
    if ( n - &__mem::memory[0] > 256 or n - &__mem::memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &__mem::memory[0];
  };
  template <typename Itr>
    requires(micron::same_as<Itr, iterator>)
  inline size_t
  at(Itr n)
  {
    if ( n - &__mem::memory[0] > 256 or n - &__mem::memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &__mem::memory[0];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, iterator>)
  inline size_t
  at(Itr n) const
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
  /*template <size_t M>
  inline hstring &
  operator+=(const char (&data)[M])
  {
    if ( (__mem::length + M) >= __mem::capacity )
      reserve(__mem::capacity + M + 1);

    size_t ln = __mem::length == 0 ? 0 : __mem::length;
    micron::memcpy(&(__mem::memory)[ln], &(data)[0], M);
    __mem::length += M - 1;
    return *this;
  };*/
  template <typename F = T>
  inline hstring &
  operator+=(const F *data)
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
  inline hstring &
  operator+=(const T d)
  {
    if ( (__mem::length + 1) >= __mem::capacity )
      reserve(__mem::capacity + 1);

    size_t ln = __mem::length == 0 ? 0 : __mem::length;
    __mem::memory[ln] = d;
    __mem::length++;
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
    hstring<F> buf(cnt + 1);
    micron::memcpy(&buf[0], &__mem::memory[pos], cnt);
    buf[cnt] = '\0';
    buf._buf_set_length(cnt + 1);
    return buf;
  };
  template <typename F = T>
  inline hstring<F>
  substr(iterator _start, iterator _end) const
  {
    if ( _start < begin() or _end >= end() )
      throw except::library_error("error micron::string substr invalid range.");
    hstring<F> buf(_end - _start + 1);
    micron::memcpy(&buf[0], _start, _end - _start);
    buf._buf_set_length(_end - _start + 1);
    *buf.last() = '\0';
    return buf;
  };
  template <typename F = T>
  inline hstring<F>
  substr(const_iterator _start, const_iterator _end) const
  {
    if ( _start < begin() or _end >= end() )
      throw except::library_error("error micron::string substr invalid range.");
    hstring<F> buf(_end - _start + 1);
    micron::memcpy(&buf[0], _start, _end - _start);
    buf._buf_set_length(_end - _start + 1);
    *buf.last() = '\0';
    return buf;
  };
  // grow container
  void
  reserve(size_t n)
  {
    if ( (n < __mem::capacity) ) {
      return;
    }
    // NOTE: this is here because there may be edge cases where hstring has been hard zero'ed out. salvage it in that
    // case
    if ( __mem::memory == nullptr ) [[unlikely]] {
      __mem::realloc(Alloc::auto_size());
      return;
    }
    __mem::expand(n);
  }
  void
  try_reserve(size_t n)
  {
    if ( (n < __mem::capacity) ) {
      throw except::memory_error("error micron::string was unable to allocate memory");
    }
    __mem::expand(n);
  }
  void
  resize(size_t n, const T ch)
  {
    if ( !(n > __mem::length) ) {
      return;
    }
    if ( n >= __mem::capacity ) {
      reserve(n);
    }
    micron::memset(&__mem::memory[__mem::length], ch, n);
    __mem::length = n;
  }
  template <typename F = T, typename I = size_t>
    requires micron::is_arithmetic_v<I>
  inline hstring<F> &
  erase(I ind, size_t cnt = 1)
  {
    if ( ind > __mem::length || cnt > __mem::length - ind )
      throw except::library_error("micron:hstring erase() out of range");
    if ( !cnt )
      return *this;
    micron::bytemove(&__mem::memory[ind], &__mem::memory[ind + (1 + (cnt - 1))], __mem::length - (ind + 1 + (cnt - 1)));
    micron::typeset<T>(&__mem::memory[__mem::length - (cnt)], 0x0, cnt);
    __mem::length -= cnt;
    return *this;
  }

  template <typename F = T>
  inline hstring<F> &
  erase(iterator itr, size_t cnt = 1)
  {
    // TODO: signedness
    if ( itr < begin() or itr > end() or cnt > (end() - itr) )
      throw except::library_error("micron:hstring erase() out of range");
    if ( !cnt )
      return *this;
    // micron::memmove(itr - cnt + 1, itr, __mem::length - (&__mem::memory[0] - itr - 1));
    micron::bytemove(itr, itr + (1 + (cnt - 1)), __mem::length - ((itr - &__mem::memory[0]) + 1 + (cnt - 1)));
    micron::typeset<T>(&__mem::memory[__mem::length - cnt], 0x0, cnt);
    __mem::length -= cnt;
    return *this;
  }
  template <typename F = T>
  inline hstring<F> &
  erase(const_iterator itr, size_t cnt = 1)
  {
    if ( itr < begin() or itr > end() or cnt > (end() - itr) )
      throw except::library_error("micron:hstring erase() out of range");
    if ( !cnt )
      return *this;
    micron::bytemove(itr, itr + (1 + (cnt - 1)), __mem::length - ((itr - &__mem::memory[0]) + 1 + (cnt - 1)));
    micron::typeset<T>(&__mem::memory[__mem::length - cnt], 0x0, cnt);
    __mem::length -= cnt;
    return *this;
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
