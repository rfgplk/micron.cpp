//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__container.hpp"

#include "../__special/initializer_list"
#include "../algorithm/algorithm.hpp"
#include "../algorithm/memory.hpp"
#include "../pointer.hpp"
#include "../types.hpp"
#include "vector.hpp"

#include "../memory/addr.hpp"

namespace micron
{

// Vector on the stack, always safe, mutable, fixed size
// equivalent of inplace_vector, should be used over vector in almost all use
// cases
template <is_regular_object T, size_t N = 64> class svector
{
  T stack[N];
  size_t length = 0;

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

  ~svector()
  {
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_destructible_v<T> ) {
      for ( size_t n = 0; n < length; n++ )
        stack[n].~T();
    }
  }
  svector(void) : length(0) {};
  template <typename... Args> svector(Args... args)
  {
    for ( size_t n = 0; n < N; n++ )
      stack[n] = T(args...);
    length = N;
  };
  svector(const size_t cnt)
  {
    if ( cnt > N ) [[unlikely]]
      exc<except::library_error>("error micron::svector(): cnt too large");

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      for ( size_t i = 0; i < cnt; i++ )
        new (&stack[i]) T{};
    } else if constexpr ( micron::is_literal_type_v<T> ) {
      micron::memset(stack, T{}, cnt);
    }
    length = cnt;
  };
  svector(const size_t cnt, const T &v)
  {
    if ( cnt > N ) [[unlikely]]
      exc<except::library_error>("error micron::svector(): cnt too large");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      for ( size_t i = 0; i < cnt; i++ )
        new (&stack[i]) T{ v };
    } else if constexpr ( micron::is_literal_type_v<T> ) {
      micron::memset(stack, v, cnt);
    }
    length = cnt;
  };
  template <typename C = T>
    requires(sizeof(C) == sizeof(T))
  svector(const vector<C> &o)
  {
    if ( o.length >= N ) {
      __impl_container::copy(stack, o.memory, N);
      // micron::copy<N>(&(*o.memory)[0], micron::real_addr_as<T>(stack[0]));
    } else {
      __impl_container::copy(stack, o.memory, o.length);
      // micron::copy(&(*o.memory)[0], micron::real_addr_as<T>(stack[0]), o.length);
    }
    length = o.length;
  };

  svector(const svector &o)
  {
    __impl_container::copy(stack, o.stack, N);
    length = o.length;
  };
  template <typename C = T, size_t M = N> svector(const svector<C, M> &o)
  {
    if constexpr ( N < M ) {
      __impl_container::copy(stack, o.stack, M);
      // micron::copy<M>(micron::real_addr_as<T>(o.stack[0]), micron::real_addr_as<T>(stack[0]));
    } else if constexpr ( M >= N ) {
      __impl_container::copy(stack, o.stack, N);
      // micron::copy<N>(micron::real_addr_as<T>(o.stack[0]), micron::real_addr_as<T>(stack[0]));
    }
    length = o.length;
  };
  svector(const std::initializer_list<T> &lst)
  {
    if ( lst.size() > N )
      exc<except::runtime_error>("error micron::svector() initializer_list too large.");
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
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
    __impl_container::move(micron::real_addr_as<T>(stack[0]), micron::real_addr_as<T>(o.stack[0]), N);
    // micron::copy<N>(micron::real_addr_as<T>(o.stack[0]), stack[0]);
    // micron::zero<N>(micron::real_addr_as<T>(o.stack[0]));
    length = o.length;
    o.length = 0;
  };
  template <typename C = T, size_t M> svector(svector<C, M> &&o)
  {
    if constexpr ( N >= M ) {
      micron::copy<N>(micron::real_addr_as<T>(o.stack[0]), micron::real_addr_as<T>(stack[0]));
      micron::zero<M>(micron::real_addr_as<T>(o.stack[0]));
      length = o.length;
      o.length = 0;
    } else {
      micron::copy<M>(micron::real_addr_as<T>(o.stack[0]), micron::real_addr_as<T>(stack[0]));
      micron::zero<M>(micron::real_addr_as<T>(o.stack[0]));
      length = o.length;
      o.length = 0;
    }
  };
  svector &
  operator=(const svector &o)
  {
    __impl_container::copy(stack, o.stack, N);
    length = o.length;
    return *this;
  };
  svector &
  operator=(svector &&o)
  {
    micron::copy<N>(micron::real_addr_as<T>(o.stack[0]), micron::real_addr_as<T>(stack[0]));
    micron::zero<N>(micron::real_addr_as<T>(o.stack[0]));
    length = o.length;
    o.length = 0;
    return *this;
  };
  template <typename R>
    requires(micron::is_integral_v<R>)
  T &
  operator[](const R n)
  {
    return stack[n];
  }
  template <typename R>
    requires(micron::is_integral_v<R>)
  const T &
  operator[](const R n) const
  {
    return stack[n];
  }
  T &
  at(const size_t n)
  {
    if ( n >= N )
      exc<except::runtime_error>("micron::svector at() out of range.");
    return stack[N];
  }
  const_iterator
  front() const
  {
    return micron::real_addr_as<T>(stack[0]);
  };
  iterator
  front()
  {
    return micron::real_addr_as<T>(stack[0]);
  };
  const_iterator
  back() const
  {
    return micron::real_addr_as<T>(stack[length - 1]);
  };
  iterator
  back()
  {
    return micron::real_addr_as<T>(stack[length - 1]);
  };
  iterator
  begin()
  {
    return micron::real_addr_as<T>(stack[0]);
  };
  const_iterator
  cbegin() const
  {
    return micron::real_addr_as<T>(stack[0]);
  };
  iterator
  end()
  {
    return micron::real_addr_as<T>(stack[length]);
  };
  const_iterator
  cend() const
  {
    return micron::real_addr_as<T>(stack[length]);
  };
  inline bool
  full() const
  {
    return ((length + 1) == N);
  }
  inline bool
  overflowed() const
  {
    return ((length + 1) > N);
  }
  inline bool
  full_or_overflowed() const
  {
    return ((length + 1) >= N);
  }
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
    czero<N>(micron::real_addr_as<T>(stack[0]));
  }
  void resize(const size_t n) = delete;
  void reserve(const size_t n) = delete;
  template <typename C = T>
  svector &
  erase(size_t n)
  {
    if ( n >= N or n >= length )
      exc<except::runtime_error>("micron::svector erase() out of range.");
    stack[n].~T();
    micron::memmove(micron::real_addr_as<T>(stack[n]), micron::real_addr_as<T>(stack[n + 1]), length - n - 1);
    czero<sizeof(svector) / sizeof(byte)>((byte *)micron::voidify(micron::real_addr_as<T>(stack[length-- - 1])));
    return *this;
  }
  template <typename C = T, size_t M>
    requires(sizeof(C) == sizeof(T))
  svector &
  append(const svector<C, M> &o)
  {
    if ( length + M >= N )
      exc<except::runtime_error>("micron::svector push_back() out of range.");
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
  emplace_back(Args &&...args)
  {
    if ( length + 1 >= N )
      exc<except::runtime_error>("micron::svector emplace_back() out of range.");
    stack[length++] = T(micron::forward<Args>(args)...);
    return *this;
  }
  svector &
  move_back(T &&i)
  {
    if ( length + 1 >= N )
      exc<except::runtime_error>("micron::svector push_back() out of range.");
    stack[length++] = micron::move(i);
    return *this;
  }
  template <typename C = T>
  svector &
  push_back(const C &i)
  {
    if ( length + 1 >= N )
      exc<except::runtime_error>("micron::svector push_back() out of range.");
    stack[length++] = i;
    return *this;
  }
  template <typename C = T>
  svector &
  append(const C &i)
  {
    if ( length + 1 >= N )
      exc<except::runtime_error>("micron::svector push_back() out of range.");
    stack[length++] = i;
    return *this;
  }
  template <typename C = T>
  svector &
  insert(size_t ind, const C &i)
  {
    if ( length + 1 >= N )
      exc<except::runtime_error>("micron::svector insert() out of range.");
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
      exc<except::runtime_error>("micron::svector insert() out of range.");
    micron::bytemove(itr + sizeof(C), itr, sizeof(C) / sizeof(byte));
    *itr = i;
    length += 1;
    return *this;
  }
  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }
};
}     // namespace micron
