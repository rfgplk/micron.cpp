//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"

#include "../algorithm/memory.hpp"
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
template<is_scalar_literal T = schar, class Alloc = micron::allocator_small<>>
class istring: private Alloc, public __immutable_memory_resource<T, Alloc>
{
  using __mem = __immutable_memory_resource<T, Alloc>;

  istring
  __replicate()
  {
  }

  static inline constexpr __attribute__((always_inline)) usize
  __alloc_size(usize n) noexcept
  {
    return n == 0 ? Alloc::auto_size() : n;
  }

  struct __bspan {
    const byte *p;
    usize n;
  };

  template<has_cstr S>
  static inline __bspan
  __as_key(const S &s) noexcept
  {
    return { reinterpret_cast<const byte *>(s.c_str()), s.size() * sizeof(typename S::value_type) };
  }

  static inline __bspan
  __as_key(const char *s) noexcept
  {
    return { reinterpret_cast<const byte *>(s), micron::strlen(s) };
  }

  template<usize M>
  static inline __bspan
  __as_key(const T (&s)[M]) noexcept
  {
    return { reinterpret_cast<const byte *>(&s[0]), (M - 1) * sizeof(T) };
  }

  struct __needle {
    const T *p;
    usize n;
  };

  // needle builders treat n as a T-element count and reinterpret the source as const T*
  // this is ONLY sound when the source width MATCHES T
  template<has_cstr S>
  static inline __needle
  __as_needle(const S &s) noexcept
  {
    return { reinterpret_cast<const T *>(s.c_str()), s.size() };
  }

  static inline __needle
  __as_needle(const char *s) noexcept
  {
    return { reinterpret_cast<const T *>(s), micron::strlen(s) };
  }

  template<usize M>
  static inline __needle
  __as_needle(const T (&s)[M]) noexcept
  {
    return { &s[0], M - 1 };
  }

  template<typename R>
  static constexpr bool __same_width_cstr = []() constexpr {
    if constexpr ( has_cstr<R> )
      return sizeof(typename R::value_type) == sizeof(T);
    else
      return false;
  }();

  // a needle source R is memory-safe iff it is a T-array of any extent OR it is a char/byte pointer AND T is itself byte-wide OR a
  // same-width has_cstr
  template<typename R>
  static constexpr bool __valid_needle
      = (micron::is_array<R>::value && micron::is_same_v<micron::remove_cv_t<micron::remove_extent_t<R>>, T>)
        || (micron::is_pointer_v<micron::decay_t<R>>
            && micron::is_same_v<micron::remove_cv_t<micron::remove_pointer_t<micron::decay_t<R>>>, char> && sizeof(T) == 1)
        || __same_width_cstr<R>;

  inline istring
  __dup() const
  {
    istring t(__mem::length + 1);
    if ( __mem::length ) micron::memcpy(&t.memory[0], &__mem::memory[0], __mem::length);
    t.memory[__mem::length] = T{ 0 };
    t.length = __mem::length;
    return t;
  }

