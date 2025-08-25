//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/algorithm.hpp"
#include "../algorithm/mem.hpp"
#include "../io/print.hpp"
#include "../pointer.hpp"
#include "../types.hpp"
#include "vector.hpp"
#include <initializer_list>

namespace micron
{

// Vector on the stack, always safe, mutable, fixed size
// equivalent of inplace_vector, should be used over vector in almost all use
// cases
template <typename T, size_t N = 64>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T>
class svector
{
  T stack[N];
  size_t length;

  // shallow copy routine
  inline void
  shallow_copy(T *dest, T *src, size_t cnt)
  {
    micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src), cnt);
  };
  // deep copy routine, nec. if obj. has const/dest (can be ignored but WILL
  // cause segfaulting if underlying doesn't account for double deletes)
  inline void
  deep_copy(T *dest, T *src, size_t cnt)
  {
    for ( size_t i = 0; i < cnt; i++ )
      dest[i] = src[i];
  };
  inline void
  __impl_copy(T *dest, T *src, size_t cnt)
  {
    if constexpr ( micron::is_class<T>::value ) {
      deep_copy(dest, src, cnt);
    } else {
      shallow_copy(dest, src, cnt);
    }
  }

public:
  using category_type = vector_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  ~svector() {};
  svector() : length(0) {};
  template <typename... Args> svector(Args... args)
  {
    for ( size_t n = 0; n < N; n++ )
      stack[n] = T(args...);
    length = N;
  };
  svector(const size_t cnt)
  {
    for ( size_t n = 0; n < cnt; n++ )
      stack[n] = T();
    length = cnt;
  };
  svector(const size_t cnt, const T &v)
  {
    for ( size_t n = 0; n < cnt; n++ )
      stack[n] = v;
    length = cnt;
  };
  template <typename C = T>
    requires(sizeof(C) == sizeof(T))
  svector(const vector<C> &o)
  {
    if ( o.length >= N ) {
      __impl_copy(o.memory, stack, N);
      // micron::copy<N>(&(*o.memory)[0], &stack[0]);
    } else {
      __impl_copy(o.memory, stack, o.length);
      // micron::copy(&(*o.memory)[0], &stack[0], o.length);
    }
    length = o.length;
  };

  svector(const svector &o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    length = o.length;
  };
  template <typename C = T, size_t M = N> svector(const svector<C, M> &o)
  {
    if constexpr ( N < M ) {
      __impl_copy(o.stack, stack, M);
      // micron::copy<M>(&o.stack[0], &stack[0]);
    } else if constexpr ( M >= N ) {
      __impl_copy(o.stack, stack, N);
      // micron::copy<N>(&o.stack[0], &stack[0]);
    }
    length = o.length;
  };
  svector(const std::initializer_list<T> &lst)
  {
    if ( lst.size() > N )
      throw except::runtime_error("error micron::svector() initializer_list too large.");
    if constexpr ( micron::is_class_v<T> ) {
      size_t i = 0;
      for ( T value : lst )
        stack[i++] = value;
    } else {
      size_t i = 0;
      for ( T &&value : lst )
        stack[i++] = micron::move(value);
    }
  };
  svector(svector &&o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    micron::zero<N>(&o.stack[0]);
    length = o.length;
    o.length = 0;
  };
  template <typename C = T, size_t M> svector(svector<C, M> &&o)
  {
    if constexpr ( N >= M ) {
      micron::copy<N>(&o.stack[0], &stack[0]);
      micron::zero<M>(&o.stack[0]);
      length = o.length;
      o.length = 0;
    } else {
      micron::copy<M>(&o.stack[0], &stack[0]);
      micron::zero<M>(&o.stack[0]);
      length = o.length;
      o.length = 0;
    }
  };
  svector &
  operator=(const svector &o)
  {
    __impl_copy(o.stack, stack, N);
    length = o.length;
    return *this;
  };
  svector &
  operator=(svector &&o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    micron::zero<N>(&o.stack[0]);
    length = o.length;
    o.length = 0;
    return *this;
  };
  T &
  operator[](const size_t n)
  {
    return stack[n];
  }
  const T &
  operator[](const size_t n) const
  {
    return stack[n];
  }
  T &
  at(const size_t n)
  {
    if ( n >= N )
      throw except::runtime_error("micron::svector at() out of range.");
    return stack[N];
  }
  iterator
  begin()
  {
    return &stack[0];
  };
  const_iterator
  cbegin() const
  {
    return &stack[0];
  };
  iterator
  end()
  {
    return &stack[length];
  };
  const_iterator
  cend() const
  {
    return &stack[length];
  };
  size_t
  size() const
  {
    return length;
  };
  size_t
  max_size() const
  {
    return N;
  };
  void
  clear()
  {
    length = 0;
    czero<N>(&stack[0]);
  }
  void resize(const size_t n) = delete;
  void reserve(const size_t n) = delete;
  template <typename C = T>
  svector &
  erase(size_t n)
  {
    if ( n >= N or n >= length )
      throw except::runtime_error("micron::svector erase() out of range.");
    stack[n].~T();
    micron::memmove(&stack[n], &stack[n + 1], length - n - 1);
    czero<sizeof(svector) / sizeof(byte)>((byte *)micron::voidify(&stack[length-- - 1]));
    return *this;
  }
  template <typename C = T, size_t M>
    requires(sizeof(C) == sizeof(T))
  svector &
  append(const svector<C, M> &o)
  {
    if ( length + M >= N )
      throw except::runtime_error("micron::svector push_back() out of range.");
    for ( size_t i = length, j = 0; j < o.size(); i++, j++ )
      stack[i] = o[j];
    length += o.size();
    return *this;
  }

  template <typename C = T, size_t M>
    requires(sizeof(C) == sizeof(T))
  svector &
  operator+=(const svector<C, M> &o)
  {
    append(o);
    return *this;
  }
  template <typename... Args>
  svector &
  push_back(Args &&...args)
  {
    if ( length + 1 >= N )
      throw except::runtime_error("micron::svector push_back() out of range.");
    stack[length++] = micron::move(T(args)...);
    return *this;
  }
  template <typename C = T>
  svector &
  push_back(C &&i)
  {
    if ( length + 1 >= N )
      throw except::runtime_error("micron::svector push_back() out of range.");
    stack[length++] = micron::move(i);
    return *this;
  }
  template <typename C = T>
  svector &
  push_back(const C &i)
  {
    if ( length + 1 >= N )
      throw except::runtime_error("micron::svector push_back() out of range.");
    stack[length++] = i;
    return *this;
  }
  template <typename C = T>
  svector &
  append(const C &i)
  {
    if ( length + 1 >= N )
      throw except::runtime_error("micron::svector push_back() out of range.");
    stack[length++] = i;
    return *this;
  }
  template <typename C = T>
  svector &
  insert(size_t ind, const C &i)
  {
    if ( length + 1 >= N )
      throw except::runtime_error("micron::svector insert() out of range.");
    micron::bytemove(&(stack)[ind] + sizeof(C), &(stack)[ind], sizeof(C) / sizeof(byte));
    stack[ind] = i;
    length += 1;
    return *this;
  }

  template <typename C = T>
  svector &
  insert(iterator itr, const C &i)
  { 
     if ( length + 1 >= N )
       throw except::runtime_error("micron::svector insert() out of range.");
     micron::bytemove(itr + sizeof(C), itr, sizeof(C) / sizeof(byte));
     *itr = i;
     length += 1;
    return *this;
  }
};
}     // namespace micron
