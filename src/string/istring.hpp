#pragma once

#include "../except.hpp"
#include <type_traits>

#include "../algorithm/mem.hpp"
#include "../allocation/chunks.hpp"
#include "../allocator.hpp"
#include "../memory/memory.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"

#include "string.h"
#include "unitypes.hpp"

namespace micron
{
// null terminated

// immutable string on the heap, immutable
// accepts only char simple types
template <non_class T = schar, class Alloc = micron::allocator_serial<>>
class istring : private Alloc, public immutable_memory<T>
{
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
    if ( immutable_memory<T>::memory == nullptr )
      return;
    this->destroy(to_chunk(immutable_memory<T>::memory, immutable_memory<T>::capacity * (sizeof(T) / sizeof(byte))));
  }
  // immutable_memory<T> memory;
  constexpr istring()
      : immutable_memory<T>(Alloc::create((Alloc::auto_size() >= sizeof(T) ? Alloc::auto_size() : sizeof(T)))) {};
  constexpr istring(const size_t n) : immutable_memory<T>(Alloc::create(n)) {};
  istring(size_t cnt, T ch) : immutable_memory<T>(Alloc::create(cnt < Alloc::auto_size() ? Alloc::auto_size() : cnt))
  {
    micron::typeset<T>(&immutable_memory<T>::memory[0], ch, cnt);
    immutable_memory<T>::length = cnt;
  }
  constexpr istring(const char *&str) : immutable_memory<T>(Alloc::create(micron::strlen(str)))
  {
    size_t end = micron::strlen(str);
    micron::memcpy(&(immutable_memory<T>::memory)[0], &str[0], end);     // - 1);
    immutable_memory<T>::length = end;                                   // - 1;
  };
  template <size_t M, typename F> constexpr istring(const F (&str)[M]) : immutable_memory<T>(Alloc::create(M))
  {
    micron::bytecpy(&(immutable_memory<T>::memory)[0], &str[0], M * sizeof(F));     // - 1);
    immutable_memory<T>::length = M - 1;                                            // - 1;
  };
  constexpr istring(const istring &o) = delete;
  constexpr istring(istring &&o)
  {
    immutable_memory<T>::memory = o.memory;
    immutable_memory<T>::length = o.length;
    immutable_memory<T>::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  }
  template <typename F> istring(istring<F> &&o)
  {
    immutable_memory<T>::memory = o.memory;
    immutable_memory<T>::length = o.length;
    immutable_memory<T>::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
  }
  istring &operator=(const istring &o) = delete;
  istring &
  operator=(istring &&o)
  {
    immutable_memory<T>::memory = o.memory;
    immutable_memory<T>::length = o.length;
    immutable_memory<T>::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
    ;
    return *this;
  }
  template <typename F>
  istring &
  operator=(istring<F> &&o)
  {
    immutable_memory<T>::memory = o.memory;
    immutable_memory<T>::length = o.length;
    immutable_memory<T>::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
    ;
    return *this;
  }
  template <template <size_t, typename> class S, typename F, size_t N>
  istring &
  operator=(const S<N, F> &o)     // okay
  {
    if ( immutable_memory<T>::capacity < o.length )
      reserve(o.length);
    clear();
    memcpy(immutable_memory<T>::memory, &o.memory[0], o.length);
    immutable_memory<T>::length = o.length;
    return *this;
  }
  template <size_t M, typename F = T>
  istring &
  operator=(const F (&str)[M])     // okay
  {
    if ( immutable_memory<T>::capacity < M )
      reserve(M);
    clear();
    memcpy(&(immutable_memory<T>::memory)[0], &str[0], M);
    immutable_memory<T>::length = M;
    return *this;
  }
  chunk<byte>
  operator*()
  {
    return { reinterpret_cast<byte *>(immutable_memory<T>::memory), immutable_memory<T>::capacity };
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
    return reinterpret_cast<const byte *>(immutable_memory<T>::memory);
  }
  template <typename C = T>
  void
  swap(istring<C> &o)
  {
    auto tmp
        = immutable_memory<C>(immutable_memory<T>::memory, immutable_memory<T>::length, immutable_memory<T>::capacity);
    immutable_memory<T>::memory = o.memory;
    immutable_memory<T>::length = o.length;
    immutable_memory<T>::capacity = o.capacity;
    o.memory = tmp.memory;
    o.length = tmp.length;
    o.capacity = tmp.capacity;
  }
  bool
  empty() const
  {
    return (bool)immutable_memory<T>::length == 0;
  };
  size_t
  size() const
  {
    return immutable_memory<T>::length;
  }
  size_t
  max_size() const
  {
    return immutable_memory<T>::capacity;
  }
  // unsafe
  T *
  data()
  {
    return immutable_memory<T>::memory;
  };
  const auto &
  cdata() const
  {
    return immutable_memory<T>::memory;
  };