  inline void
  _buf_set_length(const usize s)
  {
    __mem::length = s;
    // must maintain trailing NUL invariantly
    if ( __mem::memory && s < __mem::capacity ) __mem::memory[s] = T{ 0 };
  }

public:
  using category_type = string_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef usize size_type;
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
    if ( __mem::is_zero() ) return;
    clear();
  }

  // __mem memory;
  constexpr istring() : __mem(Alloc::auto_size())
  {
    // empty string: keep c_str()/w_str() NUL-terminated even on a recycled (non-zeroed) page
    if ( __mem::memory ) __mem::memory[0] = T{ 0 };
  };

  constexpr istring(const usize n) : __mem(__alloc_size(n))
  {
    if ( __mem::memory ) __mem::memory[0] = T{ 0 };
  };

  istring(usize cnt, T ch) : __mem(__alloc_size(cnt + 1))      // +1 for the trailing NUL
  {
    micron::typeset<T>(&__mem::memory[0], ch, cnt);
    _buf_set_length(cnt);
  }

  constexpr istring(const char *&str) : __mem(__alloc_size(micron::strlen(str)))
  {
    usize end = micron::strlen(str);
    micron::memcpy(&(__mem::memory)[0], &str[0], end);      // - 1);
    __mem::length = end;                                    // - 1;
  };

  template<usize M, typename F> constexpr istring(const F (&str)[M]) : __mem(__alloc_size(M))
  {
    // WARNING: do NOT blind-bytecpy M*sizeof(F) bytes into an M*sizeof(T)-ELEMENT buffer:
    // for sizeof(F) != sizeof(T) that is a cross-width OOB write
    usize end = 0;
    while ( end < M - 1 && !(str[end] == F{ 0 }) ) ++end;
    micron::memcpy(&(__mem::memory)[0], &str[0], end);
    _buf_set_length(end);
  };

  constexpr istring(const istring &o) = delete;

  constexpr istring(istring &&o) : __mem(micron::move(o)) { }

  template<typename F> istring(istring<F> &&o) : __mem(nullptr)      // nullptr ctor: no spurious 512B alloc/free
  {
    if ( __mem::memory ) __mem::free();
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

  template<typename F>
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

  template<typename C = T>
    requires(micron::same_as<C, T>)
  void
  swap(istring<C> &o)
  {
    T *tmp_mem = __mem::memory;
    usize tmp_len = __mem::length;
    usize tmp_cap = __mem::capacity;
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = tmp_mem;
    o.length = tmp_len;
    o.capacity = tmp_cap;
  }

  bool
  empty() const
  {
    return (bool)__mem::length == 0;
  };

  usize
  size() const
  {
    return __mem::length;
  }

  usize
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
    if ( __mem::length >= 255 ) exc<except::library_error>("micron::istring stack() out of memory.");
    return sstring<256, T>(c_str());
  };

  inline const char *
  c_str() const
  {
    if ( __mem::memory == nullptr ) return _null_str;
    return reinterpret_cast<const char *>(&(__mem::memory)[0]);
  };

  inline const wide *
  w_str() const
  {
    if ( __mem::memory == nullptr ) return _null_wstr;
    return reinterpret_cast<const wide *>(&(__mem::memory)[0]);
  };

  inline const unicode32 *
  uni_str() const
  {
    if ( __mem::memory == nullptr ) return _null_u32str;
    return reinterpret_cast<const unicode32 *>(&(__mem::memory)[0]);
  };

  inline const slice<byte>
  into_bytes() const
  {
    return slice<byte>(reinterpret_cast<byte *>(&__mem::memory[0]), reinterpret_cast<byte *>(&__mem::memory[__mem::length]));
  }

  inline const auto
  clone() const
  {
    return istring<T>(*this);      // copy
  }

  template<typename F>
  inline const F
  clone() const
  {
    return F(*this);      // copy
  }

  template<typename F>
  usize
  find(F ch, usize pos = 0) const
  {
    if ( pos >= __mem::length ) return npos;
    const usize len = __mem::length - pos;
    usize r;
    if constexpr ( sizeof(T) == 1 ) {
#if defined(__micron_x86_avx2)
      r = micron::simd::find_first_set_256(__mem::memory + pos, len, static_cast<char>(ch));
#else
      r = micron::simd::find_first_set_128(__mem::memory + pos, len, static_cast<char>(ch));
#endif
    } else {
      r = micron::simd::find_first_elem<T>(__mem::memory + pos, len, static_cast<T>(ch));
    }
    return r == len ? npos : pos + r;
  }

  template<typename F>
    requires(sizeof(F) == sizeof(T))
  usize
  find(const istring<F> &str, usize pos = 0) const
  {
    if ( str.empty() ) return pos;
    if ( pos >= __mem::length ) return npos;
    if ( str.size() > __mem::length - pos ) return npos;
    if constexpr ( sizeof(T) != 1 ) {
      const usize rr = micron::simd::find_substr_elem<T>(__mem::memory + pos, __mem::length - pos, reinterpret_cast<const T *>(str.cdata()),
                                                         str.size());
      return rr == (__mem::length - pos) ? npos : pos + rr;
    }
    auto *r = micron::memmem<byte>(reinterpret_cast<const byte *>(__mem::memory + pos), __mem::length - pos,
                                   reinterpret_cast<const byte *>(str.cdata()), str.size());
    return r == nullptr ? npos : static_cast<usize>(reinterpret_cast<const T *>(r) - __mem::memory);
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
    return &(__mem::memory)[__mem::length];      // correct, should point at nptr
  }

  inline void
  clear()
  {
    if ( __mem::is_zero() ) {
      __mem::length = 0;
      return;
    }
    zero(__mem::memory, __mem::capacity);
    __mem::length = 0;
  }

  // doesn't zero memory, just resets the cur. pointer to 0
  inline void
  fast_clear()
  {
    __mem::length = 0;
  }

  inline istring
  append(const buffer &f, usize n)
  {
    istring t(__mem::length + n + 1);      // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],         // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length],      // null is here so, overwrite it
                   &f[0], n);
    t._buf_set_length(t.length + n);
    return t;
  }

  template<typename F>
  inline istring
  append(const slice<F> &f, usize n)
  {
    istring t(__mem::length + n + 1);      // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],         // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length],      // null is here so, overwrite it
                   &f[0], n);
    t._buf_set_length(t.length + n);
    return t;
  }

  template<typename F>
  inline istring
  append(const F *f, usize n)
  {
    istring t(__mem::length + n + 1);      // +1 for the NUL terminator
    micron::memcpy(&(t.memory)[0],         // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length],      // null is here so, overwrite it
                   f, n);
    t._buf_set_length(t.length + n);
    return t;
  }

  template<typename F = T, usize M>
  inline istring
  append(const F (&str)[M])
  {
    istring t(__mem::length + M);       // length + (M-1) content + 1 NUL slot
    micron::memcpy(&(t.memory)[0],      // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length],      // null is here so, overwrite it
                   &str[0], M - 1);
    t._buf_set_length(t.length + (M - 1));
    return t;
  }

  template<typename F = T>
  inline istring
  append(const istring<F> &o)
  {
    istring t(__mem::length + o.size() + 1);      // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],                // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length],      // null trunc
                   &(o.memory)[0], o.length);
    t._buf_set_length(t.length + o.length);
    return t;
  }

  template<usize M, typename F = T>
  inline istring
  append(const sstring<M, F> &o)
  {
    istring t(__mem::length + M);       // M includes the NUL slot, so length+(end<=M-1)+1 fits
    micron::memcpy(&(t.memory)[0],      // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    usize end = micron::strlen(o.c_str());
    micron::memcpy(&(t.memory)[t.length], &(o.memory)[0],
                   end);      // truncate null
    t._buf_set_length(t.length + end);
    return t;
  }

  template<typename F = T>
  inline istring
  push_back(F ch)
  {
    istring t(__mem::length + 2);       // +1 char +1 NUL
    micron::memcpy(&(t.memory)[0],      // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    (t.memory)[t.length] = ch;
    t._buf_set_length(t.length + 1);
    return t;
  }

  template<typename F = T, usize M>
  inline istring
  push_back(const F (&str)[M])
  {
    istring t(__mem::length + M);       // length + (M-1) content + 1 NUL slot
    micron::memcpy(&(t.memory)[0],      // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length], &str[0], M - 1);
    t._buf_set_length(t.length + (M - 1));
    return t;
  }

  template<typename F = T>
  inline istring
  push_back(const istring<F> &o)
  {
    istring t(__mem::length + o.size() + 1);      // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],                // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length], &(o.memory)[0],
                   o.length);      // truncate null
    t._buf_set_length(t.length + o.length);
    return t;
  }

  template<usize M, typename F = T>
  inline istring
  push_back(const sstring<M, F> &o)
  {
    usize end = micron::strlen(o.c_str());      // excludes the NUL already
    istring t(__mem::length + end + 1);         // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],              // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    micron::memcpy(&(t.memory)[t.length], &(o.memory)[0],
                   end);                    // truncate null
    t._buf_set_length(t.length + end);      // was 'end - 1' -> underflow on empty + dropped last char
    return t;
  }

  template<typename F = T>
  inline istring
  insert(usize ind, F ch, usize cnt = 1)
  {
    if ( ind > __mem::length ) exc<except::library_error>("micron::istring insert() out of range");
    istring t(__mem::length + cnt + 1);      // +1 for the trailing NUL

    micron::memcpy(t.memory, __mem::memory, ind);
    micron::typeset<T>(t.memory + ind, ch, cnt);
    micron::memcpy(t.memory + ind + cnt, __mem::memory + ind, __mem::length - ind);

    t._buf_set_length(__mem::length + cnt);
    return t;
  }

  template<typename F = T, usize M>
  inline istring
  insert(usize ind, const char (&str)[M], usize cnt = 1)
  {
    usize str_len = M - 1;
    if ( ind > __mem::length ) exc<except::library_error>("micron::istring insert() out of range");

    istring t(__mem::length + cnt * str_len + 1);      // +1 for the trailing NUL

    micron::memcpy(t.memory, __mem::memory, ind);

    for ( usize i = 0; i < cnt; ++i ) micron::memcpy(t.memory + ind + i * str_len, str, str_len);

    micron::memcpy(t.memory + ind + cnt * str_len, __mem::memory + ind, __mem::length - ind);

    t._buf_set_length(__mem::length + cnt * str_len);
    return t;
  }

  template<typename F, usize M>
  inline istring
  insert(usize ind, const sstring<M, F> &o)
  {
    if ( ind > __mem::length ) exc<except::library_error>("micron::istring insert() out of range");
    usize end = micron::strlen(o.c_str());
    istring t(__mem::length + end + 1);      // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],           // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;
    // shift the [ind, length) tail right by 'end' BEFORE writing the insert (note: length - ind, not t.length - ind)
    micron::memmove(&(t.memory)[ind + end], &(t.memory)[ind], __mem::length - ind);
    micron::memcpy(&(t.memory)[ind], &o.memory[0], end);

    t._buf_set_length(t.length + end);
    return t;
  }

  inline const T &
  at(const usize n) const
  {
    if ( n >= __mem::length ) exc<except::library_error>("micron:string at() out of range");
    return (__mem::memory)[n];
  };

  inline usize
  at(const_iterator n) const
  {
    if ( n - &__mem::memory[0] > 256 or n - &__mem::memory[0] < 0 ) exc<except::library_error>("micron:sstring at() out of range");
    return n - &__mem::memory[0];
  };

  inline usize
  at(iterator n) const
  {
    if ( n - &__mem::memory[0] > 256 or n - &__mem::memory[0] < 0 ) exc<except::library_error>("micron:sstring at() out of range");
    return n - &__mem::memory[0];
  };

  inline const T &
  operator[](const usize n) const
  {
    return (__mem::memory)[n];
  };

  inline istring
  operator+=(const buffer &data)
  {
    istring t(__mem::length + data.size() + 1);      // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],                   // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &data[0], data.size());
    t._buf_set_length(t.length + data.size());
    return t;
  };

  template<usize M>
  inline istring
  operator+=(const char (&data)[M])
  {
    istring t(__mem::length + M);       // length + (M-1) content + 1 NUL slot
    micron::memcpy(&(t.memory)[0],      // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &(data)[0], M - 1);      // append at end; drop the source NUL
    t._buf_set_length(t.length + (M - 1));
    return t;
  };

  template<typename F = T>
  inline istring
  operator+=(const F *&data)
  {
    usize end = micron::strlen(data);
    istring t(__mem::length + end + 1);      // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],           // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &(data)[0], end);
    t._buf_set_length(t.length + end);
    return t;
  };

  template<typename F, usize M>
  inline istring
  operator+=(const sstring<M, F> &data)
  {
    istring t(__mem::length + data.size() + 1);      // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],                   // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &(data.memory)[0], data.size());
    t._buf_set_length(t.length + data.size());
    return t;
  };

  template<typename F = T>
  inline istring
  operator+=(const istring<F> &data)
  {
    istring t(__mem::length + data.size() + 1);      // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],                   // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &(data.memory)[0], data.size());
    t._buf_set_length(t.length + data.length);
    return t;
  };

  template<typename F = T>
  inline istring
  operator+=(const slice<F> &data)
  {
    if ( data.size() == 0 ) return __dup();
    istring t(__mem::length + data.size() + 1);      // +1 for the trailing NUL
    micron::memcpy(&(t.memory)[0],                   // null is here so, overwrite it
                   &__mem::memory[0], __mem::length);
    t.length += __mem::length;

    micron::memcpy(&(t.memory)[t.length], &data[0], data.size());
    t._buf_set_length(t.length + data.size());
    return t;
  };

  template<typename R>
    requires(__valid_needle<R>)
  inline istring
  operator-(const R &rhs) const
  {
    const __needle nd = __as_needle(rhs);
    if ( nd.n == 0 || nd.n > __mem::length ) return __dup();
    istring t(__mem::length + 1);
    usize w = 0, r = 0;
    while ( r < __mem::length ) {
      if ( r + nd.n <= __mem::length
           && micron::memcmp<byte>(reinterpret_cast<const byte *>(&__mem::memory[r]), reinterpret_cast<const byte *>(nd.p),
                                   nd.n * sizeof(T))
                  == 0 )
        r += nd.n;
      else
        t.memory[w++] = __mem::memory[r++];
    }
    t.memory[w] = T{ 0 };
    t.length = w;
    return t;
  };

  template<typename R>
    requires(__valid_needle<R>)
  inline istring
  operator-=(const R &rhs) const
  {
    return (*this) - rhs;
  };

  template<typename I>
    requires micron::integral<I> && (!micron::is_pointer_v<I>)
  inline istring
  operator*(I n) const
  {
    if ( n < 1 ) {
      istring t(1);
      t.memory[0] = T{ 0 };
      t.length = 0;
      return t;
    }
    const usize cnt = static_cast<usize>(n);
    const usize total = __mem::length * cnt;
    istring t(total + 1);
    for ( usize k = 0; k < cnt; ++k ) micron::memcpy(&t.memory[k * __mem::length], &__mem::memory[0], __mem::length);
    t.memory[total] = T{ 0 };
    t.length = total;
    return t;
  };

  template<typename I>
    requires micron::integral<I> && (!micron::is_pointer_v<I>)
  inline istring
  operator*=(I n) const
  {
    return (*this) * n;
  };

  template<typename R>
    requires(__valid_needle<R>)
  inline istring
  operator/(const R &rhs) const
  {
    const __needle nd = __as_needle(rhs);
    const usize total = __mem::length + nd.n;
    istring t(total + 1);
    if ( __mem::length ) micron::memcpy(&t.memory[0], &__mem::memory[0], __mem::length);
    if ( nd.n ) micron::memcpy(&t.memory[__mem::length], nd.p, nd.n);
    t.memory[total] = T{ 0 };
    t.length = total;
    return t;
  };

  template<typename R>
    requires(__valid_needle<R>)
  inline istring
  operator/=(const R &rhs) const
  {
    return (*this) / rhs;
  };

  template<typename R>
  inline istring
  operator^(const R &rhs) const
  {
    const __bspan k = __as_key(rhs);
    istring t(__mem::length + 1);
    micron::simd::xor_bytes_cycle(reinterpret_cast<byte *>(&t.memory[0]), reinterpret_cast<const byte *>(&__mem::memory[0]),
                                  __mem::length * sizeof(T), k.p, k.n);
    t.memory[__mem::length] = T{ 0 };
    t.length = __mem::length;
    return t;
  };

  template<typename R>
  inline istring
  operator^=(const R &rhs) const
  {
    return (*this) ^ rhs;
  };

  template<typename R>
  inline istring
  operator&(const R &rhs) const
  {
    const __bspan k = __as_key(rhs);
    istring t(__mem::length + 1);
    micron::simd::and_bytes_cycle(reinterpret_cast<byte *>(&t.memory[0]), reinterpret_cast<const byte *>(&__mem::memory[0]),
                                  __mem::length * sizeof(T), k.p, k.n);
    t.memory[__mem::length] = T{ 0 };
    t.length = __mem::length;
    return t;
  };

  template<typename R>
  inline istring
  operator&=(const R &rhs) const
  {
    return (*this) & rhs;
  };

  template<typename R>
  inline istring
  operator|(const R &rhs) const
  {
    const __bspan k = __as_key(rhs);
    istring t(__mem::length + 1);
    micron::simd::or_bytes_cycle(reinterpret_cast<byte *>(&t.memory[0]), reinterpret_cast<const byte *>(&__mem::memory[0]),
                                 __mem::length * sizeof(T), k.p, k.n);
    t.memory[__mem::length] = T{ 0 };
    t.length = __mem::length;
    return t;
  };

  template<typename R>
  inline istring
  operator|=(const R &rhs) const
  {
    return (*this) | rhs;
  };

  template<typename F = T>
  inline istring<F, Alloc>
  substr(usize pos = 0, usize cnt = 0) const
  {
    if ( pos > __mem::length or cnt > __mem::length - pos ) exc<except::library_error>("error micron::string substr invalid range.");
    istring<F, Alloc> buf(cnt + 1);      // size from the real copy length + 1 NUL slot
    if ( cnt ) micron::memcpy(buf.data(), &__mem::memory[pos], cnt);
    buf._buf_set_length(cnt);
    return buf;
  };

  // grow container
  inline void reserve(usize n) = delete;

  inline void try_reserve(usize n) = delete;

  inline bool
  operator==(const char *data) const
  {
    const usize M = strlen(data);
    if ( M != __mem::length ) return false;
    if ( M == 0 ) return true;
    return micron::memcmp<byte>(reinterpret_cast<const byte *>(__mem::memory), reinterpret_cast<const byte *>(data), M) == 0;
  };

  template<typename F = T, usize M>
  inline bool
  operator==(const F (&data)[M]) const
  {
    constexpr usize len = M - 1;
    if ( len != __mem::length ) return false;
    if constexpr ( len == 0 ) return true;
    return micron::memcmp<byte>(reinterpret_cast<const byte *>(__mem::memory), reinterpret_cast<const byte *>(&data[0]), len) == 0;
  };

  template<typename F = T>
  inline bool
  operator==(const istring<F> &data) const
  {
    if ( data.length != __mem::length ) return false;
    if ( __mem::length == 0 ) return true;
    return micron::memcmp<byte>(reinterpret_cast<const byte *>(__mem::memory), reinterpret_cast<const byte *>(data.memory),
                                __mem::length * sizeof(T))
           == 0;
  };
};

template<is_string S>
auto
to_persist(const S &str)
{
  return istring<typename S::value_type>(str.c_str());
}

};      // namespace micron
