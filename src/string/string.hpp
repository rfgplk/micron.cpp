//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../type_traits.hpp"

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../concepts.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/memory.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"

#include "unitypes.hpp"

#include "sstring.hpp"

namespace micron
{
// null terminated

constexpr const char _null_str[1] = "";
constexpr const wide _null_wstr[1] = L"";
constexpr const unicode32 _null_u32str[1] = U"";

// string on the heap, mutable, standard replacement of std::string
// accepts only char simple types
template <is_scalar_literal T = schar, bool Sf = true, class Alloc = micron::allocator_serial<>>
class hstring : private Alloc, public __mutable_memory_resource<T>
{
  using __mem = __mutable_memory_resource<T, Alloc>;

  // all safety functions return true IF condition failed
  inline constexpr __attribute__((always_inline)) bool
  __valid_cnt(usize cnt) const
  {
    return (cnt > micron::numeric_limits<ssize_t>::max());
  }

  inline constexpr __attribute__((always_inline)) bool
  __size_check(usize cnt) const
  {
    return (__mem::length >= __mem::capacity or (__mem::length + cnt) >= __mem::capacity);
  }

  inline __attribute__((always_inline)) bool
  __iterator_check(T *itr) const
  {
    return (itr < __mem::memory or itr > __mem::memory + __mem::length);
  }

  inline __attribute__((always_inline)) bool
  __iterator_check(const T *itr) const
  {
    return (itr < __mem::memory or itr > __mem::memory + __mem::length);
  }

  inline __attribute__((always_inline)) bool
  __iterator_bounds_check(const T *start, const T *end) const
  {
    return (start < __mem::memory or end > __mem::memory + __mem::length or start > end);
  }

  inline __attribute__((always_inline)) bool
  __null_check(const void *ptr) const
  {
    return (ptr == nullptr);
  }

  inline __attribute__((always_inline)) bool
  __index_check(usize n) const
  {
    return (n >= __mem::length);
  }

  inline __attribute__((always_inline)) bool
  __range_pos_cnt(usize pos, usize cnt) const
  {
    return (pos > __mem::length or (pos + cnt) > __mem::length);
  }

  inline __attribute__((always_inline)) bool
  __capacity_exceed(usize n) const
  {
    return (n > __mem::capacity);
  }

  inline __attribute__((always_inline)) bool
  __erase_iter_check(const T *itr, usize cnt) const
  {
    return (itr < __mem::memory or itr > __mem::memory + __mem::length
            or static_cast<ssize_t>(cnt) > ((__mem::memory + __mem::length) - itr));
  }

  inline __attribute__((always_inline)) bool
  __erase_ind_check(usize ind, usize cnt) const
  {
    return (ind > __mem::length or cnt > __mem::length - ind);
  }

  inline __attribute__((always_inline)) bool
  __iter_substr_check(const T *start, const T *end_) const
  {
    return (start < __mem::memory or end_ > __mem::memory + __mem::length or start > end_);
  }

  inline __attribute__((always_inline)) bool
  __at_iter_check(const T *itr) const
  {
    auto diff = itr - &__mem::memory[0];
    return (diff > 256 or diff < 0);
  }

