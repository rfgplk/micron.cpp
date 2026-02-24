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
#include "../slice_forward.hpp"
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
  typedef size_t size_type;
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
    __impl_container::destroy(micron::addr(stack[0]), length);
    length = 0;
  }

  svector(void) : length(0) {};

  template <typename... Args> svector(Args... args)
  {
    for ( size_type n = 0; n < N; n++ )
      stack[n] = T(args...);
    length = N;
  };

  svector(const size_type cnt)
  {
    if ( cnt > N ) [[unlikely]]
      exc<except::library_error>("error micron::svector(): cnt too large");

    __impl_container::set(micron::addr(stack[0]), T{}, cnt);
    length = cnt;
  };

  svector(const size_type cnt, const T &v)
  {
    if ( cnt > N ) [[unlikely]]
      exc<except::library_error>("error micron::svector(): cnt too large");
    __impl_container::set(micron::addr(stack[0]), v, cnt);
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

  template <typename C = T, size_type M = N> svector(const svector<C, M> &o)
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
      size_type i = 0;
      for ( T value : lst )
        stack[i++] = value;
    } else {
      size_type i = 0;
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

  template <typename C = T, size_type M> svector(svector<C, M> &&o)
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
  at(const size_type n)
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

  size_type
  size() const
  {
    return length;
  };

  size_type
  max_size() const
  {
    return N;
  };

  void
  __set_size(size_type s)
  {
    length = s;
  }

  void
  fast_clear()
  {
    if constexpr ( !micron::is_class_v<T> ) {
      length = 0;
    } else
      clear();
  }

  void
  clear()
  {
    __impl_container::destroy(micron::real_addr_as<T>(stack), length);
    length = 0;
  }

  iterator
  data()
  {
    return &stack[0];
  }

  const_iterator
  data() const
  {
    return &stack[0];
  }

  bool
  operator!() const
  {
    return empty();
  }

  bool
  empty(void) const noexcept
  {
    return (length == 0);
  }

  // overload this to always point to mem
  byte *
  operator&()
  {
    return reinterpret_cast<byte *>(stack);
  }

  const byte *
  operator&() const
  {
    return reinterpret_cast<byte *>(stack);
  }

  auto *
  addr()
  {
    return this;
  }

  const auto *
  addr() const
  {
    return this;
  }

  inline slice<T>
  operator[]()
  {
    return slice<T>(begin(), end());
  }

  inline const slice<T>
  operator[]() const
  {
    return slice<T>(begin(), end());
  }

  // copies vector out
  inline __attribute__((always_inline)) const slice<T>
  operator[](size_type from, size_type to) const
  {
    // should be N not cnt
    if ( from >= to or from > N or to > N )
      exc<except::library_error>("micron::vector operator[] out of allocated memory range.");
    return slice<T>(get(from), get(to));
  }

  inline __attribute__((always_inline)) slice<T>
  operator[](size_type from, size_type to)
  {
    // meant to be safe so this is here
    if ( from >= to or from > N or to > N )
      exc<except::library_error>("micron::vector operator[] out of allocated memory range.");
    return slice<T>(get(from), get(to));
  }

  void resize(const size_type n) = delete;
  void reserve(const size_type n) = delete;

  template <typename C = T>
  svector &
  erase(size_type n)
  {
    if ( n >= N or n >= length )
      exc<except::runtime_error>("micron::svector erase() out of range.");
    stack[n].~T();
    micron::memmove(micron::real_addr_as<T>(stack[n]), micron::real_addr_as<T>(stack[n + 1]), length - n - 1);
    __impl_container::destroy(micron::real_addr_as<T>(stack[length-- - 1]), 1);
    return *this;
  }

  template <typename C = T, size_type M>
    requires(sizeof(C) == sizeof(T))
  svector &
  append(const svector<C, M> &o)
  {
    if ( length + M >= N )
      exc<except::runtime_error>("micron::svector push_back() out of range.");
    for ( size_type i = length, j = 0; j < o.size(); i++, j++ )
      stack[i] = o[j];
    length += o.size();
    return *this;
  }

  template <typename C = T, size_type M>
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
  insert(size_type ind, const C &i)
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

  inline iterator
  get(const size_type n)
  {
    if ( n > N )
      exc<except::library_error>("micron::svector get() out of range");
    return micron::addr(stack[n]);
  }

  inline const_iterator
  get(const size_type n) const
  {
    if ( n > N )
      exc<except::library_error>("micron::svector get() out of range");
    return micron::addr(stack[n]);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    if ( n > N )
      exc<except::library_error>("micron::svector cget() out of range");
    return micron::addr(stack[n]);
  }

  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class_type() noexcept
  {
    return micron::is_class_v<T>;
  }

  static constexpr bool
  is_trivial() noexcept
  {
    return micron::is_trivial_v<T>;
  }
};
}     // namespace micron
