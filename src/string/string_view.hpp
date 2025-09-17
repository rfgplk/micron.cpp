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
  const T __start;
  const T __end;

public:
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
  string_view(const string_view &o) : __start(o.__start), __end(o.__end) {}
  string_view(string_view &&) = delete;
  string_view &
  operator=(const string_view &o)
  {
    __start = o.__start;
    __end = o.__end;
    return *this;
  }
  string_view &operator=(string_view &&) = delete;
  ~string_view() {}
  const auto &
  operator[](const size_t n) const
  {
    if ( n >= (__end - __start) )
      throw except::library_error("micron::string_view operator[] out of memory range");
    return __start[n];
  }
  T
  begin()
  {
    return __start;
  }
  T
  end()
  {
    return __end;
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
};

};     // namespace micron