  inline __attribute__((always_inline)) int
  __lexcmp(const T *a, usize alen, const T *b, usize blen) const noexcept
  {
    usize common = alen < blen ? alen : blen;
    for ( usize i = 0; i < common; ++i ) {
      // promote to unsigned so high-byte chars sort correctly
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

public:
  using category_type = string_tag;
  using mutability_type = mutable_tag;
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

  ~hstring()
  {
    if ( __mem::is_zero() )
      return;
    clear();
  }

  // contiguous_memory<T> memory;
  constexpr hstring() : __mem(Alloc::auto_size()) {};
  constexpr hstring(const usize n) : __mem(n) {};

  hstring(usize cnt, T ch) : __mem(cnt)
  {
    micron::typeset<T>(&__mem::memory[0], ch, cnt);
    __mem::length = cnt;
  }

  constexpr hstring(const char *str) : __mem(micron::strlen(str))
  {
    usize end = micron::strlen(str);
    micron::memcpy(&(__mem::memory)[0], &str[0], end);
    __mem::length = end;
  };

  template <usize M, typename F> constexpr hstring(const F (&str)[M]) : __mem((M))
  {
    micron::bytecpy(&(__mem::memory)[0], &str[0], M * sizeof(F));
    __mem::length = M - 1;
  };

  constexpr hstring(const hstring &o) : __mem(o.capacity)
  {
    micron::strcpy(__mem::memory, o.memory);
    __mem::length = o.length;
  };

  template <typename F> constexpr hstring(hstring<F> &&o) : __mem(micron::move(o)) {}

  constexpr hstring(hstring &&o) : __mem(micron::move(o)) {}

  template <typename F> constexpr hstring(const hstring<F> &o) : __mem(o.capacity)
  {
    micron::strcpy(__mem::memory, o.memory);
    __mem::length = o.length;
  };

  template <usize N, typename F> constexpr hstring(const sstring<N, F> &o) : __mem(o.length)
  {
    micron::memcpy(&(__mem::memory)[0], &o.memory[0], o.length);
    __mem::length = o.length;
  };

  template <is_iterable_container F>
    requires(sizeof(typename F::value_type) == sizeof(T))
  constexpr hstring(const F &o) : __mem(o.size())
  {
    micron::memcpy(&(__mem::memory)[0], o.data(), o.size());
    __mem::length = o.size();
  };

  template <is_iterable_container F>
    requires(sizeof(typename F::value_type) == sizeof(T))
  constexpr hstring(F &&o) : __mem(o.size())
  {
    micron::memcpy(&(__mem::memory)[0], o.data(), o.size());
    __mem::length = o.size();
  };

  // allow construction from - to iterator (be careful!)
  // NOTE: exc() in initialiser list intentionally left in place — __safety_check
  //       cannot be invoked before the object is fully constructed.
  constexpr hstring(iterator __start, iterator __end)
      : __mem((__start < __end ? ((max_t)Alloc::auto_size() < static_cast<usize>(__end - __start) ? static_cast<usize>(__end - __start)
                                                                                                  : Alloc::auto_size())
                               : (exc<except::library_error>("micron::hstring hstring() wrong iterators"), 0)))
  {
    micron::memcpy(__mem::memory, __start, __end - __start);
    __mem::length = __end - __start;
  }

  constexpr hstring(const_iterator __start, const_iterator __end)
      : __mem((__start < __end
                   ? (Alloc::auto_size() < static_cast<usize>(__end - __start) ? static_cast<usize>(__end - __start) : Alloc::auto_size())
                   : (exc<except::library_error>("micron::hstring hstring() wrong iterators"), 0)))
  {
    micron::memcpy(__mem::memory, __start, __end - __start);
    __mem::length = __end - __start;
  }

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

  template <template <usize, typename> class S, typename F, usize N>
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

  template <usize M, typename F = T>
  hstring &
  operator=(const F (&str)[M])
  {
    if ( __mem::capacity < M )
      reserve(M);
    clear();
    micron::memcpy(&(__mem::memory)[0], &str[0], M);
    __mem::length = M - 1;
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

  bool
  empty() const
  {
    return (__mem::length == 0 or __mem::memory == nullptr);
  };

  usize
  size() const
  {
    return __mem::length;
  }

  void
  set_size(size_type n)
  {
    __mem::length = n;
  }

  void
  adjust_size()
  {
    auto ln = micron::strlen(__mem::memory);
    __mem::length = ln;
  }

  usize
  max_size() const
  {
    return __mem::capacity;
  }

  iterator
  data()
  {
    return __mem::memory;
  };

  const_iterator
  data() const
  {
    return __mem::memory;
  };

  const_iterator
  cdata() const
  {
    return __mem::memory;
  };

  inline sstring<256, T>
  stack(void) const
  {
    if ( __mem::size >= 255 )
      exc<except::library_error>("micron::hstring stack() out of memory.");
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

  inline slice<T>
  into_chars() const
  {
    return slice<T>(&__mem::memory[0], &__mem::memory[__mem::length]);
  }

  inline slice<byte>
  into_bytes()
  {
    return slice<byte>(reinterpret_cast<byte *>(&__mem::memory[0]), reinterpret_cast<byte *>(&__mem::memory[__mem::length]));
  }

  inline auto
  clone()
  {
    return hstring<T>(*this);
  }

  template <typename F>
  inline F
  clone()
  {
    return F(*this);
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

  inline void
  _buf_set_length(const usize s)
  {
    __mem::length = s;
  }

  template <typename F>
  usize
  find(F ch, usize pos = 0) const
  {
    for ( ; pos < __mem::length; pos++ ) {
      if ( __mem::memory[pos] == ch )
        return pos;
    }
    return npos;
  }

  template <typename F>
  usize
  find(const hstring<F> &str, usize pos = 0) const
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
    return &(__mem::memory)[__mem::length];
  }

  inline void
  clear()
  {
    zero(__mem::memory, __mem::capacity);
    __mem::length = 0;
  }

  inline void
  fast_clear()
  {
    __mem::length = 0;
  }

  inline hstring &
  append(const buffer &f, usize n)
  {
    if ( (__mem::length + n) >= __mem::capacity )
      reserve(__mem::capacity + n);
    micron::memcpy(&(__mem::memory)[__mem::length], &f[0], n);
    __mem::length += n;
    return *this;
  }

  template <typename F>
  inline hstring &
  append(const slice<F> &f, usize n)
  {
    if ( (__mem::length + n) >= __mem::capacity )
      reserve(__mem::capacity + n);
    micron::memcpy(&(__mem::memory)[__mem::length], &f[0], n);
    __mem::length += n;
    return *this;
  }

  template <typename F>
  inline hstring &
  append(const F *f, usize n)
  {
    if ( (__mem::length + n) >= __mem::capacity )
      reserve(__mem::capacity + n);
    micron::memcpy(&(__mem::memory)[__mem::length], f, n);
    __mem::length += n - 1;
    return *this;
  }

  template <typename F = T, usize M>
  inline hstring &
  append(const F (&str)[M])
  {
    if ( (__mem::length + M) >= __mem::capacity )
      reserve(__mem::capacity + M);
    micron::memcpy(&(__mem::memory)[__mem::length], &str[0], M);
    __mem::length += M - 1;
    return *this;
  }

  template <typename F = T>
  inline hstring &
  append(const hstring<F> &o)
  {
    if ( (__mem::length + o.length) >= __mem::capacity )
      reserve(__mem::capacity + o.length);
    micron::memcpy(&(__mem::memory)[__mem::length], &(o.memory)[0], o.length);
    __mem::length += o.length;
    return *this;
  }

  template <usize M, typename F = T>
  inline hstring &
  append(const sstring<M, F> &o)
  {
    usize end = micron::strlen(o.c_str());
    if ( (__mem::length + end) >= __mem::capacity )
      reserve(__mem::capacity + end);
    micron::memcpy(&(__mem::memory)[__mem::length], &(o.memory)[0], end);
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

  template <typename F = T, usize M>
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
    micron::memcpy(&(__mem::memory)[__mem::length], &(o.memory)[0], o.length);
    __mem::length += o.length;
    return *this;
  }

  template <usize M, typename F = T>
  inline hstring &
  push_back(const sstring<M, F> &o)
  {
    usize end = micron::strlen(o.c_str());
    if ( (__mem::length + end) >= __mem::capacity )
      reserve(__mem::capacity + end);
    micron::memcpy(&(__mem::memory)[__mem::length], &(o.memory)[0], end);
    __mem::length += end - 1;
    return *this;
  }

  template <typename F = T>
  inline hstring &
  insert(usize ind, F ch, usize cnt = 1)
  {
    __safety_check<&hstring::__valid_cnt, except::library_error>("micron::hstring insert() invalid count", cnt);
    if ( __mem::length + cnt >= __mem::capacity )
      reserve(__mem::capacity + 1);
    micron::bytemove(&__mem::memory[ind + cnt], &__mem::memory[ind], __mem::length - ind);
    micron::memset(&__mem::memory[ind], ch, cnt);
    __mem::length += cnt;
    return *this;
  }

  template <typename F = T, usize M>
  inline hstring &
  insert(usize ind, const F (&str)[M], usize cnt = 1)
  {
    __safety_check<&hstring::__valid_cnt, except::library_error>("micron::hstring insert() invalid count", cnt);
    if ( __mem::length >= __mem::capacity or (__mem::length + (cnt * M)) >= __mem::capacity )
      reserve(__mem::capacity + 1);
    usize str_len = M - 1;

    micron::bytemove(&__mem::memory[ind + cnt * str_len], &__mem::memory[ind], __mem::length - ind);
    for ( usize i = 0; i < cnt; ++i )
      micron::memcpy(&__mem::memory[ind + i * str_len], str, str_len);
    __mem::length += (cnt * str_len);
    return *this;
  }

  template <typename F, usize M>
  inline hstring &
  insert(usize ind, const sstring<M, F> &o)
  {
    usize end = micron::strlen(o.c_str());
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
  insert(iterator itr, F ch, usize cnt = 1)
  {
    __safety_check<&hstring::__valid_cnt, except::library_error>("micron::hstring insert() invalid count", cnt);

    if ( __mem::length >= __mem::capacity or (__mem::length + cnt) >= __mem::capacity ) {
      usize dif = itr - __mem::memory;
      reserve(__mem::capacity + cnt);
      itr = __mem::memory + dif;
    }

    __safety_check<&hstring::__iterator_check, except::library_error>("micron::hstring insert() iterator out of range", itr);

    micron::bytemove(itr + cnt, itr, __mem::length - (itr - __mem::memory));
    micron::memset(itr, ch, cnt);
    __mem::length += cnt;
    return *this;
  }

  template <typename F = T, usize M>
  inline hstring &
  insert(iterator itr, const F (&str)[M], usize cnt = 1)
  {
    __safety_check<&hstring::__valid_cnt, except::library_error>("micron::hstring insert() invalid count", cnt);

    if ( __mem::length >= __mem::capacity or (__mem::length + (cnt * M)) >= __mem::capacity ) {
      usize dif = itr - __mem::memory;
      reserve(__mem::capacity + M);
      itr = __mem::memory + dif;
    }
    max_t str_len = M - 1;

    usize tail_len = __mem::length - (itr - __mem::memory);
    micron::bytemove(itr + cnt * str_len, itr, tail_len);
    for ( usize i = 0; i < cnt; ++i )
      micron::memcpy(itr + i * str_len, str, str_len);
    __mem::length += (cnt * str_len);
    return *this;
  }

  template <typename F = T>
  inline hstring &
  insert(iterator itr, const hstring<F> &o)
  {
    if ( __mem::length + o.length >= __mem::capacity ) {
      usize dif = itr - __mem::memory;
      reserve(__mem::capacity + o.length);
      itr = __mem::memory + dif;
    }
    micron::bytemove(itr + (o.length - 1), itr, __mem::length - (itr - &(__mem::memory)[0]));
    micron::memcpy(itr, &(o.memory)[0], o.length - 1);
    __mem::length += o.length - 1;
    return *this;
  }

  template <typename F, usize M>
  inline hstring &
  insert(iterator itr, const sstring<M, F> &o)
  {
    usize end = micron::strlen(o.c_str());
    if ( __mem::length + end >= __mem::capacity ) {
      usize dif = itr - __mem::memory;
      reserve(__mem::capacity + o.length);
      itr = __mem::memory + dif;
    }
    micron::bytemove(itr + (end - 1), itr, __mem::length - (itr - &(__mem::memory)[0]));
    micron::memcpy(itr, &o.memory[0], end - 1);
    __mem::length += end - 1;
    return *this;
  }

  template <typename F, usize M>
  inline hstring &
  insert(iterator itr, sstring<M, F> &&o)
  {
    if ( __mem::length + o.length >= __mem::capacity ) {
      usize dif = itr - __mem::memory;
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
  at(const usize n)
  {
    __safety_check<&hstring::__index_check, except::library_error>("micron::hstring at() out of range", n);
    return (__mem::memory)[n];
  };

  inline const T &
  at(const usize n) const
  {
    __safety_check<&hstring::__index_check, except::library_error>("micron::hstring at() out of range", n);
    return (__mem::memory)[n];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, const_iterator>)
  inline usize
  at(Itr n)
  {
    __safety_check<&hstring::__at_iter_check, except::library_error>("micron::hstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &__mem::memory[0];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, const_iterator>)
  inline usize
  at(Itr n) const
  {
    __safety_check<&hstring::__at_iter_check, except::library_error>("micron::hstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &__mem::memory[0];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, iterator>)
  inline usize
  at(Itr n)
  {
    __safety_check<&hstring::__at_iter_check, except::library_error>("micron::hstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &__mem::memory[0];
  };

  template <typename Itr>
    requires(micron::same_as<Itr, iterator>)
  inline usize
  at(Itr n) const
  {
    __safety_check<&hstring::__at_iter_check, except::library_error>("micron::hstring at() iterator out of range",
                                                                     static_cast<const T *>(n));
    return n - &__mem::memory[0];
  };

  inline T &
  operator[](const usize n)
  {
    return (__mem::memory)[n];
  };

  inline const T &
  operator[](const usize n) const
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

  template <typename F = T>
  inline hstring &
  operator+=(const F *data)
  {
    usize end = micron::strlen(data);
    if ( (__mem::length + end) >= __mem::capacity )
      reserve(__mem::capacity + end + 1);
    micron::memcpy(&(__mem::memory)[__mem::length], &(data)[0], end);
    __mem::length += end;
    return *this;
  };

  template <typename F, usize M>
  inline hstring &
  operator+=(const sstring<M, F> &data)
  {
    if ( (data.length + __mem::length) >= __mem::capacity ) [[unlikely]]
      reserve(__mem::capacity + data.length + 1);
    usize end = micron::strlen(data.c_str());
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
    usize ln = __mem::length == 0 ? 0 : __mem::length;
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
  substr(usize pos = 0, usize cnt = npos) const
  {
    if ( cnt == npos )     // logic sentinel — kept for both Sf modes
      cnt = (pos <= __mem::length) ? __mem::length - pos : 0;

    __safety_check<&hstring::__range_pos_cnt, except::library_error>("micron::hstring substr() invalid range", pos, cnt);

    hstring<F> buf(cnt + 1);
    micron::memcpy(buf.data(), &__mem::memory[pos], cnt);
    buf._buf_set_length(cnt);
    return buf;
  };

  template <typename F = T>
  inline hstring<F>
  substr(iterator _start, iterator _end = nullptr) const
  {
    if ( _end == nullptr )     // logic default — kept for both Sf modes
      _end = end();

    __safety_check<&hstring::__iter_substr_check, except::library_error>("micron::hstring substr() invalid range",
                                                                         static_cast<const T *>(_start), static_cast<const T *>(_end));

    usize len = static_cast<usize>(_end - _start);
    hstring<F> buf(len + 1);
    micron::memcpy(buf.data(), _start, len);
    buf._buf_set_length(len);
    return buf;
  };

  template <typename F = T>
  inline hstring<F>
  substr(const_iterator _start, const_iterator _end = nullptr) const
  {
    if ( _end == nullptr )
      _end = cend();

    __safety_check<&hstring::__iter_substr_check, except::library_error>("micron::hstring substr() invalid range", _start, _end);

    usize len = static_cast<usize>(_end - _start);
    hstring<F> buf(len + 1);
    micron::memcpy(buf.data(), _start, len);
    buf._buf_set_length(len);
    return buf;
  };

  template <typename I, typename F = T>
    requires micron::convertible_to<I, size_type> && micron::integral<I> && (!micron::is_pointer_v<I>)
  inline hstring<F> &
  truncate(I n)
  {
    if ( n >= __mem::length )
      return *this;

    __safety_check<&hstring::__capacity_exceed, except::library_error>("micron::hstring truncate() index out of range",
                                                                       static_cast<usize>(n));

    micron::typeset<T>(&__mem::memory[n], 0x0, __mem::length - n);
    __mem::length = n;
    return *this;
  }

  template <typename F = T>
  inline hstring<F> &
  truncate(iterator itr)
  {
    __safety_check<&hstring::__iterator_check, except::library_error>("micron::hstring truncate() iterator out of range", itr);

    if ( itr >= end() )
      return *this;

    usize n = static_cast<usize>(itr - begin());
    micron::typeset<T>(&__mem::memory[n], 0x0, __mem::length - n);
    __mem::length = n;
    return *this;
  }

  template <typename F = T>
  inline hstring<F> &
  truncate(const_iterator itr)
  {
    __safety_check<&hstring::__iterator_check, except::library_error>("micron::hstring truncate() iterator out of range", itr);

    if ( itr >= cend() )
      return *this;

    usize n = static_cast<usize>(itr - cbegin());
    micron::typeset<T>(&__mem::memory[n], 0x0, __mem::length - n);
    __mem::length = n;
    return *this;
  }

  void
  reserve(usize n)
  {
    if ( (n < __mem::capacity) ) {
      return;
    }
    if ( __mem::memory == nullptr ) [[unlikely]] {
      __mem::realloc(Alloc::auto_size());
      return;
    }
    __mem::expand(n);
  }

  void
  try_reserve(usize n)
  {
    if ( (n < __mem::capacity) ) {
      exc<except::memory_error>("micron::hstring try_reserve() was unable to allocate memory");
    }
    if ( __mem::memory == nullptr ) [[unlikely]] {
      __mem::realloc(Alloc::auto_size());
      return;
    }
    __mem::expand(n);
  }

  void
  resize(usize n, const T ch)
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

  template <typename F = T, typename I = usize>
    requires micron::is_arithmetic_v<I>
  inline hstring<F> &
  erase(I __ind, usize cnt = 1)
  {
    usize ind = static_cast<usize>(__ind);

    __safety_check<&hstring::__valid_cnt, except::library_error>("micron::hstring erase() invalid count", cnt);
    __safety_check<&hstring::__erase_ind_check, except::library_error>("micron::hstring erase() out of range", ind, cnt);

    if ( !cnt )
      return *this;

    micron::bytemove(&__mem::memory[ind], &__mem::memory[ind + (1 + (cnt - 1))], __mem::length - (ind + 1 + (cnt - 1)));
    micron::typeset<T>(&__mem::memory[__mem::length - (cnt)], 0x0, cnt);
    __mem::length -= cnt;
    return *this;
  }

  template <typename F = T>
  inline hstring<F> &
  erase(iterator itr, usize cnt = 1)
  {
    __safety_check<&hstring::__valid_cnt, except::library_error>("micron::hstring erase() invalid count", cnt);
    __safety_check<&hstring::__erase_iter_check, except::library_error>("micron::hstring erase() out of range", static_cast<const T *>(itr),
                                                                        cnt);

    if ( !cnt )
      return *this;

    micron::bytemove(itr, itr + (1 + (cnt - 1)), __mem::length - ((itr - &__mem::memory[0]) + 1 + (cnt - 1)));
    micron::typeset<T>(&__mem::memory[__mem::length - cnt], 0x0, cnt);
    __mem::length -= cnt;
    return *this;
  }

  template <typename F = T>
  inline hstring<F> &
  erase(const_iterator itr, usize cnt = 1)
  {
    __safety_check<&hstring::__valid_cnt, except::library_error>("micron::hstring erase() invalid count", cnt);
    __safety_check<&hstring::__erase_iter_check, except::library_error>("micron::hstring erase() out of range", itr, cnt);

    if ( !cnt )
      return *this;

    micron::bytemove(itr, itr + (1 + (cnt - 1)), __mem::length - ((itr - &__mem::memory[0]) + 1 + (cnt - 1)));
    micron::typeset<T>(&__mem::memory[__mem::length - cnt], 0x0, cnt);
    __mem::length -= cnt;
    return *this;
  }

  inline usize
  find_substr(const T *needle, usize needle_len, usize pos = 0) const
  {
    if ( needle_len == 0 or needle_len > __mem::length )
      return npos;
    usize limit = __mem::length - needle_len;
    for ( usize i = pos; i <= limit; ++i ) {
      usize j = 0;
      while ( j < needle_len and __mem::memory[i + j] == needle[j] )
        ++j;
      if ( j == needle_len )
        return i;
    }
    return npos;
  }

  template <typename F = T>
  inline hstring<F> &
  remove(const char *needle)
  {
    __safety_check<&hstring::__null_check, except::library_error>("micron::hstring remove() null needle",
                                                                  static_cast<const void *>(needle));

    usize needle_len = micron::strlen(needle);
    if ( needle_len == 0 )
      return *this;

    usize pos = find_substr(reinterpret_cast<const T *>(needle), needle_len);
    if ( pos == npos )
      return *this;

    micron::bytemove(&__mem::memory[pos], &__mem::memory[pos + needle_len], __mem::length - (pos + needle_len));
    micron::typeset<T>(&__mem::memory[__mem::length - needle_len], 0x0, needle_len);
    __mem::length -= needle_len;
    return *this;
  }

  template <typename F = T, usize M, typename G>
  inline hstring<F> &
  remove(const micron::sstring<M, G> &needle)
  {
    if ( needle.empty() )
      return *this;

    usize pos = find_substr(needle.data(), needle.size());
    if ( pos == npos )
      return *this;

    micron::bytemove(&__mem::memory[pos], &__mem::memory[pos + needle.size()], __mem::length - (pos + needle.size()));
    micron::typeset<T>(&__mem::memory[__mem::length - needle.size()], 0x0, needle.size());
    __mem::length -= needle.size();
    return *this;
  }

  template <typename F = T, typename G>
  inline hstring<F> &
  remove(const hstring<G> &needle)
  {
    if ( needle.empty() )
      return *this;

    usize pos = find_substr(needle.data(), needle.size());
    if ( pos == npos )
      return *this;

    micron::bytemove(&__mem::memory[pos], &__mem::memory[pos + needle.size()], __mem::length - (pos + needle.size()));
    micron::typeset<T>(&__mem::memory[__mem::length - needle.size()], 0x0, needle.size());
    __mem::length -= needle.size();
    return *this;
  }

  template <typename F = T>
  inline hstring<F> &
  remove_all(const char *needle)
  {
    __safety_check<&hstring::__null_check, except::library_error>("micron::hstring remove_all() null needle",
                                                                  static_cast<const void *>(needle));

    usize needle_len = micron::strlen(needle);
    if ( needle_len == 0 )
      return *this;

    usize pos = 0;
    while ( (pos = find_substr(reinterpret_cast<const T *>(needle), needle_len, pos)) != npos ) {
      micron::bytemove(&__mem::memory[pos], &__mem::memory[pos + needle_len], __mem::length - (pos + needle_len));
      micron::typeset<T>(&__mem::memory[__mem::length - needle_len], 0x0, needle_len);
      __mem::length -= needle_len;
    }
    return *this;
  }

  template <typename F = T, usize M, typename G>
  inline hstring<F> &
  remove_all(const micron::sstring<M, G> &needle)
  {
    if ( needle.empty() )
      return *this;

    usize pos = 0;
    while ( (pos = find_substr(needle.data(), needle.size(), pos)) != npos ) {
      micron::bytemove(&__mem::memory[pos], &__mem::memory[pos + needle.size()], __mem::length - (pos + needle.size()));
      micron::typeset<T>(&__mem::memory[__mem::length - needle.size()], 0x0, needle.size());
      __mem::length -= needle.size();
    }
    return *this;
  }

  template <typename F = T, typename G>
  inline hstring<F> &
  remove_all(const hstring<G> &needle)
  {
    if ( needle.empty() )
      return *this;

    usize pos = 0;
    while ( (pos = find_substr(needle.data(), needle.size(), pos)) != npos ) {
      micron::bytemove(&__mem::memory[pos], &__mem::memory[pos + needle.size()], __mem::length - (pos + needle.size()));
      micron::typeset<T>(&__mem::memory[__mem::length - needle.size()], 0x0, needle.size());
      __mem::length -= needle.size();
    }
    return *this;
  }

  explicit inline
  operator bool() const noexcept
  {
    return !empty();
  }

  template <is_string S>
  inline bool
  operator==(const S &str) const
  {
    return __lexcmp(__mem::memory, __mem::length, str.c_str(), str.size()) == 0;
  }

  template <is_string S>
  inline bool
  operator!=(const S &str) const
  {
    return __lexcmp(__mem::memory, __mem::length, str.c_str(), str.size()) != 0;
  }

  inline bool
  operator==(const T *data) const
  {
    __safety_check<&hstring::__null_check, except::library_error>("micron::hstring operator==() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(__mem::memory, __mem::length, data, micron::strlen(data)) == 0;
  }

  template <usize M>
  inline bool
  operator!=(const char (&data)[M]) const
  {
    __safety_check<&hstring::__null_check, except::library_error>("micron::hstring operator!=() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(__mem::memory, __mem::length, data, micron::strlen(data)) != 0;
  }

  template <typename S>
    requires(!micron::same_as<S, T>)
  inline bool
  operator!=(const S *data) const
  {
    __safety_check<&hstring::__null_check, except::library_error>("micron::hstring operator!=() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(__mem::memory, __mem::length, data, micron::strlen(data)) != 0;
  }

  inline bool
  operator!=(const T *data) const
  {
    __safety_check<&hstring::__null_check, except::library_error>("micron::hstring operator!=() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(__mem::memory, __mem::length, data, micron::strlen(data)) != 0;
  }

  inline bool
  operator<(const T *data) const
  {
    __safety_check<&hstring::__null_check, except::library_error>("micron::hstring operator<() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(__mem::memory, __mem::length, data, micron::strlen(data)) < 0;
  }

  inline bool
  operator>(const T *data) const
  {
    __safety_check<&hstring::__null_check, except::library_error>("micron::hstring operator>() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(__mem::memory, __mem::length, data, micron::strlen(data)) > 0;
  }

  inline bool
  operator<=(const T *data) const
  {
    __safety_check<&hstring::__null_check, except::library_error>("micron::hstring operator<=() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(__mem::memory, __mem::length, data, micron::strlen(data)) <= 0;
  }

  inline bool
  operator>=(const T *data) const
  {
    __safety_check<&hstring::__null_check, except::library_error>("micron::hstring operator>=() null pointer",
                                                                  static_cast<const void *>(data));
    return __lexcmp(__mem::memory, __mem::length, data, micron::strlen(data)) >= 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator==(const F (&data)[M]) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(&data[0]), M - 1) == 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator!=(const F (&data)[M]) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(&data[0]), M - 1) != 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator<(const F (&data)[M]) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(&data[0]), M - 1) < 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator>(const F (&data)[M]) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(&data[0]), M - 1) > 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator<=(const F (&data)[M]) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(&data[0]), M - 1) <= 0;
  }

  template <typename F = T, usize M>
  inline bool
  operator>=(const F (&data)[M]) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(&data[0]), M - 1) >= 0;
  }

  template <typename F = T>
  inline bool
  operator==(const hstring<F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) == 0;
  }

  template <typename F = T>
  inline bool
  operator!=(const hstring<F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) != 0;
  }

  template <typename F = T>
  inline bool
  operator<(const hstring<F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) < 0;
  }

  template <typename F = T>
  inline bool
  operator>(const hstring<F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) > 0;
  }

  template <typename F = T>
  inline bool
  operator<=(const hstring<F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) <= 0;
  }

  template <typename F = T>
  inline bool
  operator>=(const hstring<F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) >= 0;
  }

  template <usize N, typename F = T>
  inline bool
  operator==(const sstring<N, F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) == 0;
  }

  template <usize N, typename F = T>
  inline bool
  operator!=(const sstring<N, F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) != 0;
  }

  template <usize N, typename F = T>
  inline bool
  operator<(const sstring<N, F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) < 0;
  }

  template <usize N, typename F = T>
  inline bool
  operator>(const sstring<N, F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) > 0;
  }

  template <usize N, typename F = T>
  inline bool
  operator<=(const sstring<N, F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) <= 0;
  }

  template <usize N, typename F = T>
  inline bool
  operator>=(const sstring<N, F> &data) const
  {
    return __lexcmp(__mem::memory, __mem::length, reinterpret_cast<const T *>(data.memory), data.length) >= 0;
  }
};

};     // namespace micron
