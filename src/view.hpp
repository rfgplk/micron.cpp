//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "types.hpp"
#include "except.hpp"
#include "type_traits.hpp"

namespace micron
{

template <typename T> constexpr auto view_ptr(T &) -> typename micron::add_pointer<T>::type;
template <typename T>
constexpr auto
view_type(T &) -> typename micron::remove_reference<T>::type
{
  return typename micron::remove_reference<T>::type{};
}

template <typename T = byte *>
  requires micron::is_pointer_v<T>
class view
{
  const T start;
  const T _end;

public:
  typedef const T const_ptr;
  typedef T ptr;
  view() = delete;
  view(T s) : start(s), _end(nullptr) {}
  view(T s, T e) : start(s), _end(e) {}
  template <typename F> view(F &f) : start(reinterpret_cast<T>(&f)), _end(reinterpret_cast<T>(&f + sizeof(F))) {}
  view(const view &) = delete;
  view(view &&) = delete;
  view &operator=(const view &) = delete;
  view &operator=(view &&) = delete;
  ~view() {}
  const T
  operator[](const size_t n) const
  {
    if ( n > (_end - start) )
      throw except::library_error("micron::view operator[] out of memory range");
    return &start[n];
  }
  const_ptr
  begin() const
  {
    return start;
  }
  const_ptr
  end() const
  {
    return _end;
  }
};
};     // namespace micron
