//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "type_traits.hpp"

#include "memory/addr.hpp"
#include "memory/memory.hpp"
#include "types.hpp"

namespace micron
{
struct regular_iterator_tag {
};
// iterator is slightly different to stdlib iterators
// they provide more powerful ways to iterate over objects, but sacrificing some of the versatility that C++ have.
// micron iterators are bound to the fundamental type as declared, as such it isn't possible to iterate over different
// types (nor should you want to). in return you get way more powerful utilities which simplify code
template <typename P>
  requires(micron::is_fundamental_v<P>)
class iterator
{
  using iterator_category = micron::regular_iterator_tag;
  using value_type = P;
  using pointer = value_type *;
  using reference = value_type &;

  pointer start_ptr;
  pointer end_ptr;

public:
  // using different_type = std::ptrdiff_t;

  iterator() = delete;

  template <typename T>
    requires(micron::is_object_v<T>)
  iterator(T &t) : start_ptr(reinterpret_cast<pointer>(t.begin())), end_ptr(reinterpret_cast<pointer>(t.end()))
  {
  }
  template <typename T>
    requires(micron::is_pointer_v<T>) && (micron::is_convertible_v<T, P>)
  iterator(T &&t, const size_t rng = 0) : start_ptr(reinterpret_cast<pointer>(t)), end_ptr(reinterpret_cast<pointer>(micron::addr(t) + rng))
  {
  }
  template <typename T>
    requires(micron::is_fundamental_v<T>) && (micron::is_convertible_v<T, P>)
  iterator(T &&t, const size_t rng = 0)
      : start_ptr(reinterpret_cast<pointer>(micron::addr(t))), end_ptr(reinterpret_cast<pointer>(micron::addr(t) + rng))
  {
  }
  iterator(pointer sptr, pointer eptr) : start_ptr(sptr), end_ptr(eptr) {}
  iterator(const iterator &o) : start_ptr(o.start_ptr), end_ptr(o.end_ptr) {}
  iterator(iterator &&o) : start_ptr(o.start_ptr), end_ptr(o.end_ptr)
  {
    o.start_ptr = nullptr;
    o.end_ptr = nullptr;
  }
  iterator &
  operator=(const iterator &o)
  {
    start_ptr = o.start_ptr;
    end_ptr = o.end_ptr;
    return *this;
  }
  iterator &
  operator=(iterator &&o)
  {
    start_ptr = o.start_ptr;
    o.start_ptr = nullptr;
    end_ptr = o.end_ptr;
    o.end_ptr = nullptr;
    return *this;
  }
  ~iterator()
  {
    start_ptr = nullptr;
    end_ptr = nullptr;
  };

  reference
  operator*(void) const
  {
    return *start_ptr;
  }
  pointer
  operator->(void)
  {
    return start_ptr;
  }
  pointer
  retreat(const umax_t n)
  {
    start_ptr -= n;
    return start_ptr;
  }
  pointer
  nth(const size_t n)
  {
    for ( size_t i = 0; i < n; i++ )
      next();
    return this->operator()();
  }
  pointer
  end(void)
  {
    return end_ptr;
  }
  pointer
  advance(const umax_t n)
  {
    start_ptr += n;
    return start_ptr;
  }
  inline pointer
  prev(void)
  {
    return --start_ptr;
  }
  inline pointer
  next(void)
  {
    return ++start_ptr;
  }
  inline value_type
  next_value(void)
  {
    return *(++start_ptr);
  }
  inline size_t
  count(void)
  {
    size_t c = 0;
    while ( next() != end_ptr )
      c++;     // hehe
    return c;
  }
  inline size_t
  size_hint(void) const
  {
    return end_ptr - start_ptr;
  }
  template <typename F>
  void
  for_each(F f)
  {
    do {
      *start_ptr = f(*this->operator()());
    } while ( next() != end() );
  }
  bool
  operator!(void) const
  {
    return start_ptr == nullptr;
  }
  iterator &
  operator++(void)
  {
    ++start_ptr;
    return *this;
  }
  iterator &
  operator--(void)
  {
    --start_ptr;
    return *this;
  }
  template <typename F>
  inline void
  skip(F f)
  {
    if ( f(*peek()) )
      next();
  }
  inline pointer
  peek(void)
  {
    return start_ptr;
  }
  inline pointer
  operator()(void)
  {
    return start_ptr;
  }
  bool
  operator==(const iterator &o) const
  {
    return start_ptr == o.start_ptr;
  }
  bool
  operator!=(const iterator &o) const
  {
    return !(*this == o);
  }
};

};