  inline sstring<256, T>
  stack(void) const
  {
    if ( immutable_memory<T>::size >= 255 )
      throw except::library_error("micron::istring stack() out of memory.");
    return sstr<512, T>(c_str());
  };
  inline const char *
  c_str() const
  {
    if ( immutable_memory<T>::memory == nullptr )
      return _null_str;
    return reinterpret_cast<const char *>(&(immutable_memory<T>::memory)[0]);
  };
  inline const wide *
  w_str() const
  {
    if ( immutable_memory<T>::memory == nullptr )
      return _null_wstr;
    return reinterpret_cast<const wide *>(&(immutable_memory<T>::memory)[0]);
  };
  inline const unicode32 *
  uni_str() const
  {
    if ( immutable_memory<T>::memory == nullptr )
      return _null_u32str;
    return reinterpret_cast<const unicode32 *>(&(immutable_memory<T>::memory)[0]);
  };
  inline const slice<byte>
  into_bytes() const
  {
    return slice<byte>(reinterpret_cast<byte *>(&immutable_memory<T>::memory[0]),
                       reinterpret_cast<byte *>(&immutable_memory<T>::memory[immutable_memory<T>::length]));
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
    for ( ; pos < immutable_memory<T>::length; pos++ ) {
      if ( (immutable_memory<T>::memory)[pos] == ch )
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
    return const_cast<T *>(&(immutable_memory<T>::memory)[0]);
  }
  inline const_iterator
  end() const
  {
    return const_cast<T *>(&(immutable_memory<T>::memory)[immutable_memory<T>::length]);
  }
  inline const_iterator
  last() const
  {
    return const_cast<T *>(&(immutable_memory<T>::memory)[immutable_memory<T>::length - 1]);
  }
  inline const_iterator
  cbegin() const
  {
    return &(immutable_memory<T>::memory)[0];
  }
  inline const_iterator
  cend() const
  {
    return &(immutable_memory<T>::memory)[immutable_memory<T>::length];     // correct, should point at nptr
  }
  inline void
  clear()
  {
    zero(immutable_memory<T>::memory, immutable_memory<T>::capacity);
    immutable_memory<T>::length = 0;
  }
  inline istring
  append(const buffer &f, size_t n)
  {
    istring t(immutable_memory<T>::length + n);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
    micron::memcpy(&(t.memory)[t.length],     // null is here so, overwrite it
                   &f[0], n);
    t.length += (n);
    return t;
  }
  template <typename F>
  inline istring
  append(const slice<F> &f, size_t n)
  {
    istring t(immutable_memory<T>::length + n);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
    micron::memcpy(&(t.memory)[t.length],     // null is here so, overwrite it
                   &f[0], n);
    t.length += n;
    return *this;
  }
  template <typename F>
  inline istring
  append(const F *f, size_t n)
  {
    istring t(immutable_memory<T>::length + n);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
    micron::memcpy(&(t.memory)[t.length],     // null is here so, overwrite it
                   f, n);
    t.length += n - 1;
    return t;
  }
  template <typename F = T, size_t M>
  inline istring
  append(const F (&str)[M])
  {
    istring t(immutable_memory<T>::length + M);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
    micron::memcpy(&(t.memory)[t.length],     // null is here so, overwrite it
                   &str[0], M);
    t.length += (M - 1);
    return t;
  }
  template <typename F = T>
  inline istring
  append(const istring<F> &o)
  {
    istring t(immutable_memory<T>::length + o.size());
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
    micron::memcpy(&(t.memory)[t.length],     // null trunc
                   &(o.memory)[0], o.length);
    t.length += o.length;
    return t;
  }
  template <size_t M, typename F = T>
  inline istring
  append(const sstring<M, F> &o)
  {
    istring t(immutable_memory<T>::length + M);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
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
    istring t(immutable_memory<T>::length + 1);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
    (t.memory)[t.length++] = ch;
    return t;
  }
  template <typename F = T, size_t M>
  inline istring
  push_back(const F (&str)[M])
  {
    istring t(immutable_memory<T>::length + M);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
    micron::memcpy(&(t.memory)[t.length], &str[0], M);
    t.length += M - 1;
    return t;
  }
  template <typename F = T>
  inline istring
  push_back(const istring<F> &o)
  {
    istring t(immutable_memory<T>::length + o.size());
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
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
    istring t(immutable_memory<T>::length + end);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
    micron::memcpy(&(t.memory)[t.length], &(o.memory)[0],
                   end);     // truncate null
    t.length += end - 1;
    return t;
  }
  template <typename F = T>
  inline istring
  insert(size_t ind, F ch, size_t cnt = 1)
  {
    istring t(immutable_memory<T>::length + cnt);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
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
    istring t(immutable_memory<T>::length + (M * cnt));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
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
    istring t(immutable_memory<T>::length + (end));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;
    micron::bytemove(&(t.memory)[ind + (end)], &(t.memory)[ind], t.length - ind);
    micron::memcpy(&(t.memory)[ind], &o.memory[0], end);

    t.length += end;
    return t;
  }
  inline const T &
  at(const size_t n) const
  {
    if ( n >= immutable_memory<T>::length )
      throw except::library_error("micron:string at() out of range");
    return (immutable_memory<T>::memory)[n];
  };
  inline size_t
  at(const_iterator n) const
  {
    if ( n - &immutable_memory<T>::memory[0] > 256 or n - &immutable_memory<T>::memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &immutable_memory<T>::memory[0];
  };

  inline size_t
  at(iterator n) const
  {
    if ( n - &immutable_memory<T>::memory[0] > 256 or n - &immutable_memory<T>::memory[0] < 0 )
      throw except::library_error("micron:sstring at() out of range");
    return n - &immutable_memory<T>::memory[0];
  };
  inline const T &
  operator[](const size_t n) const
  {
    return (immutable_memory<T>::memory)[n];
  };

  inline istring
  operator+=(const buffer &data)
  {
    istring t(immutable_memory<T>::length + (data.size()));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;

    micron::memcpy(&(t.memory)[t.length], &data[0], data.size());
    t.length += data.size();
    return t;
  };
  template <size_t M>
  inline istring
  operator+=(const char (&data)[M])
  {
    istring t(immutable_memory<T>::length + M);
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;

    size_t ln = immutable_memory<T>::length == 0 ? 0 : immutable_memory<T>::length - 1;
    micron::memcpy(&(t.memory)[ln], &(data)[0], M);
    t.length += M - 1;
    return t;
  };
  template <typename F = T>
  inline istring
  operator+=(const F *&data)
  {
    size_t end = micron::strlen(data);
    istring t(immutable_memory<T>::length + (end));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;

    micron::memcpy(&(t.memory)[t.length], &(data)[0], end);
    t.length += end;
    return t;
  };
  template <typename F, size_t M>
  inline istring
  operator+=(const sstring<M, F> &data)
  {
    istring t(immutable_memory<T>::length + (data.size()));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;

    micron::memcpy(&(t.memory)[t.length], &(data.memory)[0], data.size());
    t.length += data.length;
    return t;
  };
  template <typename F = T>
  inline istring
  operator+=(const istring<F> &data)
  {
    istring t(immutable_memory<T>::length + (data.size()));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;

    micron::memcpy(&(t.memory)[t.length], &(data.memory)[0], data.size());
    t.length += data.length;
    return t;
  };

  template <typename F = T>
  inline istring
  operator+=(const slice<F> &data)
  {
    istring t(immutable_memory<T>::length + (data.size()));
    micron::memcpy(&(t.memory)[0],     // null is here so, overwrite it
                   &immutable_memory<T>::memory[0], immutable_memory<T>::length);
    t.length += immutable_memory<T>::length;

    micron::memcpy(&(t.memory)[t.length], &data[0], data.size() - 1);
    t.length += data.size() - 1;
    return t;
  };
  template <typename F = T>
  inline istring<F>
  substr(size_t pos = 0, size_t cnt = 0) const
  {
    if ( pos > immutable_memory<T>::length or (cnt + pos) > immutable_memory<T>::capacity )
      throw except::library_error("error micron::string substr invalid range.");
    istring<F> buf(immutable_memory<T>::capacity);
    micron::memcpy(&buf[0], &immutable_memory<T>::memory[pos], cnt);
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
    if ( M == immutable_memory<T>::length ) {
      for ( size_t i = 0; i < immutable_memory<T>::length; i++ )
        if ( data[i] != immutable_memory<T>::memory[i] )
          return false;
    } else
      return false;
    return true;
  };
  template <typename F = T, size_t M>
  inline bool
  operator==(const F (&data)[M]) const
  {
    if ( M == immutable_memory<T>::length ) {
      for ( size_t i = 0; i < immutable_memory<T>::length; i++ )
        if ( data[i] != immutable_memory<T>::memory[i] )
          return false;
    } else
      return false;
    return true;
  };
  template <typename F = T>
  inline bool
  operator==(const istring<F> &data) const
  {
    if ( data.length == immutable_memory<T>::length ) {
      for ( size_t i = 0; i < immutable_memory<T>::length; i++ )
        if ( data.memory[i] != immutable_memory<T>::memory[i] )
          return false;
    } else
      return false;
    return true;
  };

  inline bool
  operator==(const char *data)
  {
    size_t M = strlen(data);
    if ( M == immutable_memory<T>::length ) {
      for ( size_t i = 0; i < immutable_memory<T>::length; i++ )
        if ( data[i] != immutable_memory<T>::memory[i] )
          return false;
    } else
      return false;
    return true;
  };
  template <typename F = T, size_t M>
  inline bool
  operator==(const F (&data)[M])
  {
    if ( M == immutable_memory<T>::length ) {
      for ( size_t i = 0; i < immutable_memory<T>::length; i++ )
        if ( data[i] != immutable_memory<T>::memory[i] )
          return false;
    } else
      return false;
    return true;
  };
  template <typename F = T>
  inline bool
  operator==(const istring<F> &data)
  {
    if ( data.length == immutable_memory<T>::length ) {
      for ( size_t i = 0; i < immutable_memory<T>::length; i++ )
        if ( data.memory[i] != immutable_memory<T>::memory[i] )
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
