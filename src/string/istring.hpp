//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"

#include "../algorithm/mem.hpp"
#include "../allocation/chunks.hpp"
#include "../allocator.hpp"
#include "../memory/memory.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"

#include "string.hpp"
#include "unitypes.hpp"

namespace micron
{
// null terminated

// immutable string on the heap, immutable
// accepts only char simple types
template <non_class T = schar, class Alloc = micron::allocator_serial<>>
class istring : private Alloc, public __immutable_memory_resource<T>
{
  using __mem = __immutable_memory_resource<T, Alloc>;
  istring
  __replicate()
  {
  }

public:
  using category_type = string_tag;
  using mutability_type = immutable_tag;
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
  ~istring()
  {
    if ( __mem::is_zero() )
      return;
    clear();
  }
  // __mem memory;
  constexpr istring() : __mem(Alloc::auto_size()) {};
  constexpr istring(const size_t n) : __mem(n) {};
  istring(size_t cnt, T ch) : __mem(cnt)
  {
    micron::typeset<T>(&__mem::memory[0], ch, cnt);
    __mem::length = cnt;
  }
  constexpr istring(const char *&str) : __mem(micron::strlen(str))
  {
    size_t end = micron::strlen(str);
    micron::memcpy(&(__mem::memory)[0], &str[0], end);     // - 1);
    __mem::length = end;                                   // - 1;
  };
  template <size_t M, typename F> constexpr istring(const F (&str)[M]) : __mem(M)
  {
    micron::bytecpy(&(__mem::memory)[0], &str[0], M * sizeof(F));     // - 1);
    __mem::length = M - 1;                                            // - 1;
  };
  constexpr istring(const istring &o) = delete;
  constexpr istring(istring &&o)
  {
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  }
  template <typename F> istring(istring<F> &&o)
  {
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  }
  istring &operator=(const istring &o) = delete;
  istring &
  operator=(istring &&o)
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
  template <typename F>
  istring &
  operator=(istring<F> &&o)
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
  byte *operator&() = delete;
  const byte *
  operator&() const
  {
    return reinterpret_cast<const byte *>(__mem::memory);
  }
  template <typename C = T>
  void
  swap(istring<C> &o)
  {
    auto tmp = immutable_memory<C>(__mem::memory, __mem::length, __mem::capacity);
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
      throw except::library_error("micron::istring stack() out of memory.");
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
  inline const slice<byte>
  into_bytes() const
  {
    return slice<byte>(reinterpret_cast<byte *>(&__mem::memory[0]),
                       reinterpret_cast<byte *>(&__mem::memory[__mem::length]));
  }
  inline const auto
  clone() const
  {
    return istring<T>(*this);     // copy
  }
  template <typename F>
  inline const F
  clone() const
  {
    return F(*this);     // copy
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
  find(const istring<F> &str, size_t pos = 0)
  {
    return npos;
  }
  inline const_iterator
  begin() const
  {
    return const_cast<T *>(&(__mem::memory)[0]);
  }
  inline const_iterator
  end() const
  {
    return const_cast<T *>(&(__mem::memory)[__mem::length]);
  }
  inline const_iterator
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
  inline istring
  append(const buffer &f, size_t n)
  {
    istring t(__mem::length + n);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length],     // null is here so, overwrite it
                   &f[0], n);
    t.length += (n);
    return t;
  }
  template <typename F>
  inline istring
  append(const slice<F> &f, size_t n)
  {
    istring t(__mem::length + n);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length],     // null is here so, overwrite it
                   &f[0], n);
    t.length += n;
    return *this;
  }
  template <typename F>
  inline istring
  append(const F *f, size_t n)
  {
    istring t(__mem::length + n);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length],     // null is here so, overwrite it
                   f, n);
    t.length += n - 1;
    return t;
  }
  template <typename F = T, size_t M>
  inline istring
  append(const F (&str)[M])
  {
    istring t(__mem::length + M);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length],     // null is here so, overwrite it
                   &str[0], M);
    t.length += (M - 1);
    return t;
  }
  template <typename F = T>
  inline istring
  append(const istring<F> &o)
  {
    istring t(__mem::length + o.size());
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length],     // null trunc
                   &(o.memory)[0], o.length);
    t.length += o.length;
    return t;
  }
  template <size_t M, typename F = T>
  inline istring
  append(const sstring<M, F> &o)
  {
    istring t(__mem::length + M);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    size_t end = micron::strlen(o.c_str());
    micron::memcpy(&(t.memory)[t.length], &(o.memory)[0],
                   end);     // truncate null
    t.length += end;
    return t;
  }

  template <typename F = T>
  inline istring
  push_back(F ch)
  {
    istring t(__mem::length + 1);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    (t.memory)[t.length++] = ch;
    return t;
  }
  template <typename F = T, size_t M>
  inline istring
  push_back(const F (&str)[M])
  {
    istring t(__mem::length + M);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length], &str[0], M);
    t.length += M - 1;
    return t;
  }
  template <typename F = T>
  inline istring
  push_back(const istring<F> &o)
  {
    istring t(__mem::length + o.size());
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length], &(o.memory)[0],
                   o.length);     // truncate null
    t.length += o.length;
    return t;
  }
  template <size_t M, typename F = T>
  inline istring
  push_back(const sstring<M, F> &o)
  {
    size_t end = micron::strlen(o.c_str());
    istring t(__mem::length + end);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length], &(o.memory)[0],
                   end);     // truncate null
    t.length += end - 1;
    return t;
  }
  template <typename F = T>
  inline istring
  insert(size_t ind, F ch, size_t cnt = 1)
  {
    istring t(__mem::length + cnt);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    char *buf = reinterpret_cast<char *>(malloc(cnt));
    micron::typeset<T>(&buf[0], ch, cnt);
    micron::bytemove(&(t.memory)[ind + cnt], &(t.memory)[ind], t.length - ind);
    micron::memcpy(&(t.memory)[ind], &buf[0], cnt);
    free(buf);
    t.length += cnt;
    return t;
  }
  template <typename F = T, size_t M>
  inline istring
  insert(size_t ind, const F (&str)[M], size_t cnt = 1)
  {
    istring t(__mem::length + (M * cnt));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    ssize_t str_len = strlen(str);
    char *buf = reinterpret_cast<char *>(malloc(cnt * str_len));
    for ( size_t i = 0; i < cnt; i++ )
      micron::memcpy(&buf[str_len * i], &str[0], str_len);
    micron::bytemove(&(t.memory)[ind + (cnt * str_len)], &(t.memory)[ind], t.length - ind);
    micron::memcpy(&(t.memory)[ind], &buf[0],

                   cnt * str_len);
    free(buf);
    t.length += (cnt * str_len);
    return t;
  }

  template <typename F, size_t M>
  inline istring
  insert(size_t ind, const sstring<M, F> &o)
  {
    size_t end = micron::strlen(o.c_str());
    istring t(__mem::length + (end));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::bytemove(&(t.memory)[ind + (end)], &(t.memory)[ind], t.length - ind);
    micron::memcpy(&(t.memory)[ind], &o.memory[0], end);

    t.length += end;
    return t;
  }
  inline const T &
  at(const size_t n) const
  {
    if ( n >= __mem::length )
      throw except::library_error("micron:string at() out of range");
    return (__mem::memory)[n];
  };
  inline size_t
  at(const_iterator n) const
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
  inline const T &
  operator[](const size_t n) const
  {
    return (__mem::memory)[n];
  };

  inline istring
  operator+=(const buffer &data)
  {
    istring t(__mem::length + (data.size()));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &data[0], data.size());
    t.length += data.size();
    return t;
  };
  template <size_t M>
  inline istring
  operator+=(const char (&data)[M])
  {
    istring t(__mem::length + M);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    size_t ln = __mem::length == 0 ? 0 : __mem::length - 1;
    micron::memcpy(&(t.memory)[ln], &(data)[0], M);
    t.length += M - 1;
    return t;
  };
  template <typename F = T>
  inline istring
  operator+=(const F *&data)
  {
    size_t end = micron::strlen(data);
    istring t(__mem::length + (end));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &(data)[0], end);
    t.length += end;
    return t;
  };
  template <typename F, size_t M>
  inline istring
  operator+=(const sstring<M, F> &data)
  {
    istring t(__mem::length + (data.size()));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &(data.memory)[0], data.size());
    t.length += data.length;
    return t;
  };
  template <typename F = T>
  inline istring
  operator+=(const istring<F> &data)
  {
    istring t(__mem::length + (data.size()));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &(data.memory)[0], data.size());
    t.length += data.length;
    return t;
  };

  template <typename F = T>
  inline istring
  operator+=(const slice<F> &data)
  {
    istring t(__mem::length + (data.size()));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &data[0], data.size() - 1);
    t.length += data.size() - 1;
    return t;
  };
  template <typename F = T>
  inline istring<F>
  substr(size_t pos = 0, size_t cnt = 0) const
  {
    if ( pos > __mem::length or (cnt + pos) > __mem::capacity )
      throw except::library_error("error micron::string substr invalid range.");
    istring<F> buf(__mem::capacity);
    micron::memcpy(&buf[0], &__mem::memory[pos], cnt);
    buf[cnt] = '\0';
    return buf;
  };
  // grow container
  inline void reserve(size_t n) = delete;

  inline void try_reserve(size_t n) = delete;
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
  operator==(const istring<F> &data) const
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
  operator==(const istring<F> &data)
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

template <is_string S>
auto
to_persist(const S &str)
{
  return istring<typename S::value_type>(str.c_str());
}

};     // namespace micron
