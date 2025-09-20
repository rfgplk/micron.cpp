//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "../type_traits.hpp"

#include "../algorithm/mem.hpp"
#include "../allocator.hpp"
#include "../memory/memory.hpp"
#include "../memory_block.hpp"
#include "../pointer.hpp"
#include "../slice.hpp"

#include "unitypes.hpp"

namespace micron
{

template <is_string S> class string_view
{
  using T = typename S::const_pointer;
  T __start;
  T __end;

public:
  ~string_view() {}
  string_view() = delete;
  string_view(T s) : __start(s), __end(micron::strlen(s)) {}
  string_view(T s, T e) : __start(s), __end(e) {}
  template <is_string F>
    requires(!micron::is_same_v<F, S>)
  string_view(typename F::iterator a, typename F::iterator b)
      : __start(reinterpret_cast<typename S::iterator>(a)), __end(reinterpret_cast<typename S::iterator>(b))
  {
  }
  string_view(const char *ptr, size_t count) : __start(ptr), __end(ptr + count) {}
  template <is_string F>
  string_view(const F &f)
      : __start(reinterpret_cast<typename S::const_iterator>(f.cbegin())),
        __end(reinterpret_cast<typename S::const_iterator>(f.cend()))
  {
  }
  template <is_string F>
  string_view(const F &f, const size_t n)
      : __start(reinterpret_cast<typename S::const_iterator>(f.cbegin())),
        __end(reinterpret_cast<typename S::const_iterator>(f.cbegin() + n))
  {
    if ( (n + f.cbegin()) >= f.cend() )
      throw except::library_error("micron::string_view set() out of memory range");
  }
  string_view(const string_view &o) : __start(o.__start), __end(o.__end) {}
  string_view(string_view &&) = delete;
  string_view &
  operator=(const string_view &o)
  {
    __start = o.__start;
    __end = o.__end;
    return *this;
  }
  string_view &
  operator=(const S &o)
  {
    if ( o.empty() )
      return *this;
    __start = o.cbegin();
    __end = o.cend();
    return *this;
  }
  string_view &
  set(const S &o, const size_t n = 0)
  {
    if ( o.empty() )
      return *this;
    if ( n >= o.size() )
      throw except::library_error("micron::string_view set() out of memory range");
    __start = o.cbegin() + n;
    __end = o.cend();
    return *this;
  }
  string_view &
  advance(const size_t n)
  {
    if ( n >= (__end - __start) )
      throw except::library_error("micron::string_view advance() out of memory range");
    __start += n;
    return *this;
  }
  // unsafe
  string_view &
  __advance(const size_t n)
  {
    __start += n;
    return *this;
  }
  string_view &
  __move(const size_t n)
  {
    __start += n;
    __end += n;
    return *this;
  }
  string_view &
  __push(const size_t n)
  {
    __end += n;
    return *this;
  }
  const auto &
  operator[](const size_t n) const
  {
    return __start[n];
  }
  const auto &
  at(const size_t n) const
  {
    if ( n >= (__end - __start) )
      throw except::library_error("micron::string_view operator[] out of memory range");
    return __start[n];
  }
  T
  ptr(const size_t n) const
  {
    return __start + n;
  }
  const T
  begin() const
  {
    return __start;
  }
  const T
  end() const
  {
    return __end;
  }
  const auto &
  front() const
  {
    return *__start;
  }
  const auto &
  last() const
  {
    return *__end;
  }
  size_t
  size() const
  {
    return __end - __start;
  }
  string_view
  substr(const size_t a, const size_t b)
  {
    return string_view(__start + a, __start + b);
  }
  string_view
  substr(const size_t a)
  {
    if ( a >= (__end - __start) )
      throw except::library_error("micron::string_view substr() out of memory range");
    return string_view(__start + a, __end);
  }
};

};     // namespace micron
